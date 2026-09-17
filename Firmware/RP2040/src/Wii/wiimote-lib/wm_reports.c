#include "wm_reports.h"
#include "wm_crypto.h"
#include "inet.h"

#include <stdlib.h>
#include <string.h>

struct report * report_queue_push(struct wiimote_state * state)
{
  struct queued_report * rpt;

  //allocate new report
  rpt = (struct queued_report *)malloc(sizeof(struct queued_report));
  memset(rpt, 0, sizeof(struct queued_report));

  //append to the end of the queue
  if (state->sys.queue_end != NULL)
  {
    state->sys.queue_end->next = rpt;
  }
  else
  {
    state->sys.queue = rpt;
  }
  state->sys.queue_end = rpt;

  return &(rpt->rpt);
}

struct report * report_queue_peek(struct wiimote_state * state)
{
  if (state->sys.queue == NULL) return NULL; //empty queue

  return &(state->sys.queue->rpt);
}

void report_queue_pop(struct wiimote_state * state)
{
  struct queued_report * rpt;

  if (state->sys.queue == NULL) return; //nothing to remove

  //remove from the queue
  rpt = state->sys.queue;
  if (state->sys.queue_end == rpt)
  {
    state->sys.queue_end = NULL;
  }
  state->sys.queue = rpt->next;

  //free report mem
  free(rpt);
}

void report_queue_push_ack(struct wiimote_state *state, uint8_t report, uint8_t result)
{
  //push acknowledgement report x22
  struct report * rpt = report_queue_push(state);
  rpt->len = 6;
  rpt->data.io = 0xa1;
  rpt->data.type = 0x22;

  struct report_ack * ack = (struct report_ack *)rpt->data.buf;
  ack->report = report;
  ack->result = result;
}

void report_queue_push_status(struct wiimote_state * state)
{
  //push status report x20
  struct report * rpt = report_queue_push(state);
  rpt->len = 8;
  rpt->data.io = 0xa1;
  rpt->data.type = 0x20;

  struct report_status * status = (struct report_status *)rpt->data.buf;
  status->low_battery         = state->sys.low_battery;
  status->extension_connected = state->sys.extension_connected;
  status->speaker_enabled     = state->sys.speaker_enabled;
  status->ircam_enabled       = state->sys.ircam_enabled;
  status->led_1               = state->sys.led_1;
  status->led_2               = state->sys.led_2;
  status->led_3               = state->sys.led_3;
  status->led_4               = state->sys.led_4;
  status->battery_level       = state->sys.battery_level;
}

void report_format_mem_resp(struct wiimote_state * state, struct report * rpt,
  int size, int error, uint16_t addr, uint8_t * buf, bool encrypt)
{
  struct report_mem_resp * resp = (struct report_mem_resp *)rpt->data.buf;

  rpt->len = 23;
  rpt->data.io = 0xa1;
  rpt->data.type = 0x21;

  resp->size = size - 1;
  resp->error = error;
  resp->addr = htons(addr);
  if (buf != NULL) //buf will be null for error reports
  {
    memcpy(resp->data, buf, size);

    if (encrypt)
    {
      ext_encrypt_bytes(&state->sys.extension_crypto_state, resp->data, addr & 0x7, size);
    }
  }
}

void report_append_buttons(struct wiimote_state * state, uint8_t * buf)
{
  struct report_buttons * rpt = (struct report_buttons *)buf;

  rpt->left  = state->usr.left;
  rpt->right = state->usr.right;
  rpt->down  = state->usr.down;
  rpt->up    = state->usr.up;
  rpt->one   = state->usr.one;
  rpt->two   = state->usr.two;
  rpt->a     = state->usr.a;
  rpt->b     = state->usr.b;
  rpt->plus  = state->usr.plus;
  rpt->minus = state->usr.minus;
  rpt->home  = state->usr.home;
}

void report_append_accelerometer(struct wiimote_state * state, uint8_t * buf)
{
  struct report_accelerometer * rpt = (struct report_accelerometer *)buf;

  rpt->buttons.accel_0 = state->usr.accel_x;
  rpt->buttons.accel_1 = (state->usr.accel_z & 0x2) |
                         ((state->usr.accel_y >> 1) & 0x1);

  rpt->x = state->usr.accel_x >> 2;
  rpt->y = state->usr.accel_y >> 2;
  rpt->z = state->usr.accel_z >> 2;
}

