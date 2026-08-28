#ifndef WIIMOTE_H
#define WIIMOTE_H

#include <stdint.h>
#include <stdbool.h>
#include "wm_crypto.h"

enum wiimote_connected_extension_type
{
  Nunchuk = 0x0,
  Classic = 0x1,
  BalanceBoard = 0x2,
  NoExtension = 0xff
};

struct wiimote_ir_object
{
  uint16_t x;
  uint16_t y;
  uint8_t size;
  uint8_t xmin;
  uint8_t ymin;
  uint8_t xmax;
  uint8_t ymax;
  uint8_t intensity;
};

struct wiimote_nunchuk
{
  uint16_t accel_x;
  uint16_t accel_y;
  uint16_t accel_z;
  uint8_t x;
  uint8_t y;
  bool c;
  bool z;
};

struct wiimote_classic
{
  bool a;
  bool b;
  bool x;
  bool y;
  bool minus;
  bool plus;
  bool rtrigger;
  bool ltrigger;
  bool home;
  bool rz;
  bool lz;
  bool up;
  bool down;
  bool left;
  bool right;
  uint8_t ls_x;
  uint8_t ls_y;
  uint8_t rs_x;
  uint8_t rs_y;
  uint8_t lt;
  uint8_t rt;

};

struct wiimote_motionplus
{
  uint16_t yaw_down;
  uint16_t roll_left;
  uint16_t pitch_left;
  bool yaw_slow;
  bool roll_slow;
  bool pitch_slow;
};

struct wiimote_state_usr
{
  bool a;
  bool b;
  bool minus;
  bool plus;
  bool home;
  bool one;
  bool two;
  bool up;
  bool down;
  bool left;
  bool right;

  //special buttons
  bool sync;
  bool power;

  //accelerometer (10 bit range)
  //0 acceleration is approximately 0x200
  uint16_t accel_x;
  uint16_t accel_y;
  uint16_t accel_z;

  struct wiimote_ir_object ir_object[4];

  enum wiimote_connected_extension_type connected_extension_type;

  struct wiimote_nunchuk nunchuk;
  struct wiimote_classic classic;
  struct wiimote_motionplus motionplus;
};

void reset_ir_object(struct wiimote_ir_object * object);
void reset_input_ir(struct wiimote_ir_object ir_object[4]);
void reset_input_nunchuk(struct wiimote_nunchuk * nunchuk);
void reset_input_classic(struct wiimote_classic * classic);
void reset_input_motionplus(struct wiimote_motionplus * motionplus);

struct wiimote_state_sys
{
  bool led_1;
  bool led_2;
  bool led_3;
  bool led_4;

  bool rumble;

  bool ircam_enabled;
  bool speaker_enabled;

  uint8_t battery_level;
  bool low_battery;

  int extension_hotplug_timer;
  bool extension_connected;
  enum wiimote_connected_extension_type connected_extension_type;

  struct ext_crypto_state extension_crypto_state;
  bool extension_report;
  bool extension_encrypted;
  uint8_t extension_report_type;
  uint8_t extension_type;
  uint8_t wmp_state; //0 inactive, 1 active, 2 deactivated

  uint8_t reporting_mode;
  bool reporting_continuous;
  bool report_changed;

  struct queued_report * queue;
  struct queued_report * queue_end;

  uint8_t register_a2[10]; //speaker
  uint8_t register_a4[256]; //extension
  uint8_t register_a6[256]; //wii motion plus
  uint8_t register_b0[52]; //ir camera
};

struct wiimote_state
{
  struct wiimote_state_sys sys;
  struct wiimote_state_usr usr;
};

//new adding -> only buttons struture for wiimote
struct wiimote_buttons{

  bool a;
  bool b;
  bool minus;
  bool plus;
  bool home;
  bool one;
  bool two;
  bool up;
  bool down;
  bool left;
  bool right;

  //special buttons
  bool sync;
  bool power;

  //accelerometer (10 bit range)
  //0 acceleration is approximately 0x200
  uint16_t accel_x;
  uint16_t accel_y;
  uint16_t accel_z;

  //ir
  int8_t ir_x;
  int8_t ir_y;
};

struct wii_console_info{
  uint8_t wii_addr_saved;
  uint8_t wii_addr[6];
};

//new adding -> wiimote + classic
typedef struct{
  struct wiimote_buttons wiimote;
  struct wiimote_nunchuk nunchuk;
  struct wiimote_classic classic;
  uint8_t switch_mode;
  uint8_t mode;
  uint8_t reset_ir;
  uint8_t fake_motion;
  uint8_t center_accel;
  uint8_t sideway;
  struct wii_console_info console_info;
}WiimoteReport;

enum MODE_WIIMOTE{
    NO_EXTENSION = 0xFF,
    WIIMOTE_AND_NUNCHUCK = 0x00,
    CLASSIC_CONTROLLER = 0x01
};

void wiimote_init(struct wiimote_state *state);
void wiimote_destroy(struct wiimote_state *state);

void wiimote_reset(struct wiimote_state *state);

int process_report(struct wiimote_state *state, const uint8_t *buf, int len);
int generate_report(struct wiimote_state * state, uint8_t * buf);

void read_eeprom(struct wiimote_state * state, uint32_t offset, uint16_t size);
void write_eeprom(struct wiimote_state * state, uint32_t offset, uint8_t size, const uint8_t * buf);
void read_register(struct wiimote_state *state, uint32_t offset, uint16_t size);
void write_register(struct wiimote_state *state, uint32_t offset, uint8_t size, const uint8_t * buf);

void init_extension(struct wiimote_state *state);

#endif //WIIMOTE_H
