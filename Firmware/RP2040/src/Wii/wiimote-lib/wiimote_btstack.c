#include "wiimote_btstack.h"
#include <stdio.h>
#include <math.h>
#include "btstack.h"

#include "sdp_consts.h"
#include "wiimote.h"
#include "motion.h"
#include <btstack_tlv.h>
#include "pico/flash.h"
#include "utils.h"

#define SDP_RESPONSE_BUFFER_SIZE (HCI_ACL_PAYLOAD_SIZE-L2CAP_HEADER_SIZE)

// TAG to store remote device address in TLV
#define TLV_TAG_HIDR ((((uint32_t) 'H') << 24 ) | (((uint32_t) 'I') << 16) | (((uint32_t) 'D') << 8) | 'R')

// used to store remote device in TLV
static const btstack_tlv_t * btstack_tlv_singleton_impl;
static void *                btstack_tlv_singleton_context;
bd_addr_t wii_baddr;
static uint8_t hid_service_buffer[700];
static uint8_t pnp_service_buffer[200];
static const char hid_device_name[] = "Nintendo RVL-CNT-01";
static uint16_t hid_cid;
static struct wiimote_state wiimote;
static uint8_t buf[256];
static int len;
static btstack_timer_source_t loop_wii;
static btstack_timer_source_t led_state;
void (*callback_led_func[2])();
int state = -1;
//convert_data
static const double pointer_margin = 0.5;
float pointer_x = 0.5;
float pointer_y = 0.5;
volatile WiimoteReport *input_report;

static void l2cap_sdp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void input_update_wiimote();


/**
 * The default command format always writes 2 IACs, but we only want one
 * @param num_current_iac must be 1
 * @param iac_lap
 */
static const hci_cmd_t hci_write_current_iac_lap_one_iac = {
    HCI_OPCODE_HCI_WRITE_CURRENT_IAC_LAP_TWO_IACS, "13"
};

static btstack_packet_callback_registration_t hci_event_callback_registration;

static uint16_t sdp_server_l2cap_cid;
static uint16_t sdp_server_response_size;
static uint8_t sdp_response_buffer[SDP_RESPONSE_BUFFER_SIZE];


static void send_data(){
    input_update_wiimote();
    process_report(&wiimote, buf, len);
    len = generate_report(&wiimote, buf);
    if (len > 0){
        hid_device_send_interrupt_message(hid_cid, &buf[0], len);
    }
}


static void get_data_wii(uint16_t cid, hid_report_type_t report_type, uint16_t report_id, int report_size, uint8_t * report){

    UNUSED(cid);

    if(report_type == HID_REPORT_TYPE_OUTPUT){
        buf[0] = 0xA2;
        buf[1] = report_id;
        memcpy(&buf[2], report, report_size);
        len = report_size + 2;
    }

}

void task_wiimote(struct btstack_timer_source *ts){

    hid_device_request_can_send_now_event(hid_cid);

    // Restart timer
    btstack_run_loop_set_timer(ts, 1);
    btstack_run_loop_add_timer(ts);

}

static void wiimote_emulator_reset(void)
{
    printf("Resetting...\n");

    if (sdp_server_l2cap_cid) {
        l2cap_disconnect(sdp_server_l2cap_cid);
        sdp_server_l2cap_cid = 0;
    }

    // Reset buffer data
    memset(buf, 0, sizeof(buf));
    len = 0;
}


static void sdp_respond(void)
{
    if (!sdp_server_response_size || !sdp_server_l2cap_cid) {
        return;
    }
            
    // update state before sending packet
    uint16_t size = sdp_server_response_size;
    sdp_server_response_size = 0;
    l2cap_send(sdp_server_l2cap_cid, sdp_response_buffer, size);

}