void report_append_ir_10(struct wiimote_state * state, uint8_t * buf)
{
  struct report_ir_basic * rpt = (struct report_ir_basic *)buf;

  rpt->x1_lo = state->usr.ir_object[0].x;
  rpt->y1_lo = state->usr.ir_object[0].y;
  rpt->x1_hi = state->usr.ir_object[0].x >> 8;
  rpt->y1_hi = state->usr.ir_object[0].y >> 8;

  rpt->x2_lo = state->usr.ir_object[1].x;
  rpt->y2_lo = state->usr.ir_object[1].y;
  rpt->x2_hi = state->usr.ir_object[1].x >> 8;
  rpt->y2_hi = state->usr.ir_object[1].y >> 8;

  rpt->x3_lo = state->usr.ir_object[2].x;
  rpt->y3_lo = state->usr.ir_object[2].y;
  rpt->x3_hi = state->usr.ir_object[2].x >> 8;
  rpt->y3_hi = state->usr.ir_object[2].y >> 8;

  rpt->x4_lo = state->usr.ir_object[3].x;
  rpt->y4_lo = state->usr.ir_object[3].y;
  rpt->x4_hi = state->usr.ir_object[3].x >> 8;
  rpt->y4_hi = state->usr.ir_object[3].y >> 8;

}

void report_append_ir_12(struct wiimote_state * state, uint8_t * buf)
{
  struct report_ir_ext * rpt = (struct report_ir_ext *)buf;
  int i;

  for (i=0; i<4; i++)
  {
    rpt->obj[i].x_lo = state->usr.ir_object[i].x;
    rpt->obj[i].y_lo = state->usr.ir_object[i].y;
    rpt->obj[i].x_hi = state->usr.ir_object[i].x >> 8;
    rpt->obj[i].y_hi = state->usr.ir_object[i].y >> 8;
    rpt->obj[i].size = state->usr.ir_object[i].size;
  }

}

void report_append_interleaved(struct wiimote_state * state, uint8_t * buf)
{
  struct report_interleaved * rpt = (struct report_interleaved *)buf;
  int i;

  if (state->sys.reporting_mode == 0x3e)
  {
    rpt->buttons.accel_0 = state->usr.accel_z >> 4;
    rpt->buttons.accel_1 = state->usr.accel_z >> 6;
    rpt->accel = state->usr.accel_x >> 2;

    for (i=0; i<2; i++)
    {
      rpt->obj[i].x_lo = state->usr.ir_object[i].x;
      rpt->obj[i].y_lo = state->usr.ir_object[i].y;
      rpt->obj[i].x_hi = state->usr.ir_object[i].x >> 8;
      rpt->obj[i].y_hi = state->usr.ir_object[i].y >> 8;
      rpt->obj[i].size = state->usr.ir_object[i].size;
      rpt->obj[i].x_min = state->usr.ir_object[i].xmin;
      rpt->obj[i].y_min = state->usr.ir_object[i].ymin;
      rpt->obj[i].y_max = state->usr.ir_object[i].xmax;
      rpt->obj[i].y_max = state->usr.ir_object[i].ymax;
      rpt->obj[i].intensity = state->usr.ir_object[i].intensity;
    }

    state->sys.reporting_mode = 0x3f;
  }
  else
  {
    rpt->buttons.accel_0 = state->usr.accel_z;
    rpt->buttons.accel_1 = state->usr.accel_z >> 2;
    rpt->accel = state->usr.accel_y >> 2;

    for (i=0; i<2; i++)
    {
      rpt->obj[i].x_lo = state->usr.ir_object[i+2].x;
      rpt->obj[i].y_lo = state->usr.ir_object[i+2].y;
      rpt->obj[i].x_hi = state->usr.ir_object[i+2].x >> 8;
      rpt->obj[i].y_hi = state->usr.ir_object[i+2].y >> 8;
      rpt->obj[i].size = state->usr.ir_object[i+2].size;
      rpt->obj[i].x_min = state->usr.ir_object[i+2].xmin;
      rpt->obj[i].y_min = state->usr.ir_object[i+2].ymin;
      rpt->obj[i].y_max = state->usr.ir_object[i+2].xmax;
      rpt->obj[i].y_max = state->usr.ir_object[i+2].ymax;
      rpt->obj[i].intensity = state->usr.ir_object[i+2].intensity;
    }

    state->sys.reporting_mode = 0x3e;
  }
}

