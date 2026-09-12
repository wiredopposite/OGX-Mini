#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "inet.h"
#include "wiimote.h"
#include "wm_reports.h"
#include "eeprom.bin.h"

static uint8_t *eeprom_bin;
int tries = 0;

static uint8_t classic_calibration[16] =
{
  // 0xF8, 0x04, 0x7A, 0xF8, 0x04, 0x7A, 0xF8, 0x04, 0x7A, 0xF8, 0x04, 0x7A, 0x00, 0x00, 0x00, 0x00
  0xE1, 0x19, 0x7C, 0xEF, 0x22, 0x7C, 0xE6, 0x1E, 0x85, 0xDE, 0x15, 0x8B, 0x0E, 0x22, 0x8F, 0xE4
};

static uint8_t nunchuk_calibration[16] =
{
  0x81, 0x80, 0x7F, 0x22, 0xB5, 0xB3, 0xB3, 0x03, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x83, 0x14, 0x69
};

int process_report(struct wiimote_state *state, const uint8_t * buf, int len)
{
  struct report_data * data = (struct report_data *)buf;

  //every output report contains rumble info
  state->sys.rumble = data->buf[0] & 0x01;

  switch (data->type)
  {
    case 0x11: //player LEDs
    {
      struct report_leds * rpt = (struct report_leds *)data->buf;

      state->sys.led_1 = rpt->led_1;
      state->sys.led_2 = rpt->led_2;
      state->sys.led_3 = rpt->led_3;
      state->sys.led_4 = rpt->led_4;

      report_queue_push_ack(state, data->type, 0x00);
      break;
    }
    case 0x12: //data reporting mode
    {
      struct report_mode * rpt = (struct report_mode *)data->buf;

      state->sys.reporting_continuous = rpt->continuous;
      state->sys.reporting_mode = rpt->mode;

      report_queue_push_ack(state, data->type, 0x00);
      break;
    }
    case 0x13:
    case 0x1a: //ir camera enable
    {
      struct report_ir_enable * rpt = (struct report_ir_enable *)data->buf;

      state->sys.ircam_enabled = rpt->enabled;

      report_queue_push_ack(state, data->type, 0x00);
      break;
    }
    case 0x14:
    case 0x19: //speaker enable
    {
      struct report_speaker_enable * rpt = (struct report_speaker_enable *)data->buf;

      state->sys.speaker_enabled = !rpt->muted;

      report_queue_push_ack(state, data->type, 0x00);
      break;
    }
    case 0x15: //status information request
      //send a status report 0x20
      report_queue_push_status(state);
      break;
    case 0x16: //write memory
    {
      struct report_mem_write * rpt = (struct report_mem_write *)data->buf;

      if (rpt->source0 || rpt->source1)
      {
        write_register(state, ntohl(OFFSET24(rpt->offset)), rpt->size, rpt->data);
      }
      else
      {
        write_eeprom(state, ntohl(OFFSET24(rpt->offset)), rpt->size, rpt->data);
      }
      break;
    }
    case 0x17: //read memory
    {
      struct report_mem_read * rpt = (struct report_mem_read *)data->buf;

      if (rpt->source0 || rpt->source1)
      {
        read_register(state, ntohl(OFFSET24(rpt->offset)), ntohs(rpt->size));
      }
      else
      {
        read_eeprom(state, ntohl(OFFSET24(rpt->offset)), ntohs(rpt->size));
      }

      break;
    }
  }

  return 0;
}