static void hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size)
{
    UNUSED(channel);
    UNUSED(size);

    static int set_iac_lap = 0;

    uint8_t status;

    // We only care about HCI packets
    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }

    // printf("hci_packet_handler %x\n", hci_event_packet_get_type(packet));

    switch(hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
            // Wait for the stack to enter the initializing state, before setting the IAC LAP
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                set_iac_lap = 1;
                btstack_tlv_get_instance(&btstack_tlv_singleton_impl, &btstack_tlv_singleton_context);
                // Load last Wii address from TLV for auto-connect (like a real Wiimote)
                if (btstack_tlv_singleton_impl && input_report) {
                    int len = btstack_tlv_singleton_impl->get_tag(btstack_tlv_singleton_context, TLV_TAG_HIDR,
                        (uint8_t *)input_report->console_info.wii_addr, 6);
                    if (len == 6) {
                        input_report->console_info.wii_addr_saved = 1;
                        printf("Auto-connect: loaded last Wii address from flash\n");
                    }
                }
            }
            break;
        case HCI_EVENT_COMMAND_COMPLETE:
            // Check IAC LAP result
            if (hci_event_command_complete_get_command_opcode(packet) == hci_write_current_iac_lap_one_iac.opcode) {
                uint8_t status = hci_event_command_complete_get_return_parameters(packet)[0];
                printf("Set IAC LAP: %s\n", (status != ERROR_CODE_SUCCESS) ? "Failed" : "OK");
                memcpy(wii_baddr, (const uint8_t *)input_report->console_info.wii_addr, 6);
                // Only initiate connection if we have a saved Wii address (non-zero). Otherwise stay discoverable and let Wii/PC connect to us.
                int addr_valid = (wii_baddr[0] | wii_baddr[1] | wii_baddr[2] | wii_baddr[3] | wii_baddr[4] | wii_baddr[5]) != 0;
                if (addr_valid) {
                    hid_device_connect(wii_baddr, &hid_cid);
                } else {
                    printf("No Wii addr saved; discoverable as Nintendo RVL-CNT-01 (press Sync on Wii or pair from PC)\n");
                }
            }
            break;
        case HCI_EVENT_PIN_CODE_REQUEST:
                    // inform about pin code request
                    uint8_t pin_code[6];
                    hci_event_pin_code_request_get_bd_addr(packet, wii_baddr);
                    reverse_bd_addr(wii_baddr, pin_code);
                    //printf_hexdump(pin_code, sizeof(pin_code));
                    gap_pin_code_response_binary(wii_baddr, pin_code, sizeof(pin_code));
                    break;
        case L2CAP_EVENT_CHANNEL_OPENED: {
            uint16_t cid = l2cap_event_channel_opened_get_local_cid(packet);
            uint8_t status = l2cap_event_channel_opened_get_status(packet);

            if (status == ERROR_CODE_SUCCESS) {
                printf("L2CAP channel opened (cid %d)\n", (int)cid);
            } else {
                    if (status == L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_SECURITY) {
                    // Assume remote has forgotten link key, delete it and try again
                    printf("Dropping link key due to L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_SECURITY\n");
                    bd_addr_t addr;
                    l2cap_event_channel_opened_get_address(packet, addr);
                    gap_drop_link_key_for_bd_addr(addr);
                    gap_disconnect(l2cap_event_channel_opened_get_handle(packet));
                } else {
                    printf("L2CAP connection failed (cid %d, status 0x%02x)\n", (int)cid, (int)status);
                }
            }
        }
        break;
        case HCI_EVENT_HID_META:
                switch (hci_event_hid_meta_get_subevent_code(packet)){
                    case HID_SUBEVENT_CONNECTION_OPENED:
                        status = hid_subevent_connection_opened_get_status(packet);
                        if (status != ERROR_CODE_SUCCESS) {
                            // outgoing connection failed
                            printf("Connection failed, status 0x%x\n", status);
                            hid_cid = 0;
                            // Authentication Failure -> Need to Sync again with the console
                            if(status == 0x66){
                                input_report->console_info.wii_addr_saved = 0;
                                if (btstack_tlv_singleton_impl)
                                    btstack_tlv_singleton_impl->delete_tag(btstack_tlv_singleton_context, TLV_TAG_HIDR);
                                printf("Fail to connect -> Need to Sync again\n");
                            }
                            return;
                        }
                        hid_cid = hid_subevent_connection_opened_get_hid_cid(packet);
                        /* Always get remote address from event (for incoming connection we didn't have it yet) */
                        hid_subevent_connection_opened_get_bd_addr(packet, wii_baddr);
                        printf("Connected to ");
                        printf_hexdump(wii_baddr, sizeof(wii_baddr));
                        /* Update console_info and TLV so next boot we auto-connect to this Wii */
                        memcpy((uint8_t *)input_report->console_info.wii_addr, wii_baddr, 6);
                        input_report->console_info.wii_addr_saved = 1;
                        flash_safe_execute(save_wii_addr, wii_baddr, 100);
                        if (btstack_tlv_singleton_impl)
                            btstack_tlv_singleton_impl->store_tag(btstack_tlv_singleton_context, TLV_TAG_HIDR, (const uint8_t *) &wii_baddr, sizeof(bd_addr_t));
                        printf("Wii addr saved for auto-connect\n");
                        // Remove timer led
                        btstack_run_loop_remove_timer(&led_state);
                        // Set the led on
                        (*callback_led_func[1])();
                        // Run wiimote process
                        btstack_run_loop_set_timer(&loop_wii, 1);
                        btstack_run_loop_add_timer(&loop_wii);
                        break;
                    case HID_SUBEVENT_CONNECTION_CLOSED:
                        printf("HID Disconnected\n");
                        hid_cid = 0;
                        // Stop wiimote process
                        btstack_run_loop_remove_timer(&loop_wii);
                        // Start led timer
                        btstack_run_loop_set_timer(&led_state, 100);
                        btstack_run_loop_add_timer(&led_state);
                        hid_device_connect(wii_baddr, &hid_cid);
                        break;
                    case HID_SUBEVENT_CAN_SEND_NOW:
                        send_data();
                        break;
                    default:
                        break;
                }
            break;
        default:
            break;
    }

    // Write IAC LAP when ready
    if (set_iac_lap && hci_can_send_command_packet_now()) {
        set_iac_lap = 0;
        // Limited Inquiry Access Code
        hci_send_cmd(&hci_write_current_iac_lap_one_iac, 1, GAP_IAC_LIMITED_INQUIRY);

        // Set device discoverable now, to emit a write scan enable after IAC LAP is set
        gap_discoverable_control(1);

        // It's weird but this fix the lag on sticks 
        gap_set_default_link_policy_settings(LM_LINK_POLICY_ENABLE_SNIFF_MODE);

        // HID Device
        hid_device_init(1, sizeof(wiimote_report_descriptor),wiimote_report_descriptor);

        // HID Device report callback 
        hid_device_register_report_data_callback(&get_data_wii);
    }
}


