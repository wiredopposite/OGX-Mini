//Portability note: may need modifcation for different targets (endianness, etc.)

#ifndef WM_REPORTS_H
#define WM_REPORTS_H

#include "wiimote.h"
#include <stdint.h>

#define OFFSET24(offset32) ((offset32)<<8)

struct report_data
{
  uint8_t io;
  uint8_t type;
  uint8_t buf[21];
  uint8_t padding;
} __attribute__((packed));

struct report
{
  uint32_t len;  //data (packet) length
  struct report_data data;
};

struct queued_report
{
  struct queued_report * next;
  struct report rpt;
};

/* Output reports (from controller) */

struct report_buttons
{
  int left:1;
  int right:1;
  int down:1;
  int up:1;
  int plus:1;
  int accel_0:2;
  int unused:1;

  int two:1;
  int one:1;
  int b:1;
  int a:1;
  int minus:1;
  int accel_1:2;
  int home:1;
} __attribute__((packed));

struct report_accelerometer
{
  struct report_buttons buttons;

  uint8_t x;
  uint8_t y;
  uint8_t z;
} __attribute__((packed));

struct report_ir_basic
{
  uint8_t x1_lo;
  uint8_t y1_lo;

  int x2_hi:2;
  int y2_hi:2;
  int x1_hi:2;
  int y1_hi:2;

  uint8_t x2_lo;
  uint8_t y2_lo;

  uint8_t x3_lo;
  uint8_t y3_lo;

  int x4_hi:2;
  int y4_hi:2;
  int x3_hi:2;
  int y3_hi:2;

  uint8_t x4_lo;
  uint8_t y4_lo;
} __attribute__((packed));

struct report_ir_ext_obj
{
  uint8_t x_lo;
  uint8_t y_lo;

  int size:4;
  int x_hi:2;
  int y_hi:2;
} __attribute__((packed));

struct report_ir_ext
{
  struct report_ir_ext_obj obj[4];
} __attribute__((packed));

struct report_ir_full_obj
{
  uint8_t x_lo;
  uint8_t y_lo;

  int size:4;
  int x_hi:2;
  int y_hi:2;

  int x_min:7;
  int unused0:1;
  int y_min:7;
  int unused1:1;
  int x_max:7;
  int unused2:1;
  int y_max:7;
  int unused3:1;

  uint8_t unused4;
  uint8_t intensity;
} __attribute__((packed));

struct report_interleaved
{
  struct report_buttons buttons;

  uint8_t accel;

  struct report_ir_full_obj obj[2];
} __attribute__((packed));

struct report_ext_nunchuk
{
  uint8_t x;
  uint8_t y;
  uint8_t accel_x_hi;
  uint8_t accel_y_hi;
  uint8_t accel_z_hi;

  int z:1;
  int c:1;
  int accel_x_lo:2;
  int accel_y_lo:2;
  int accel_z_lo:2;
} __attribute__((packed));

struct report_ext_nunchuk_pt
{
  //different format if in passthrough mode with motionplus
  uint8_t x;
  uint8_t y;
  uint8_t accel_x_hi;
  uint8_t accel_y_hi;

  int accel_z_hi:7;
  int ext:1;

  int unused:2;
  int z:1;
  int c:1;
  int accel_x_lo:1;
  int accel_y_lo:1;
  int accel_z_lo:2;
} __attribute__((packed));

struct report_ext_classic
{
  int lx:6;
  int rx_hi:2;

  int ly:6;
  int rx_m:2;

  int ry:5;
  int lt_hi:2;
  int rx_lo:1;

  int rt:5;
  int lt_lo:3;

  int unused:1;
  int rtrigger:1;
  int plus:1;
  int home:1;
  int minus:1;
  int ltrigger:1;
  int down:1;
  int right:1;

  int up:1;
  int left:1;
  int rz:1;
  int x:1;
  int a:1;
  int y:1;
  int b:1;
  int lz:1;
} __attribute__((packed));