int generate_report(struct wiimote_state * state, uint8_t * buf)
{
  int len;

  struct report_data * data = (struct report_data *)buf;
  uint8_t * contents;

  if (state->usr.connected_extension_type != state->sys.connected_extension_type)
  {
    if (state->sys.extension_connected)
    {
      state->sys.extension_connected = 0;
      state->sys.connected_extension_type = NoExtension;
      state->sys.extension_hotplug_timer = 30;
      report_queue_push_status(state);
    }

    bool extension_connected = (state->usr.connected_extension_type != NoExtension);
    if (extension_connected && --state->sys.extension_hotplug_timer <= 0)
    {
      state->sys.extension_connected = extension_connected;
      state->sys.connected_extension_type = state->usr.connected_extension_type;
      report_queue_push_status(state);
      init_extension(state);
    }
  }

  if (!state->sys.reporting_continuous && !state->sys.report_changed && state->sys.queue == NULL)
    return 0;

  if (state->sys.queue == NULL)
  {
    //regular report
    memset(data, 0, sizeof(struct report_data));
    len = 2;
    data->io = 0xa1;
    data->type = state->sys.reporting_mode;
  }
  else
  {
    //queued report (acknowledgement, response, etc)
    struct report * rpt;
    rpt = report_queue_peek(state);
    len = rpt->len;
    memcpy(data, &rpt->data, sizeof(struct report_data));
    report_queue_pop(state);
  }

  contents = data->buf;

  //fill report
  switch (data->type)
  {
    case 0x30: // core buttons
      report_append_buttons(state, contents);
      len += 2;
      break;
    case 0x31: // core buttons + accelerometer
      report_append_buttons(state, contents);
      report_append_accelerometer(state, contents);
      len += 2 + 3;
      break;
    case 0x32: // core buttons + 8 extension bytes
      report_append_buttons(state, contents);
      len += 2 + 8;
      break;
    case 0x33: // core buttons + accelerometer + 12 ir bytes
      report_append_buttons(state, contents);
      report_append_accelerometer(state, contents);
      report_append_ir_12(state, contents + 5);
      len += 2 + 3 + 12;
      break;
    case 0x34: // core buttons + 19 extension bytes
      report_append_buttons(state, contents);
      report_append_extension(state, contents + 2, 19);
      len += 2 + 19;
      break;
    case 0x35: // core buttons + accelerometer + 16 extension bytes
      report_append_buttons(state, contents);
      report_append_accelerometer(state, contents);
      report_append_extension(state, contents + 5, 16);
      len += 2 + 3 + 16;
      break;
    case 0x36: // core buttons + 10 ir bytes + 9 extension bytes
      report_append_buttons(state, contents);
      report_append_ir_10(state, contents + 2);
      report_append_extension(state, contents + 12, 9);
      len += 2 + 10 + 9;
      break;
    case 0x37: // core buttons + accelerometer + 10 ir bytes + 6 extension bytes
      report_append_buttons(state, contents);
      report_append_accelerometer(state, contents);
      report_append_ir_10(state, contents + 5);
      report_append_extension(state, contents + 15, 6);
      len += 2 + 3 + 10 + 6;
      break;
    case 0x3d: // 21 extension bytes
      report_append_extension(state, contents, 21);
      len += 21;
      break;
    case 0x3e: // interleaved core buttons + accelerometer with 36 ir bytes pt I
    case 0x3f: // interleaved core buttons + accelerometer with 36 ir bytes pt II
      report_append_buttons(state, contents);
      report_append_interleaved(state, contents);
      len += 21;
      break;
    default: // special output report (acknowledgement, status, or memory read)
      report_append_buttons(state, contents);
      break;
  }

  return len;
}

void read_eeprom(struct wiimote_state * state, uint32_t offset, uint16_t size)
{
  uint8_t *file;
  uint8_t *buffer;
  struct report * rpt;
  int i;

  offset = offset & 0xFFFF;

  //addresses greater than 0x16FF cannot be read or written
  if (offset + size > 0x16FF)
  {
    rpt = report_queue_push(state);
    report_format_mem_resp(state, rpt, 0x10, 0x8, offset, NULL, false);
    return;
  }

  //fseek
  file = &eeprom_bin[0 + offset];

  buffer = (uint8_t *)malloc(size);
  if (!buffer)
  {
    return;
  }

  //fread
  memcpy(buffer, file, size);

  //equivalent to ceil(size / 0x10)
  int total_packets = (size + 0x10 - 1) / 0x10;

  //allocate all the needed reports
  for (i = 0; i < total_packets; i++)
  {
    report_queue_push(state);
  }

  //copy packet data
  struct queued_report * queue_item = state->sys.queue;

  for (i = 0; i < total_packets; i++)
  {
    rpt = &queue_item->rpt;
    int packet_size = (i == total_packets - 1) ? (size - i * 0x10) : 0x10;
    report_format_mem_resp(state, rpt, packet_size, 0x0, offset + i*0x10, &buffer[i*0x10], false);
    queue_item = queue_item->next;
  }

  free(buffer);
}

