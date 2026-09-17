/* Maple Bus host implementation. Ported from DreamPicoPort (OrangeFox86). */

#include "MapleBus.h"
#include "maple_config.h"
#include "maple_utils.h"
#include <pico/stdlib.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <hardware/gpio.h>
#include <cstring>

#define MAPLE_OUT_PIO pio0
#define MAPLE_IN_PIO  pio1

static MapleBus* s_bus_for_write_irq = nullptr;
static MapleBus* s_bus_for_read_irq  = nullptr;

static void maple_write_irq_handler(void) {
    if (s_bus_for_write_irq) s_bus_for_write_irq->writeIsr();
}
static void maple_read_irq_handler(void) {
    if (s_bus_for_read_irq) s_bus_for_read_irq->readIsr();
}

MapleBus::MapleBus(uint32_t pinA, int32_t dirPin, bool dirOutHigh)
    : mPinA(pinA)
    , mPinB(pinA + 1)
    , mDirPin(dirPin)
    , mDirOutHigh(dirOutHigh)
    , mMaskA(1u << pinA)
    , mMaskB(1u << (pinA + 1))
    , mMaskAB(mMaskA | mMaskB)
    , mOutOffset(pio_add_program(MAPLE_OUT_PIO, &maple_out_program))
    , mInOffset(pio_add_program(MAPLE_IN_PIO, &maple_in_program))
    , mSmOut(pio_claim_unused_sm(MAPLE_OUT_PIO, true))
    , mSmIn(pio_claim_unused_sm(MAPLE_IN_PIO, true))
    , mDmaWriteChannel(dma_claim_unused_channel(true))
    , mDmaReadChannel(dma_claim_unused_channel(true))
    , mCurrentPhase(Phase::IDLE)
    , mExpectingResponse(false)
    , mResponseTimeoutUs(MAPLE_RESPONSE_TIMEOUT_US)
    , mProcKillTime(UINT64_MAX)
    , mLastReceivedWordTimeUs(0)
    , mLastReadTransferCount(0)
{
    /* Out SM: clock from MAPLE_NS_PER_BIT and maple_out DOUBLE_PHASE_TICKS */
    pio_sm_config cOut = maple_out_program_get_default_config(mOutOffset);
    sm_config_set_sideset_pins(&cOut, mPinA);
    sm_config_set_set_pins(&cOut, mPinA, 2);
    sm_config_set_out_shift(&cOut, false, true, 32);
    float div = (MAPLE_CPU_FREQ_KHZ * (MAPLE_NS_PER_BIT / 3.0f * 2.0f)) / maple_out_DOUBLE_PHASE_TICKS / 1000000.0f;
    sm_config_set_clkdiv(&cOut, div);
    pio_sm_init(MAPLE_OUT_PIO, mSmOut, mOutOffset, &cOut);

    /* In SM */
    pio_sm_config cIn = maple_in_program_get_default_config(mInOffset);
    sm_config_set_in_pins(&cIn, mPinA);
    sm_config_set_jmp_pin(&cIn, mPinA);
    sm_config_set_out_shift(&cIn, true, false, 32);
    sm_config_set_in_shift(&cIn, false, true, 32);
    sm_config_set_clkdiv(&cIn, 1.0f);
    pio_sm_init(MAPLE_IN_PIO, mSmIn, mInOffset, &cIn);

    gpio_set_dir_in_masked(mMaskAB);
    gpio_set_pulls(mPinA, true, false);
    gpio_set_pulls(mPinB, true, false);

    if (mDirPin >= 0) {
        gpio_init((uint)mDirPin);
        setDirection(false);
        gpio_set_dir((uint)mDirPin, true);
    }

    /* One bus per host: register for IRQ */
    if (!s_bus_for_write_irq) {
        s_bus_for_write_irq = this;
        irq_set_exclusive_handler(PIO0_IRQ_0, maple_write_irq_handler);
        irq_set_enabled(PIO0_IRQ_0, true);
        pio_set_irq0_source_enabled(MAPLE_OUT_PIO, pis_interrupt0, true);
    }
    if (!s_bus_for_read_irq) {
        s_bus_for_read_irq = this;
        irq_set_exclusive_handler(PIO1_IRQ_0, maple_read_irq_handler);
        irq_set_enabled(PIO1_IRQ_0, true);
        pio_set_irq0_source_enabled(MAPLE_IN_PIO, pis_interrupt0, true);
    }

    /* DMA write: memory -> PIO TX FIFO, bswap */
    dma_channel_config dc = dma_channel_get_default_config(mDmaWriteChannel);
    channel_config_set_read_increment(&dc, true);
    channel_config_set_write_increment(&dc, false);
    channel_config_set_bswap(&dc, true);
    channel_config_set_dreq(&dc, pio_get_dreq(MAPLE_OUT_PIO, mSmOut, true));
    dma_channel_configure(mDmaWriteChannel, &dc,
        &MAPLE_OUT_PIO->txf[mSmOut], mWriteBuffer, 1, false);

    /* DMA read: PIO RX FIFO -> memory, bswap */
    dma_channel_config dr = dma_channel_get_default_config(mDmaReadChannel);
    channel_config_set_read_increment(&dr, false);
    channel_config_set_write_increment(&dr, true);
    channel_config_set_bswap(&dr, true);
    channel_config_set_dreq(&dr, pio_get_dreq(MAPLE_IN_PIO, mSmIn, false));
    dma_channel_configure(mDmaReadChannel, &dr,
        mReadBuffer, &MAPLE_IN_PIO->rxf[mSmIn], sizeof(mReadBuffer) / sizeof(mReadBuffer[0]), false);
}