struct report_ext_classic_pt
{
  //different format if in passthrough mode with motionplus
  int up:1;
  int lx:5;
  int rx_hi:2;

  int left:1;
  int ly:5;
  int rx_m:2;

  int ry:5;
  int lt_hi:2;
  int rx_lo:1;

  int lt_lo:3;
  int rt:5;

  int ext:1;
  int rtrigger:1;
  int plus:1;
  int home:1;
  int minus:1;
  int ltrigger:1;
  int down:1;
  int right:1;

  int unused:2;
  int rz:1;
  int x:1;
  int a:1;
  int y:1;
  int b:1;
  int lz:1;
} __attribute__((packed));

struct report_ext_motionplus
{
  uint8_t yaw_lo;
  uint8_t roll_lo;
  uint8_t pitch_lo;

  int pitch_slow:1;
  int yaw_slow:1;
  int yaw_hi:6;

  int ext:1;
  int roll_slow:1;
  int roll_hi:6;

  int unused_1:1;
  int unused_0:1;
  int pitch_hi:6;
} __attribute__((packed));

struct report_status
{
  struct report_buttons buttons;

  int low_battery:1;
  int extension_connected:1;
  int speaker_enabled:1;
  int ircam_enabled:1;
  int led_1:1;
  int led_2:1;
  int led_3:1;
  int led_4:1;

  uint8_t unused0;
  uint8_t unused1;

  uint8_t battery_level;
} __attribute__((packed));

struct report_mem_resp
{
  struct report_buttons buttons;

  int error:4;
  int size:4;

  uint16_t addr;  //big endian

  uint8_t data[16];
} __attribute__((packed));

struct report_ack
{
  struct report_buttons buttons;

  uint8_t report;
  uint8_t result;
} __attribute__((packed));

/* Input reports (from wii) */

struct report_rumble
{
  int rumble:1;
  int unused:7;
} __attribute__((packed));

struct report_leds
{
  int rumble:1;
  int unused:3;
  int led_1:1;
  int led_2:1;
  int led_3:1;
  int led_4:1;
} __attribute__((packed));

struct report_mode
{
  int rumble:1;
  int unused1:1;
  int continuous:1;
  int unused0:5;

  uint8_t mode;
} __attribute__((packed));

struct report_ir_enable
{
  int rumble:1;
  int ack_requested:1;
  int enabled:1;
  int unused0:5;
} __attribute__((packed));

struct report_speaker_enable
{
  int rumble:1;
  int unused1:1;
  int muted:1;
  int unused0:5;
} __attribute__((packed));

struct report_mem_read
{
  int rumble:1;
  int unused1:1;
  int source0:1;
  int source1:1;
  int unused0:4;

  uint32_t offset:24; //big endian
  uint16_t size;      //big endian
} __attribute__((packed));

struct report_mem_write
{
  int rumble:1;
  int unused1:1;
  int source0:1;
  int source1:1;
  int unused0:4;

  uint32_t offset:24; //big endian
  uint8_t size;

  uint8_t data[16];
} __attribute__((packed));

struct report * report_queue_push(struct wiimote_state * state);
struct report * report_queue_peek(struct wiimote_state * state);
void report_queue_pop(struct wiimote_state * state);

void report_queue_push_ack(struct wiimote_state *state, uint8_t report, uint8_t result);
void report_queue_push_status(struct wiimote_state * state);

void report_format_mem_resp(struct wiimote_state * state, struct report * rpt,
  int size, int error, uint16_t addr, uint8_t * buf, bool encrypt);

void report_append_buttons(struct wiimote_state * state, uint8_t * buf);
void report_append_accelerometer(struct wiimote_state * state, uint8_t * buf);
void report_append_ir_10(struct wiimote_state * state, uint8_t * buf);
void report_append_ir_12(struct wiimote_state * state, uint8_t * buf);
void report_append_interleaved(struct wiimote_state * state, uint8_t * buf);
void report_append_extension(struct wiimote_state * state, uint8_t * buf, uint8_t bytes);

#endif