void write_eeprom(struct wiimote_state * state, uint32_t offset, uint8_t size, const uint8_t * buf)
{
  uint8_t *file;
  struct report * rpt;

  offset = offset & 0xFFFF;

  //addresses greater than 0x16FF cannot be read or written
  if (offset + size > 0x16FF)
  {
    rpt = report_queue_push(state);
    report_format_mem_resp(state, rpt, 0x10, 0x8, offset, NULL, false);
    return;
  }

  //fseek
  file = &eeprom_bin[0 + offset];

  //fwrite
  memcpy(file, buf, size);
  report_queue_push_ack(state, 0x16, 0x00);
}

void read_register(struct wiimote_state *state, uint32_t offset, uint16_t size)
{
  uint8_t * buffer;
  struct report * rpt;
  int i;
  bool encrypt = false;

  switch ((offset >> 16) & 0xfe) //select register, ignore lsb
  {
    case 0xa2: //speaker
      buffer = state->sys.register_a2 + (offset & 0xff);
      break;
    case 0xa4: //extension
      if (state->sys.wmp_state == 1)
      {
        //i guess this isn't needed after all
        //^^this is an old comment, so is this needed or not?
        if (((offset & 0xff) == 0xf6) || ((offset & 0xff) == 0xf7))
        {
          tries += 1;
          printf("%d \n", tries);
          if (tries == 5)
          {
            state->sys.register_a6[0xf7] = 0x0e;
          }
        }

        buffer = state->sys.register_a6 + (offset & 0xff);
      }
      else
      {
        buffer = state->sys.register_a4 + (offset & 0xff);
      }

      if (state->sys.extension_encrypted)
      {
        encrypt = true;
      }

      break;
    case 0xa6: //motionplus
      if (state->sys.wmp_state == 1)
      {
         rpt = report_queue_push(state);
         report_format_mem_resp(state, rpt, 0x10, 0x7, offset, NULL, false);
         return;
      }
      buffer = state->sys.register_a6 + (offset & 0xff);
      break;
    case 0xb0: //ir camera
      buffer = state->sys.register_b0 + (offset & 0xff);
      break;
    default: //???
      break;
  }

  //equivalent to ceil(size / 0x10)
  int total_packets = (size + 0x10 - 1) / 0x10;

  //allocate all the needed reports
  for (i = 0; i < total_packets; i++)
  {
    report_queue_push(state);
  }

  struct queued_report * queue_item = state->sys.queue;

  for (i = 0; i < total_packets; i++)
  {
    rpt = &queue_item->rpt;
    int packet_size = (i == total_packets - 1) ? (size - i * 0x10) : 0x10;
    report_format_mem_resp(state, rpt, packet_size, 0x0, offset + i*0x10, &buffer[i*0x10], encrypt);
    queue_item = queue_item->next;
  }
}