static void crc8_word(uint32_t word, uint8_t& crc) {
    const uint8_t* p = (const uint8_t*)&word;
    for (unsigned i = 0; i < 4; i++) crc ^= p[i];
}
static void crc8_words(const uint32_t* src, uint32_t len, uint8_t& crc) {
    for (; len; len--, src++) crc8_word(*src, crc);
}

bool MapleBus::lineCheck() {
#if (MAPLE_OPEN_LINE_CHECK_TIME_US > 0)
    uint64_t end = time_us_64() + MAPLE_OPEN_LINE_CHECK_TIME_US + 1;
    while (time_us_64() < end) {
        if ((gpio_get_all() & mMaskAB) != mMaskAB) return false;
    }
#endif
    return true;
}

void MapleBus::setDirection(bool output) {
    if (!output) {
        gpio_set_dir_in_masked(mMaskAB);
        gpio_set_function(mPinB, GPIO_FUNC_SIO);
        gpio_set_function(mPinA, GPIO_FUNC_SIO);
    }
    if (mDirPin >= 0)
        gpio_put((uint)mDirPin, mDirOutHigh ? output : !output);
}

void MapleBus::writeIsr() {
    pio_interrupt_clear(MAPLE_OUT_PIO, 1u);
    pio_sm_set_enabled(MAPLE_OUT_PIO, mSmOut, false);
    if (!mExpectingResponse) {
        gpio_set_mask(mMaskAB);
        gpio_set_dir_out_masked(mMaskAB);
    } else {
        gpio_set_pulls(mPinB, false, true);
    }
    gpio_set_dir_in_masked(mMaskAB);
    gpio_set_function(mPinB, GPIO_FUNC_SIO);
    gpio_set_function(mPinA, GPIO_FUNC_SIO);

    if (mExpectingResponse) {
        pio_sm_clear_fifos(MAPLE_IN_PIO, mSmIn);
        pio_sm_restart(MAPLE_IN_PIO, mSmIn);
        pio_sm_clkdiv_restart(MAPLE_IN_PIO, mSmIn);
        pio_sm_exec(MAPLE_IN_PIO, mSmIn, pio_encode_jmp(mInOffset));
        pio_sm_set_consecutive_pindirs(MAPLE_IN_PIO, mSmIn, mPinA, 2, false);
        pio_gpio_init(MAPLE_IN_PIO, mPinA);
        pio_gpio_init(MAPLE_IN_PIO, mPinB);
        pio_sm_set_enabled(MAPLE_IN_PIO, mSmIn, true);
        setDirection(false);
        gpio_set_pulls(mPinB, true, false);
        mProcKillTime = (mResponseTimeoutUs == UINT64_MAX) ? UINT64_MAX : (time_us_64() + mResponseTimeoutUs);
        mCurrentPhase = Phase::WAITING_FOR_READ_START;
    } else {
        setDirection(false);
        mCurrentPhase = Phase::WRITE_COMPLETE;
    }
}

void MapleBus::readIsr() {
    pio_interrupt_clear(MAPLE_IN_PIO, 1u);
    if (mCurrentPhase == Phase::WAITING_FOR_READ_START) {
        mCurrentPhase = Phase::READ_IN_PROGRESS;
        mLastReceivedWordTimeUs = time_us_64();
    } else if (mCurrentPhase == Phase::READ_IN_PROGRESS) {
        pio_sm_set_enabled(MAPLE_IN_PIO, mSmIn, false);
        gpio_set_dir_in_masked(mMaskAB);
        gpio_set_function(mPinB, GPIO_FUNC_SIO);
        gpio_set_function(mPinA, GPIO_FUNC_SIO);
        mCurrentPhase = Phase::READ_COMPLETE;
    }
}

