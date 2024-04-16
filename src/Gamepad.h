#pragma once

#ifndef _GAMEPAD_H_
#define _GAMEPAD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <cstdint>

struct GamepadButtons
{
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
};

struct GamepadTriggers
{
	uint8_t l 		{0};
	uint8_t r 		{0};
};

struct GamepadJoysticks
{
	int16_t lx 		{0};
	int16_t ly 		{0};
	int16_t rx 		{0};
	int16_t ry 		{0};
};

struct GamepadRumble
{
	uint8_t l	{0};
	uint8_t r	{0};
};

class Gamepad 
{
	public:
		GamepadButtons buttons;
		GamepadTriggers triggers;
		GamepadJoysticks joysticks;
		GamepadRumble rumble;

		void reset_pad();
		void reset_rumble();
		void reset_hid_rumble();
};

Gamepad& gamepad(int idx);

#ifdef __cplusplus
}
#endif

#endif // _GAMEPAD_H_