void write_register(struct wiimote_state *state, uint32_t offset, uint8_t size, const uint8_t * buf)
{
  uint8_t * reg;
  int result = 0x00;

  switch ((offset >> 16) & 0xfe) //select register, ignore lsb
  {
    case 0xa2: //speaker
      reg = state->sys.register_a2;
      memcpy(reg + (offset & 0xff), buf, size);
      break;
    case 0xa4: //extension
      reg = (state->sys.wmp_state == 1) ? state->sys.register_a6 : state->sys.register_a4;

      memcpy(reg + (offset & 0xff), buf, size);

      //TODO: double check what this does, the buf location it's looking for
      if (((offset & 0xff) == 0xf0) && (buf[0] == 0x55) && (state->sys.wmp_state == 1)) //deactivate wmp
      {
        state->sys.wmp_state = 3;

        init_extension(state);

        report_queue_push_ack(state, 0x16, 0x00);
        state->sys.extension_connected = 0;
        report_queue_push_status(state);
        state->sys.extension_connected = 1;
        report_queue_push_status(state);
        return;
      }
      else if (((offset & 0xff) == 0xfe) && (buf[0] == 0x00) && (state->sys.wmp_state == 1)) //also deactivate wmp?
      {
        state->sys.wmp_state = 0;

        init_extension(state);

        report_queue_push_ack(state, 0x16, 0x00);
        state->sys.extension_connected = 0;
        report_queue_push_status(state);
        state->sys.extension_connected = 1;
        report_queue_push_status(state);
        return;
      }
      else if ((offset & 0xff) == 0x4c) //last part of encryption code
      {
        ext_generate_tables(&state->sys.extension_crypto_state, &reg[0x40]);
        state->sys.extension_encrypted = 1;
      }
      else if ((offset & 0xff) == 0xf0)
      {
        if (buf[0] == 0xaa)
        {
          state->sys.extension_encrypted = 1;
        }
        else if (buf[0] == 0x55)
        {
          state->sys.extension_encrypted = 0;
        }
      }
      else if ((offset & 0xff) == 0xf1)
      {
        state->sys.register_a6[0xf7] = 0x1a;

        //idk why or how, but sometimes this must be updated
        state->sys.register_a6[0x50] = 0xe7;
        state->sys.register_a6[0x51] = 0x98;
        state->sys.register_a6[0x52] = 0x31;
        state->sys.register_a6[0x53] = 0x8a;
        state->sys.register_a6[0x54] = 0x18;
        state->sys.register_a6[0x55] = 0x82;
        state->sys.register_a6[0x56] = 0x37;
        state->sys.register_a6[0x57] = 0x5e;
        state->sys.register_a6[0x58] = 0x02;
        state->sys.register_a6[0x59] = 0x4f;
        state->sys.register_a6[0x5a] = 0x68;
        state->sys.register_a6[0x5b] = 0x47;
        state->sys.register_a6[0x5c] = 0x78;
        state->sys.register_a6[0x5d] = 0xef;
        state->sys.register_a6[0x5e] = 0xbb;
        state->sys.register_a6[0x5f] = 0xd7;

        state->sys.register_a6[0x60] = 0x86;
        state->sys.register_a6[0x61] = 0xc8;
        state->sys.register_a6[0x62] = 0x95;
        state->sys.register_a6[0x63] = 0xbd;
        state->sys.register_a6[0x64] = 0x20;
        state->sys.register_a6[0x65] = 0x9b;
        state->sys.register_a6[0x66] = 0xeb;
        state->sys.register_a6[0x67] = 0x8b;
        state->sys.register_a6[0x68] = 0x79;
        state->sys.register_a6[0x69] = 0x81;
        state->sys.register_a6[0x6a] = 0xdc;
        state->sys.register_a6[0x6b] = 0x61;
        state->sys.register_a6[0x6c] = 0x13;
        state->sys.register_a6[0x6d] = 0x54;
        state->sys.register_a6[0x6e] = 0x79;
        state->sys.register_a6[0x6f] = 0x4c;

        state->sys.register_a6[0x70] = 0xb7;
        state->sys.register_a6[0x71] = 0x26;
        state->sys.register_a6[0x72] = 0x82;
        state->sys.register_a6[0x73] = 0x17;
        state->sys.register_a6[0x74] = 0xe8;
        state->sys.register_a6[0x75] = 0x0f;
        state->sys.register_a6[0x76] = 0xa9;
        state->sys.register_a6[0x77] = 0xb5;
        state->sys.register_a6[0x78] = 0x45;
        state->sys.register_a6[0x79] = 0xa0;
        state->sys.register_a6[0x7a] = 0x38;
        state->sys.register_a6[0x7b] = 0x8e;
        state->sys.register_a6[0x7c] = 0x9e;
        state->sys.register_a6[0x7d] = 0x86;
        state->sys.register_a6[0x7e] = 0x72;
        state->sys.register_a6[0x7f] = 0x55;

        state->sys.register_a6[0x80] = 0x3d;
        state->sys.register_a6[0x81] = 0x46;
        state->sys.register_a6[0x82] = 0x2e;
        state->sys.register_a6[0x83] = 0x3e;
        state->sys.register_a6[0x84] = 0x10;
        state->sys.register_a6[0x85] = 0x1f;
        state->sys.register_a6[0x86] = 0x8e;
        state->sys.register_a6[0x87] = 0x0c;
        state->sys.register_a6[0x88] = 0xf4;
        state->sys.register_a6[0x89] = 0x04;
        state->sys.register_a6[0x8a] = 0x89;
        state->sys.register_a6[0x8b] = 0x4c;
        state->sys.register_a6[0x8c] = 0xca;
        state->sys.register_a6[0x8d] = 0x3e;
        state->sys.register_a6[0x8e] = 0x9f;
        state->sys.register_a6[0x8f] = 0x36;
      }

      break;
    case 0xa6: //motionplus
      reg = state->sys.register_a6;
      memcpy(reg + (offset & 0xff), buf, size);

      if (((offset & 0xff) == 0xfe) && ((buf[0] >> 2) & 0x1)) //activate wmp
      {
        state->sys.wmp_state = 1;
        state->sys.extension_report_type = (buf[0] & 0x7);
        printf("activate wmp\n");

        init_extension(state);

        report_queue_push_ack(state, 0x16, 0x00);
        state->sys.extension_connected = 0;
        report_queue_push_status(state);
        state->sys.extension_connected = 1;
        report_queue_push_status(state);
        return;
      }

      break;
    case 0xb0: //ir camera
      reg = state->sys.register_b0;
      memcpy(reg + (offset & 0xff), buf, size);
      break;
    default: //???
      break;
  }

  report_queue_push_ack(state, 0x16, result);
}