void report_append_extension(struct wiimote_state * state, uint8_t * buf, uint8_t bytes)
{
  //a600fe = 0x04 activate motionplus, 0x05 activate nunchuk passthrough, 0x07 activate classic passthrough
      //if no other extension, send 0x20

  //a600f0 = 0x55 deactivate motionplus
      //send report 0x20 twice (once for unplugged, once for plugged in)

  //0xa400fa contents
  //0000 A420 0000   Nunchuk
  //0000 A420 0101    Classic
  //0000 A420 0405    WMP
  //0000 A420 0505    WMP nunchuk
  //0000 A420 0705    WMP classic

  //0000 A620 0005    Inactive WMP
  //      ^=6         Deactivated WMP

  //these should be set to the the address offset of the extension data
  //and the length in bytes of the extesnion data
  //right now, they are always the same in all situations
  int addr_offset = 0x08, length = 6;

  switch (state->sys.extension_report_type)
  {
    case 0x00: //nunchuk
    {
      struct report_ext_nunchuk * rpt = (struct report_ext_nunchuk *)buf;

      rpt->x = state->usr.nunchuk.x;
      rpt->y = state->usr.nunchuk.y;

      rpt->accel_x_hi = state->usr.nunchuk.accel_x >> 2;
      rpt->accel_y_hi = state->usr.nunchuk.accel_y >> 2;
      rpt->accel_z_hi = state->usr.nunchuk.accel_z >> 2;
      rpt->accel_x_lo = state->usr.nunchuk.accel_x;
      rpt->accel_y_lo = state->usr.nunchuk.accel_y;
      rpt->accel_z_lo = state->usr.nunchuk.accel_z;

      rpt->c = !state->usr.nunchuk.c;
      rpt->z = !state->usr.nunchuk.z;

      break;
    }
    case 0x01: //classic
    {
      struct report_ext_classic * rpt = (struct report_ext_classic *)buf;

      rpt->lx = state->usr.classic.ls_x;
      rpt->ly = state->usr.classic.ls_y;
      rpt->rx_hi = state->usr.classic.rs_x >> 3;
      rpt->rx_m = state->usr.classic.rs_x >> 1;
      rpt->rx_lo = state->usr.classic.rs_x;
      rpt->ry = state->usr.classic.rs_y;

      rpt->lt_hi = state->usr.classic.lt >> 3;
      rpt->lt_lo = state->usr.classic.lt;
      rpt->rt = state->usr.classic.rt;

      rpt->left = !state->usr.classic.left;
      rpt->right = !state->usr.classic.right;
      rpt->up = !state->usr.classic.up;
      rpt->down = !state->usr.classic.down;
      rpt->ltrigger = !state->usr.classic.ltrigger;
      rpt->rtrigger = !state->usr.classic.rtrigger;
      rpt->lz = !state->usr.classic.lz;
      rpt->rz = !state->usr.classic.rz;
      rpt->a = !state->usr.classic.a;
      rpt->b = !state->usr.classic.b;
      rpt->x = !state->usr.classic.x;
      rpt->y = !state->usr.classic.y;
      rpt->plus = !state->usr.classic.plus;
      rpt->minus = !state->usr.classic.minus;
      rpt->home = !state->usr.classic.home;

      rpt->unused = 1;

      break;
    }
    case 0x04: //motionplus
    {
      struct report_ext_motionplus * rpt = (struct report_ext_motionplus *)buf;

      rpt->yaw_hi = state->usr.motionplus.yaw_down >> 8;
      rpt->yaw_lo = state->usr.motionplus.yaw_down;
      rpt->roll_hi = state->usr.motionplus.roll_left >> 8;
      rpt->roll_lo = state->usr.motionplus.roll_left;
      rpt->pitch_hi = state->usr.motionplus.pitch_left >> 8;
      rpt->pitch_lo = state->usr.motionplus.pitch_left;

      rpt->yaw_slow = state->usr.motionplus.yaw_slow;
      rpt->pitch_slow = state->usr.motionplus.pitch_slow;
      rpt->roll_slow = state->usr.motionplus.roll_slow;

      rpt->ext = 0;
      rpt->unused_0 = 1;

      break;
    }
    case 0x05: //motionplus + nunchuk
      if (state->sys.extension_report)
      {
        struct report_ext_motionplus * rpt = (struct report_ext_motionplus *)buf;

        rpt->yaw_hi = state->usr.motionplus.yaw_down >> 8;
        rpt->yaw_lo = state->usr.motionplus.yaw_down;
        rpt->roll_hi = state->usr.motionplus.roll_left >> 8;
        rpt->roll_lo = state->usr.motionplus.roll_left;
        rpt->pitch_hi = state->usr.motionplus.pitch_left >> 8;
        rpt->pitch_lo = state->usr.motionplus.pitch_left;

        rpt->yaw_slow = state->usr.motionplus.yaw_slow;
        rpt->pitch_slow = state->usr.motionplus.pitch_slow;
        rpt->roll_slow = state->usr.motionplus.roll_slow;

        rpt->ext = 1;
        rpt->unused_0 = 1;

        state->sys.extension_report = 0;
      }
      else
      {
        struct report_ext_nunchuk_pt * rpt = (struct report_ext_nunchuk_pt *)buf;

        rpt->x = state->usr.nunchuk.x;
        rpt->y = state->usr.nunchuk.y;

        rpt->accel_x_hi = state->usr.nunchuk.accel_x >> 2;
        rpt->accel_y_hi = state->usr.nunchuk.accel_y >> 2;
        rpt->accel_z_hi = state->usr.nunchuk.accel_z >> 3;
        rpt->accel_x_lo = state->usr.nunchuk.accel_x >> 1;
        rpt->accel_y_lo = state->usr.nunchuk.accel_y >> 1;
        rpt->accel_z_lo = state->usr.nunchuk.accel_z >> 1;

        rpt->c = !state->usr.nunchuk.c;
        rpt->z = !state->usr.nunchuk.z;

        rpt->ext = 1;

        state->sys.extension_report = 1;
      }
      break;
    case 0x07: //motionplus + classic
      if (state->sys.extension_report)
      {
        struct report_ext_motionplus * rpt = (struct report_ext_motionplus *)buf;

        rpt->yaw_hi = state->usr.motionplus.yaw_down >> 8;
        rpt->yaw_lo = state->usr.motionplus.yaw_down;
        rpt->roll_hi = state->usr.motionplus.roll_left >> 8;
        rpt->roll_lo = state->usr.motionplus.roll_left;
        rpt->pitch_hi = state->usr.motionplus.pitch_left >> 8;
        rpt->pitch_lo = state->usr.motionplus.pitch_left;

        rpt->yaw_slow = state->usr.motionplus.yaw_slow;
        rpt->pitch_slow = state->usr.motionplus.pitch_slow;
        rpt->roll_slow = state->usr.motionplus.roll_slow;

        rpt->ext = 1;
        rpt->unused_0 = 1;

        state->sys.extension_report = 0;
      }
      else
      {
        struct report_ext_classic_pt * rpt = (struct report_ext_classic_pt *)buf;

        rpt->lx = state->usr.classic.ls_x >> 1;
        rpt->ly = state->usr.classic.ls_y >> 1;
        rpt->rx_hi = state->usr.classic.rs_x >> 3;
        rpt->rx_m = state->usr.classic.rs_x >> 1;
        rpt->rx_lo = state->usr.classic.rs_x;
        rpt->ry = state->usr.classic.rs_y;

        rpt->lt_hi = state->usr.classic.lt >> 3;
        rpt->lt_lo = state->usr.classic.lt;
        rpt->rt = state->usr.classic.rt;

        rpt->left = !state->usr.classic.left;
        rpt->right = !state->usr.classic.right;
        rpt->up = !state->usr.classic.up;
        rpt->down = !state->usr.classic.down;
        rpt->ltrigger = !state->usr.classic.ltrigger;
        rpt->rtrigger = !state->usr.classic.rtrigger;
        rpt->lz = !state->usr.classic.lz;
        rpt->rz = !state->usr.classic.rz;
        rpt->a = !state->usr.classic.a;
        rpt->b = !state->usr.classic.b;
        rpt->x = !state->usr.classic.x;
        rpt->y = !state->usr.classic.y;
        rpt->plus = !state->usr.classic.plus;
        rpt->minus = !state->usr.classic.minus;
        rpt->home = !state->usr.classic.home;

        rpt->ext = 1;

        state->sys.extension_report = 1;
      }
      break;
  }


  if (state->sys.extension_encrypted)
  {
    ext_encrypt_bytes(&state->sys.extension_crypto_state, buf, addr_offset, length);
  }
}
