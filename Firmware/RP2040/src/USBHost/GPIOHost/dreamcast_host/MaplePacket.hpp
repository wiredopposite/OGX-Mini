/* Minimal Maple packet for host GET_CONDITION. Ported from DreamPicoPort MaplePacket.hpp. */

#pragma once

#include <cstdint>
#include <cstring>
#include "dreamcast_constants.h"
#include "maple_config.h"

struct MaplePacket {
    enum class ByteOrder { HOST, NETWORK };

    struct Frame {
        uint8_t command;
        uint8_t recipientAddr;
        uint8_t senderAddr;
        uint8_t length;

        static constexpr uint32_t LEN_POSITION_HOST = 0;
        static constexpr uint32_t SENDER_ADDR_POSITION_HOST = 8;
        static constexpr uint32_t RECIPIENT_ADDR_POSITION_HOST = 16;
        static constexpr uint32_t COMMAND_POSITION_HOST = 24;
        static constexpr uint32_t LEN_POSITION_NETWORK = 24;
        static constexpr uint32_t SENDER_ADDR_POSITION_NETWORK = 8;
        static constexpr uint32_t RECIPIENT_ADDR_POSITION_NETWORK = 16;
        static constexpr uint32_t COMMAND_POSITION_NETWORK = 0;

        static uint8_t getFramePacketLength(uint32_t frameWord, ByteOrder byteOrder = ByteOrder::HOST) {
            return (frameWord >> (byteOrder == ByteOrder::NETWORK ? LEN_POSITION_NETWORK : LEN_POSITION_HOST)) & 0xFF;
        }
        static uint8_t getFrameSenderAddr(uint32_t frameWord, ByteOrder byteOrder = ByteOrder::HOST) {
            return (frameWord >> (byteOrder == ByteOrder::NETWORK ? SENDER_ADDR_POSITION_NETWORK : SENDER_ADDR_POSITION_HOST)) & 0xFF;
        }
        static uint8_t getFrameRecipientAddr(uint32_t frameWord, ByteOrder byteOrder = ByteOrder::HOST) {
            return (frameWord >> (byteOrder == ByteOrder::NETWORK ? RECIPIENT_ADDR_POSITION_NETWORK : RECIPIENT_ADDR_POSITION_HOST)) & 0xFF;
        }
        static uint8_t getFrameCommand(uint32_t frameWord, ByteOrder byteOrder = ByteOrder::HOST) {
            return (frameWord >> (byteOrder == ByteOrder::NETWORK ? COMMAND_POSITION_NETWORK : COMMAND_POSITION_HOST)) & 0xFF;
        }

        void setFromFrameWord(uint32_t frameWord, ByteOrder byteOrder = ByteOrder::HOST) {
            length = getFramePacketLength(frameWord, byteOrder);
            senderAddr = getFrameSenderAddr(frameWord, byteOrder);
            recipientAddr = getFrameRecipientAddr(frameWord, byteOrder);
            command = getFrameCommand(frameWord, byteOrder);
        }

        uint32_t toWord(ByteOrder byteOrder = ByteOrder::HOST) const {
            if (byteOrder == ByteOrder::NETWORK)
                return (uint32_t)length << LEN_POSITION_NETWORK |
                       (uint32_t)senderAddr << SENDER_ADDR_POSITION_NETWORK |
                       (uint32_t)recipientAddr << RECIPIENT_ADDR_POSITION_NETWORK |
                       (uint32_t)command << COMMAND_POSITION_NETWORK;
            return (uint32_t)length << LEN_POSITION_HOST |
                   (uint32_t)senderAddr << SENDER_ADDR_POSITION_HOST |
                   (uint32_t)recipientAddr << RECIPIENT_ADDR_POSITION_HOST |
                   (uint32_t)command << COMMAND_POSITION_HOST;
        }

        bool isValid() const { return command != COMMAND_INVALID; }
    };

    Frame frame;
    static constexpr unsigned MAX_PAYLOAD = 32;
    uint32_t payload[MAX_PAYLOAD];
    unsigned payloadCount;
    ByteOrder payloadByteOrder;

    MaplePacket() : frame{}, payload{}, payloadCount(0), payloadByteOrder(ByteOrder::HOST) {
        frame.command = COMMAND_INVALID;
    }

    void setFrame(uint8_t cmd, uint8_t recipient, uint8_t sender, uint8_t len) {
        frame.command = cmd;
        frame.recipientAddr = recipient;
        frame.senderAddr = sender;
        frame.length = len;
        payloadCount = len;
    }

    uint32_t getFrameWord() const {
        Frame f = frame;
        f.length = (uint8_t)payloadCount;
        return f.toWord(payloadByteOrder);
    }

    static uint32_t getNumTotalBits(uint32_t numPayloadWords) {
        return ((numPayloadWords + 1) * 4 + 1) * 8;
    }
    uint32_t getNumTotalBits() const { return getNumTotalBits((uint32_t)payloadCount); }

    static uint32_t getTxTimeNs(uint32_t numPayloadWords, uint32_t nsPerBit) {
        return (getNumTotalBits(numPayloadWords) + 14) * nsPerBit;
    }
    uint32_t getTxTimeNs() const { return getTxTimeNs((uint32_t)payloadCount, MAPLE_NS_PER_BIT); }

    static uint32_t flipWordBytes(uint32_t word) {
        return (word << 24) | ((word << 8) & 0xFF0000) | ((word >> 8) & 0xFF00) | (word >> 24);
    }

    bool isValid() const { return frame.isValid() && frame.length == payloadCount; }
};