void reset_ir_object(struct wiimote_ir_object * ir_object)
{
  memset(ir_object, 0xff, sizeof(struct wiimote_ir_object));
}

void reset_input_ir(struct wiimote_ir_object ir_object[4])
{
  memset(ir_object, 0xff, sizeof(struct wiimote_ir_object) * 4);
}

void reset_input_nunchuk(struct wiimote_nunchuk * nunchuk)
{
  memset(nunchuk, 0, sizeof(struct wiimote_nunchuk));

  nunchuk->x = 128;
  nunchuk->y = 128;
  nunchuk->accel_x = 512;
  nunchuk->accel_y = 512;
  nunchuk->accel_z = 760;
}

void reset_input_classic(struct wiimote_classic * classic)
{
  memset(classic, 0, sizeof(struct wiimote_classic));

  classic->ls_x = 32;
  classic->ls_y = 32;
  classic->rs_x = 15;
  classic->rs_y = 15;
}

void reset_input_motionplus(struct wiimote_motionplus * motionplus)
{
  memset(motionplus, 0, sizeof(struct wiimote_motionplus));

  motionplus->yaw_down = 0x1F7F;
  motionplus->roll_left = 0x1F7F;
  motionplus->pitch_left = 0x1F7F;
  motionplus->yaw_slow = 1;
  motionplus->roll_slow = 1;
  motionplus->pitch_slow = 1;
}