bool MapleBus::write(const MaplePacket& packet, bool autostartRead, uint64_t readTimeoutUs, MaplePacket::ByteOrder rxByteOrder) {
    if (mCurrentPhase != Phase::IDLE) return false;

    dma_channel_abort(mDmaWriteChannel);
    dma_channel_abort(mDmaReadChannel);

    uint8_t crc = 0;
    uint32_t frameWord = packet.getFrameWord();
    crc8_word(frameWord, crc);
    for (unsigned i = 0; i < packet.payloadCount; i++) crc8_word(packet.payload[i], crc);

    bool flipBytes = (packet.payloadByteOrder != MaplePacket::ByteOrder::NETWORK);
    uint32_t totalBits = MaplePacket::getNumTotalBits((uint32_t)packet.payloadCount);
    unsigned len = 0;
    mWriteBuffer[len++] = flipBytes ? MaplePacket::flipWordBytes(totalBits) : totalBits;
    mWriteBuffer[len++] = frameWord;
    for (unsigned i = 0; i < packet.payloadCount; i++)
        mWriteBuffer[len++] = packet.payload[i];
    mWriteBuffer[len++] = flipBytes ? (uint32_t)crc : ((uint32_t)crc << 24);

    if (!lineCheck()) return false;

    mExpectingResponse = autostartRead;
    mResponseTimeoutUs = readTimeoutUs;
    mRxByteOrder = rxByteOrder;
    mCurrentPhase = Phase::WRITE_IN_PROGRESS;

    if (autostartRead) {
        mLastReadTransferCount = sizeof(mReadBuffer) / sizeof(mReadBuffer[0]);
        dma_channel_transfer_to_buffer_now(mDmaReadChannel, mReadBuffer, mLastReadTransferCount);
        pio_sm_clear_fifos(MAPLE_IN_PIO, mSmIn);
        pio_sm_restart(MAPLE_IN_PIO, mSmIn);
        pio_sm_clkdiv_restart(MAPLE_IN_PIO, mSmIn);
        pio_sm_exec(MAPLE_IN_PIO, mSmIn, pio_encode_jmp(mInOffset));
        pio_sm_set_consecutive_pindirs(MAPLE_IN_PIO, mSmIn, mPinA, 2, false);
    }

    setDirection(true);
    pio_sm_clear_fifos(MAPLE_OUT_PIO, mSmOut);
    pio_sm_restart(MAPLE_OUT_PIO, mSmOut);
    pio_sm_clkdiv_restart(MAPLE_OUT_PIO, mSmOut);
    pio_sm_exec(MAPLE_OUT_PIO, mSmOut, pio_encode_jmp(mOutOffset));
    pio_sm_set_consecutive_pindirs(MAPLE_OUT_PIO, mSmOut, mPinA, 2, false);
    pio_gpio_init(MAPLE_OUT_PIO, mPinA);
    pio_gpio_init(MAPLE_OUT_PIO, mPinB);
    pio_sm_set_enabled(MAPLE_OUT_PIO, mSmOut, true);
    dma_channel_transfer_from_buffer_now(mDmaWriteChannel, mWriteBuffer, len);

    uint32_t txNs = MaplePacket::getTxTimeNs((uint32_t)packet.payloadCount, MAPLE_NS_PER_BIT);
    txNs = txNs * (100 + MAPLE_WRITE_TIMEOUT_EXTRA_PERCENT) / 100;
    mProcKillTime = time_us_64() + MAPLE_INT_DIVIDE_CEILING(txNs, 1000);
    return true;
}

bool MapleBus::startRead(uint64_t readTimeoutUs, MaplePacket::ByteOrder rxByteOrder) {
    if (mCurrentPhase != Phase::IDLE) return false;
    dma_channel_abort(mDmaWriteChannel);
    dma_channel_abort(mDmaReadChannel);
    mRxByteOrder = rxByteOrder;
    bool flipBytes = (rxByteOrder != MaplePacket::ByteOrder::NETWORK);
    dma_channel_config c = dma_channel_get_default_config(mDmaReadChannel);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_bswap(&c, flipBytes);
    channel_config_set_dreq(&c, pio_get_dreq(MAPLE_IN_PIO, mSmIn, false));
    dma_channel_configure(mDmaReadChannel, &c, mReadBuffer, &MAPLE_IN_PIO->rxf[mSmIn],
        sizeof(mReadBuffer) / sizeof(mReadBuffer[0]), false);
    mLastReadTransferCount = sizeof(mReadBuffer) / sizeof(mReadBuffer[0]);
    dma_channel_transfer_to_buffer_now(mDmaReadChannel, mReadBuffer, mLastReadTransferCount);
    mProcKillTime = (readTimeoutUs == 0) ? UINT64_MAX : (time_us_64() + readTimeoutUs);
    mCurrentPhase = Phase::WAITING_FOR_READ_START;
    setDirection(false);
    pio_sm_clear_fifos(MAPLE_IN_PIO, mSmIn);
    pio_sm_restart(MAPLE_IN_PIO, mSmIn);
    pio_sm_clkdiv_restart(MAPLE_IN_PIO, mSmIn);
    pio_sm_exec(MAPLE_IN_PIO, mSmIn, pio_encode_jmp(mInOffset));
    pio_sm_set_consecutive_pindirs(MAPLE_IN_PIO, mSmIn, mPinA, 2, false);
    pio_gpio_init(MAPLE_IN_PIO, mPinA);
    pio_gpio_init(MAPLE_IN_PIO, mPinB);
    pio_sm_set_enabled(MAPLE_IN_PIO, mSmIn, true);
    return true;
}

