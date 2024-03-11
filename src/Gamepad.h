#pragma once

#ifndef _GAMEPAD_H_
#define _GAMEPAD_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <cstdint>

#include "usbh/tusb_hid/ps3.h"
#include "usbh/tusb_hid/ps4.h"
#include "usbh/tusb_hid/ps5.h"
#include "usbh/tusb_xinput/xinput_host.h"

struct GamepadState{
	bool up 		{false};
	bool down 		{false};
	bool left 		{false};
	bool right 		{false};
	bool a			{false};
	bool b 			{false};
	bool x 			{false};
	bool y 			{false};
	bool l3			{false};
	bool r3 		{false};
	bool back 		{false};
	bool start 		{false};
	bool rb 		{false};
	bool lb 		{false};
	bool sys 		{false};
	bool misc		{false};
	uint8_t lt 		{0};
	uint8_t rt 		{0};
	int16_t lx 		{0};
	int16_t ly 		{0};
	int16_t rx 		{0};
	int16_t ry 		{0};
};

struct GamepadOutState{
	uint8_t lrumble	{0};
	uint8_t rrumble	{0};
};

class Gamepad {
public:
    GamepadState state;

	void update_gamepad_state_from_xinput(const xinput_gamepad_t* xinput_data);
	void reset_state();
};

class GamepadOut {
public:
	GamepadOutState out_state;

	void update_gamepad_rumble(uint8_t left_rumble, uint8_t right_rumble);
	void rumble_hid_reset();
};

extern Gamepad gamepad;
extern GamepadOut gamepadOut;

#ifdef __cplusplus
	}
#endif

#endif // _GAMEPAD_H_