void init_extension(struct wiimote_state * state)
{
  if (state->sys.connected_extension_type == NoExtension)
  {
    memset(state->sys.register_a4, 0xff, sizeof(state->sys.register_a4));
  }
  else
  {
    memset(state->sys.register_a4, 0, sizeof(state->sys.register_a4));
  }

  if (state->sys.wmp_state == 1)
  {
    //register_a6[0xfa] = 0x00;
    //register_a6[0xfb] = 0x00;
    //register_a6[0xfc] = 0xa4;
    //register_a6[0xfd] = 0x20;
    //this might not be needed now that memory is proper
    //register_a6[0xfe] = state->sys.extension_report_type;
    //register_a6[0xff] = 0x05;
    state->sys.register_a6[0xfc] = 0xa4;

    state->sys.extension_encrypted = 0;

    //random guess, pulled from wiimote, not sure what this is for
    state->sys.register_a6[0xf0] = 0x55;
    state->sys.register_a6[0xf1] = 0xff;
    state->sys.register_a6[0xf2] = 0xff;
    state->sys.register_a6[0xf3] = 0xff;
    state->sys.register_a6[0xf4] = 0xff;
    state->sys.register_a6[0xf5] = 0xff;
    state->sys.register_a6[0xf6] = 0x00;

    //a4 40 post init
    // state->sys.register_a6[0x40] = 0x81;
    // state->sys.register_a6[0x41] = 0x80;
    // state->sys.register_a6[0x42] = 0x80;
    // state->sys.register_a6[0x43] = 0x28;
    // state->sys.register_a6[0x44] = 0xb4;
    // state->sys.register_a6[0x45] = 0xb3;
    // state->sys.register_a6[0x46] = 0xb3;
    // state->sys.register_a6[0x47] = 0x26;
    // state->sys.register_a6[0x48] = 0xe3;
    // state->sys.register_a6[0x49] = 0x22;
    // state->sys.register_a6[0x4a] = 0x7a;
    // state->sys.register_a6[0x4b] = 0xd8;
    // state->sys.register_a6[0x4c] = 0x1b;
    // state->sys.register_a6[0x4d] = 0x81;
    // state->sys.register_a6[0x4e] = 0x31;
    // state->sys.register_a6[0x4f] = 0x86;

    state->sys.register_a6[0x20] = 0x7c;
    state->sys.register_a6[0x21] = 0x97;
    state->sys.register_a6[0x22] = 0x7f;
    state->sys.register_a6[0x23] = 0x0a;
    state->sys.register_a6[0x24] = 0x7c;
    state->sys.register_a6[0x25] = 0xa8;
    state->sys.register_a6[0x26] = 0x33;
    state->sys.register_a6[0x27] = 0xb7;
    state->sys.register_a6[0x28] = 0xcc;
    state->sys.register_a6[0x29] = 0x12;
    state->sys.register_a6[0x2a] = 0x33;
    state->sys.register_a6[0x2b] = 0x08;
    state->sys.register_a6[0x2c] = 0xc8;
    state->sys.register_a6[0x2d] = 0x01;
    state->sys.register_a6[0x2e] = 0x72;
    state->sys.register_a6[0x2f] = 0xd4;

    state->sys.register_a6[0x30] = 0x7c;
    state->sys.register_a6[0x31] = 0x53;
    state->sys.register_a6[0x32] = 0x87;
    state->sys.register_a6[0x33] = 0x58;
    state->sys.register_a6[0x34] = 0x7c;
    state->sys.register_a6[0x35] = 0x9f;
    state->sys.register_a6[0x36] = 0x36;
    state->sys.register_a6[0x37] = 0xb2;
    state->sys.register_a6[0x38] = 0xc9;
    state->sys.register_a6[0x39] = 0x34;
    state->sys.register_a6[0x3a] = 0x35;
    state->sys.register_a6[0x3b] = 0xf8;
    state->sys.register_a6[0x3c] = 0x2d;
    state->sys.register_a6[0x3d] = 0x60;
    state->sys.register_a6[0x3e] = 0xd7;
    state->sys.register_a6[0x3f] = 0xd5;

    //not sure block, this may not be needed
    state->sys.register_a6[0x50] = 0x15;
    state->sys.register_a6[0x51] = 0x6d;
    state->sys.register_a6[0x52] = 0xe0;
    state->sys.register_a6[0x53] = 0x23;
    state->sys.register_a6[0x54] = 0x20;
    state->sys.register_a6[0x55] = 0x79;
    state->sys.register_a6[0x56] = 0xd3;
    state->sys.register_a6[0x57] = 0x73;
    state->sys.register_a6[0x58] = 0x01;
    state->sys.register_a6[0x59] = 0xa9;
    state->sys.register_a6[0x5a] = 0xf0;
    state->sys.register_a6[0x5b] = 0x25;
    state->sys.register_a6[0x5c] = 0xb0;
    state->sys.register_a6[0x5d] = 0xbc;
    state->sys.register_a6[0x5e] = 0xff;
    state->sys.register_a6[0x5f] = 0xe1;

    state->sys.register_a6[0x60] = 0xd8;
    state->sys.register_a6[0x61] = 0x3f;
    state->sys.register_a6[0x62] = 0x82;
    state->sys.register_a6[0x63] = 0x52;
    state->sys.register_a6[0x64] = 0x75;
    state->sys.register_a6[0x65] = 0x99;
    state->sys.register_a6[0x66] = 0xbe;
    state->sys.register_a6[0x67] = 0xdb;
    state->sys.register_a6[0x68] = 0xcb;
    state->sys.register_a6[0x69] = 0x61;
    state->sys.register_a6[0x6a] = 0x60;
    state->sys.register_a6[0x6b] = 0x0f;
    state->sys.register_a6[0x6c] = 0x35;
    state->sys.register_a6[0x6d] = 0xbd;
    state->sys.register_a6[0x6e] = 0xd4;
    state->sys.register_a6[0x6f] = 0x4d;

    state->sys.register_a6[0x70] = 0x5c;
    state->sys.register_a6[0x71] = 0x9f;
    state->sys.register_a6[0x72] = 0x5d;
    state->sys.register_a6[0x73] = 0x81;
    state->sys.register_a6[0x74] = 0x71;
    state->sys.register_a6[0x75] = 0xde;
    state->sys.register_a6[0x76] = 0x22;
    state->sys.register_a6[0x77] = 0xe6;
    state->sys.register_a6[0x78] = 0xb9;
    state->sys.register_a6[0x79] = 0x23;
    state->sys.register_a6[0x7a] = 0xa4;
    state->sys.register_a6[0x7b] = 0x58;
    state->sys.register_a6[0x7c] = 0xb7;
    state->sys.register_a6[0x7d] = 0x62;
    state->sys.register_a6[0x7e] = 0x33;
    state->sys.register_a6[0x7f] = 0xa4;

    state->sys.register_a6[0x80] = 0xcd;
    state->sys.register_a6[0x81] = 0x8b;
    state->sys.register_a6[0x82] = 0x3a;
    state->sys.register_a6[0x83] = 0xfe;
    state->sys.register_a6[0x84] = 0x98;
    state->sys.register_a6[0x85] = 0xf0;
    state->sys.register_a6[0x86] = 0xd9;
    state->sys.register_a6[0x87] = 0x57;
    state->sys.register_a6[0x88] = 0x0c;
    state->sys.register_a6[0x89] = 0xe8;
    state->sys.register_a6[0x8a] = 0x27;
    state->sys.register_a6[0x8b] = 0x51;
    state->sys.register_a6[0x8c] = 0xb6;
    state->sys.register_a6[0x8d] = 0xea;
    state->sys.register_a6[0x8e] = 0xe5;
    state->sys.register_a6[0x8f] = 0x78;


    //init progress byte, set it to done
    state->sys.register_a6[0xf7] = 0x0c;
    state->sys.register_a6[0xf8] = 0x00;
    state->sys.register_a6[0xf9] = 0x00;
  }
  else
  {
    //state->sys.register_a6[0xf7] = 0x0c;

    state->sys.register_a6[0xf0] = 0x55;
    state->sys.register_a6[0xf1] = 0xff;
    state->sys.register_a6[0xf2] = 0xff;
    state->sys.register_a6[0xf3] = 0xff;
    state->sys.register_a6[0xf4] = 0xff;
    state->sys.register_a6[0xf5] = 0xff;
    state->sys.register_a6[0xf6] = 0xff;
    state->sys.register_a6[0xf7] = 0x02;
    state->sys.register_a6[0xf8] = 0xff;
    state->sys.register_a6[0xf9] = 0xff;
    state->sys.register_a6[0xfa] = 0x01;
    state->sys.register_a6[0xfb] = 0x00;
    state->sys.register_a6[0xfc] = 0xa6;
    state->sys.register_a6[0xfd] = 0x20;
    state->sys.register_a6[0xfe] = 0x00;
    state->sys.register_a6[0xff] = 0x05;

    if (state->sys.connected_extension_type == NoExtension)
    {
      return;
    }

    memset(&state->sys.register_a4[0xf0], 0x0, 0x10);

    state->sys.register_a4[0xf0] = 0x55;
    state->sys.register_a4[0xfc] = 0xa4;
    state->sys.register_a4[0xfd] = 0x20;

    switch (state->sys.connected_extension_type)
    {
      default:
      case Nunchuk:
        state->sys.register_a4[0xfe] = 0x00;
        state->sys.register_a4[0xff] = 0x00;
        memcpy(&state->sys.register_a4[0x20], nunchuk_calibration, 0x10);
        memcpy(&state->sys.register_a4[0x30], nunchuk_calibration, 0x10);
        break;
      case Classic:
        state->sys.register_a4[0xfe] = 0x01;
        state->sys.register_a4[0xff] = 0x01;
        memcpy(&state->sys.register_a4[0x20], classic_calibration, 0x10);
        memcpy(&state->sys.register_a4[0x30], classic_calibration, 0x10);
        break;
      case BalanceBoard:
        state->sys.register_a4[0xfe] = 0x04;
        state->sys.register_a4[0xff] = 0x02;
        break;
    }

    state->sys.extension_report_type = state->sys.register_a4[0xfe];
    state->sys.extension_type = state->sys.register_a4[0xff];
  }
}