MapleBus::Status MapleBus::processEvents(uint64_t currentTimeUs) {
    Status st;
    st.phase = mCurrentPhase;
    st.readBuffer = nullptr;
    st.readBufferLen = 0;

    if (st.phase == Phase::READ_COMPLETE) {
        uint64_t t0 = time_us_64();
        while (!pio_sm_is_rx_fifo_empty(MAPLE_IN_PIO, mSmIn) && (time_us_64() - t0 < 1000))
            tight_loop_contents();
        uint32_t total = sizeof(mReadBuffer) / sizeof(mReadBuffer[0]);
        uint32_t left = dma_channel_hw_addr(mDmaReadChannel)->transfer_count;
        uint32_t dmaWordsRead = total - left;

        if (dmaWordsRead > 1) {
            uint8_t len = (mRxByteOrder != MaplePacket::ByteOrder::NETWORK)
                ? (mReadBuffer[0] & 0xFF) : (mReadBuffer[0] >> 24);
            uint8_t expectedCrc = (mRxByteOrder != MaplePacket::ByteOrder::NETWORK)
                ? (mReadBuffer[dmaWordsRead - 1] & 0xFF) : (mReadBuffer[dmaWordsRead - 1] >> 24);

            if (len <= dmaWordsRead - 2) {
                for (uint32_t i = 0; i < dmaWordsRead - 1; i++) mLastRead[i] = mReadBuffer[i];
                uint8_t crc = 0;
                crc8_words(mLastRead, dmaWordsRead - 1, crc);
                if (crc == expectedCrc) {
                    st.readBuffer = mLastRead;
                    st.readBufferLen = dmaWordsRead - 1;
                    st.rxByteOrder = mRxByteOrder;
                } else {
                    st.phase = Phase::READ_FAILED;
                    st.failureReason = FailureReason::CRC_INVALID;
                }
            } else {
                st.phase = Phase::READ_FAILED;
                st.failureReason = FailureReason::MISSING_DATA;
            }
        } else {
            st.phase = Phase::READ_FAILED;
            st.failureReason = FailureReason::MISSING_DATA;
        }
        mCurrentPhase = Phase::IDLE;
        return st;
    }

    if (st.phase == Phase::WRITE_COMPLETE) {
        mCurrentPhase = Phase::IDLE;
        return st;
    }

    if (st.phase == Phase::READ_IN_PROGRESS) {
        uint32_t tc = dma_channel_hw_addr(mDmaReadChannel)->transfer_count;
        if (tc == 0) {
            st.phase = Phase::READ_FAILED;
            st.failureReason = FailureReason::BUFFER_OVERFLOW;
            mCurrentPhase = Phase::IDLE;
        } else if (tc == mLastReadTransferCount) {
            if (currentTimeUs > mLastReceivedWordTimeUs &&
                (currentTimeUs - mLastReceivedWordTimeUs) >= MAPLE_INTER_WORD_READ_TIMEOUT_US) {
                pio_sm_set_enabled(MAPLE_IN_PIO, mSmIn, false);
                st.phase = Phase::READ_FAILED;
                st.failureReason = FailureReason::TIMEOUT;
                mCurrentPhase = Phase::IDLE;
            }
        } else {
            mLastReadTransferCount = tc;
            mLastReceivedWordTimeUs = currentTimeUs;
        }
        return st;
    }

    if (st.phase != Phase::IDLE && currentTimeUs >= mProcKillTime) {
        if (st.phase == Phase::WAITING_FOR_READ_START) {
            pio_sm_set_enabled(MAPLE_IN_PIO, mSmIn, false);
            st.phase = Phase::READ_FAILED;
            st.failureReason = FailureReason::TIMEOUT;
        } else {
            pio_sm_set_enabled(MAPLE_OUT_PIO, mSmOut, false);
            pio_sm_set_enabled(MAPLE_IN_PIO, mSmIn, false);
            setDirection(false);
            st.phase = Phase::WRITE_FAILED;
            st.failureReason = FailureReason::TIMEOUT;
        }
        mCurrentPhase = Phase::IDLE;
    }
    return st;
}