static void l2cap_sdp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    // printf("sdp_packet_handler %x\n", packet_type);

    switch (packet_type) {
        case L2CAP_DATA_PACKET: {

            uint16_t transaction_id = big_endian_read_16(packet, 1);

            //printf("SDP Request: type %u, transaction id %u, len %u, mtu %u\n", pdu_id, transaction_id, param_len, remote_mtu);

            //use the transaction id to determine the response to send
            state = transaction_id;

            if (state >= 0){
                switch (state) {
                    case 0:
                        sdp_server_response_size = sizeof(resp0);
                        memcpy(sdp_response_buffer, resp0, sizeof(resp0));
                        break;               
                    case 1:
                        sdp_server_response_size = sizeof(resp1);
                        memcpy(sdp_response_buffer, resp1, sizeof(resp1));
                        break;
                    case 2:
                        sdp_server_response_size = sizeof(resp2);
                        memcpy(sdp_response_buffer, resp2, sizeof(resp2));
                        break;
                    case 3:
                        sdp_server_response_size = sizeof(resp3);
                        memcpy(sdp_response_buffer, resp3, sizeof(resp3));
                        break;
                    case 4:
                        sdp_server_response_size = sizeof(resp4);
                        memcpy(sdp_response_buffer, resp4, sizeof(resp4));
                        break;
                    default:
                        break;
                }
            }

            if (!sdp_server_response_size) {
                break;
            }

            l2cap_request_can_send_now_event(sdp_server_l2cap_cid);
            break;
        }

        case HCI_EVENT_PACKET:
            switch (hci_event_packet_get_type(packet)) {
                case L2CAP_EVENT_INCOMING_CONNECTION:
                    if (sdp_server_l2cap_cid) {
                        // Just reject other incoming connections
                        l2cap_decline_connection(channel);
                        break;
                    }

                    // Accept connection
                    sdp_server_l2cap_cid = channel;
                    sdp_server_response_size = 0;
                    l2cap_accept_connection(sdp_server_l2cap_cid);
                    break;
                case L2CAP_EVENT_CHANNEL_OPENED:
                    // Reset if open failed
                    if (packet[2]) {
                        wiimote_emulator_reset();
                    }
                    break;
                case L2CAP_EVENT_CAN_SEND_NOW:
                    sdp_respond();
                    break;
                case L2CAP_EVENT_CHANNEL_CLOSED:
                    // Reset if channel closed
                    if (channel == sdp_server_l2cap_cid) {
                        wiimote_emulator_reset();
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}


static void led_handler(struct btstack_timer_source *ts)
{
    // Invert the led
    static uint8_t led_on = 0;
    led_on = !led_on;
    (*callback_led_func[led_on])();

    // Restart timer
    btstack_run_loop_set_timer(ts, 100);
    btstack_run_loop_add_timer(ts);
}


void wiimote_emulator(void *report){

    printf("Init Wiimote Emulator\n");

    // Data from gamepad
    input_report = report;

    // Disable SSP
    gap_ssp_set_enable(0);

    // Set device connectable
    gap_connectable_control(1);

    // Set device non discoverable for now, we'll set this after setting IAC LAP
    gap_discoverable_control(0);

    // Set device bondable
    gap_set_bondable_mode(1);

    // Set local name
    gap_set_local_name("Nintendo RVL-CNT-01");

    // Set class
    gap_set_class_of_device(0x002504);

    // Register HCI callback to set IAC LAP once HCI is working
    // and to catch the connection handle
    hci_event_callback_registration.callback = &hci_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // init L2CAP
    l2cap_init();

    // Register SDP service with max possible MTU
    l2cap_register_service(l2cap_sdp_packet_handler, BLUETOOTH_PSM_SDP, 0xffff, LEVEL_0);

    // SDP Server
    sdp_init();
    memset(hid_service_buffer, 0, sizeof(hid_service_buffer));

    hid_sdp_record_t hid_sdp_record = {
        // hid sevice subclass 2504 Gamepad?, hid counntry code 33 US
        0x2504,
        33,
        1,
        1,
        1,
        0,
        0,
        0xFFFF,
        0xFFFF,
        3200,
        wiimote_report_descriptor,
        sizeof(wiimote_report_descriptor),
        hid_device_name
    };


    hid_create_sdp_record(hid_service_buffer, sdp_create_service_record_handle(), &hid_sdp_record);
    btstack_assert(de_get_len( hid_service_buffer) <= sizeof(hid_service_buffer));
    sdp_register_service(hid_service_buffer);

    // See https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers if you don't have a USB Vendor ID and need a Bluetooth Vendor ID
    // device info: BlueKitchen GmbH, product 1, version 1
    device_id_create_sdp_record(pnp_service_buffer, sdp_create_service_record_handle(), DEVICE_ID_VENDOR_ID_SOURCE_BLUETOOTH, BLUETOOTH_COMPANY_ID_BLUEKITCHEN_GMBH, 1, 1);
    btstack_assert(de_get_len(pnp_service_buffer) <= sizeof(pnp_service_buffer));
    sdp_register_service(pnp_service_buffer);

     // register for HID events
    hid_device_register_packet_handler(&hci_packet_handler);

    // Power on!
    hci_power_control(HCI_POWER_ON);

    // Init wiimote structure
    wiimote_init(&wiimote);
    
    //No extension for default
    reset_input_ir(wiimote.usr.ir_object);
    wiimote.usr.connected_extension_type = NoExtension;

    // Loop control to send report
    loop_wii.process = &task_wiimote;

    btstack_run_loop_execute();
}


static void input_update_wiimote(){

    if(input_report->reset_ir){
        reset_input_classic(&wiimote.usr.classic);
        reset_input_nunchuk(&wiimote.usr.nunchuk);
        reset_input_ir(wiimote.usr.ir_object);
        // Change the extension too
        wiimote.usr.connected_extension_type = input_report->mode;
        input_report->reset_ir = 0;
    }

    switch(input_report->mode){
        case NO_EXTENSION:{
            float pointer_delta_x = 0, pointer_delta_y = 0;

            memcpy(&wiimote.usr, &input_report->wiimote, 14);

            pointer_delta_x += (input_report->wiimote.ir_x / 127.0) * 0.008;
            pointer_delta_y += (input_report->wiimote.ir_y / 127.0) * 0.008;

            pointer_x = fmax(-pointer_margin, fmin(1.0 + pointer_margin, pointer_x + pointer_delta_x));
            pointer_y = fmax(-pointer_margin, fmin(1.0 + pointer_margin, pointer_y + pointer_delta_y));

            set_motion_state(&wiimote, pointer_x, pointer_y);
        }
            break;
        case WIIMOTE_AND_NUNCHUCK:{
            // Wiimote
            float pointer_delta_x = 0, pointer_delta_y = 0;
            memcpy(&wiimote.usr, &input_report->wiimote, 14);

            // Nunchuck
            memcpy(&wiimote.usr.nunchuk, &input_report->nunchuk, sizeof(struct wiimote_nunchuk));

            pointer_delta_x += (input_report->wiimote.ir_x / 127.0) * 0.008;
            pointer_delta_y += (input_report->wiimote.ir_y / 127.0) * 0.008;

            pointer_x = fmax(-pointer_margin, fmin(1.0 + pointer_margin, pointer_x + pointer_delta_x));
            pointer_y = fmax(-pointer_margin, fmin(1.0 + pointer_margin, pointer_y + pointer_delta_y));

            set_motion_state(&wiimote, pointer_x, pointer_y);

            if(input_report->fake_motion || input_report->center_accel){

                if(input_report->fake_motion) {
                    static int step = 48;

                    if(input_report->wiimote.accel_x >= 1000 || input_report->wiimote.accel_x <= 100){
                        step = -step;
                    }

                    // Start moving all directions

                    int temp_value = step + input_report->wiimote.accel_x;

                    input_report->wiimote.accel_x = (uint16_t)temp_value;
                    input_report->nunchuk.accel_x = (uint16_t)temp_value;

                    input_report->wiimote.accel_y = (uint16_t)temp_value;
                    input_report->nunchuk.accel_y = (uint16_t)temp_value;

                    input_report->wiimote.accel_z = (uint16_t)temp_value;
                    input_report->nunchuk.accel_z = (uint16_t)temp_value;
                }

                wiimote.usr.accel_x = input_report->wiimote.accel_x;
                wiimote.usr.accel_y = input_report->wiimote.accel_y;
                wiimote.usr.accel_z = input_report->wiimote.accel_z;

                if(input_report->center_accel){
                    input_report->center_accel = 0;
                }
            }
        }
            break;
        case CLASSIC_CONTROLLER:
            memcpy(&wiimote.usr.classic, &input_report->classic, sizeof(struct wiimote_classic));
            break;
        default:
            break;
    }
    
}


void wiimote_emulator_set_led(void (*led_on)(), void (*led_off)()){
    callback_led_func[0] = led_off;
    callback_led_func[1] = led_on;

    led_state.process = &led_handler;
    btstack_run_loop_set_timer(&led_state, 100);
    btstack_run_loop_add_timer(&led_state); 
}