void wiimote_destroy(struct wiimote_state *state)
{
  while (state->sys.queue != NULL)
  {
    struct queued_report * rpt = state->sys.queue;
    state->sys.queue = rpt->next;
    free(rpt);
  }
}

void wiimote_init(struct wiimote_state *state)
{

  /*INIT EEPROM*/
  //eeprom allocate memory
  eeprom_bin = (uint8_t*)malloc(TOTAL_SIZE_EEPROM * sizeof(uint8_t));
  //Copy the first data
  memcpy(eeprom_bin, &eeprom_bin_1[0], sizeof(eeprom_bin_1));
  //Copy the last data
  memcpy(&eeprom_bin[LAST_DATA_BEGIN_POS], &eeprom_bin_2[0], sizeof(eeprom_bin_2));
  /*INIT EEPROM*/

  memset(state, 0, sizeof(struct wiimote_state));

  //flat
  state->usr.accel_x = 0x82 << 2;
  state->usr.accel_y = 0x82 << 2;
  state->usr.accel_z = 0x9f << 2;

  reset_input_ir(state->usr.ir_object);
  reset_input_nunchuk(&state->usr.nunchuk);
  reset_input_classic(&state->usr.classic);
  reset_input_motionplus(&state->usr.motionplus);

  state->usr.connected_extension_type = NoExtension;

  wiimote_reset(state);

  //power on report
  struct report * rpt = report_queue_push(state);
  rpt->len = 4;
  rpt->data.io = 0xa1;
  rpt->data.type = 0x30;
}

void wiimote_reset(struct wiimote_state *state)
{
  memset(&state->sys, 0, sizeof(struct wiimote_state_sys));

  state->sys.reporting_mode = 0x30;
  state->sys.battery_level = 0xff;

  state->sys.connected_extension_type = NoExtension;

  init_extension(state);
}
