#ifndef _HOST_MANAGER_H_
#define _HOST_MANAGER_H_

#include <cstdint>
#include <memory>	
#include <array>
#include <atomic>

#include "board_config.h"
#include "Gamepad.h"
#include "OGXMini/OGXMini.h"
#include "USBHost/HardwareIDs.h"
#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostDriver/HostDriver.h"
#include "USBHost/HostDriver/PS5/PS5.h"
#include "USBHost/HostDriver/PS4/PS4.h"
#include "USBHost/HostDriver/PS3/PS3.h"
#include "USBHost/HostDriver/PSClassic/PSClassic.h"
#include "USBHost/HostDriver/DInput/DInput.h"
#include "USBHost/HostDriver/SwitchWired/SwitchWired.h"
#include "USBHost/HostDriver/SwitchPro/SwitchPro.h"
#include "USBHost/HostDriver/XInput/XboxOne.h"
#include "USBHost/HostDriver/XInput/Xbox360.h"
#include "USBHost/HostDriver/XInput/Xbox360W.h"
#include "USBHost/HostDriver/XInput/XboxOG.h"
#include "USBHost/HostDriver/N64/N64.h"
#include "USBHost/HostDriver/HIDGeneric/HIDGeneric.h"

class HostManager 
{
public:
	enum class DriverClass { NONE, HID, XINPUT };

	HostManager(HostManager const&) = delete;
	void operator=(HostManager const&)  = delete;

    static HostManager& get_instance() 
    {
		static HostManager instance;
		return instance;
	}

	inline void initialize(std::array<Gamepad, MAX_GAMEPADS>& gamepads) 
	{ 
		for (size_t i = 0; i < MAX_GAMEPADS; ++i)
		{
			gamepads_[i] = &gamepads[i];
		}
	}

	inline bool setup_driver(const HostDriver::Type driver_type, const uint8_t address, const uint8_t instance, uint8_t const* report_desc = nullptr, uint16_t desc_len = 0)
	{
		uint8_t gp_idx = find_free_gamepad();
		if (gp_idx == INVALID_IDX)
		{
			return false;
		}

		DriverClass driver_class = determine_driver_class(driver_type);
		uint8_t hs_idx = get_host_slot(driver_class, address);

		if (hs_idx == INVALID_IDX) //This is a new device, else it's another interface on an already mounted device
		{
			if ((hs_idx = find_free_host_slot()) == INVALID_IDX)
			{
				return false;
			}
		}

		HostSlot* host_slot = &host_slots_[hs_idx];
		if (instance >= host_slot->interfaces.size())
		{
			return false;
		}

		HostSlot::Interface* interface = &host_slot->interfaces[instance];

		switch (driver_type)
		{
			case HostDriver::Type::PS5:
				interface->driver = std::make_unique<PS5Host>(gp_idx);
				break;
			case HostDriver::Type::PS4:
				interface->driver = std::make_unique<PS4Host>(gp_idx);
				break;
			case HostDriver::Type::PS3:
				interface->driver = std::make_unique<PS3Host>(gp_idx);
				break;
			case HostDriver::Type::DINPUT:
				interface->driver = std::make_unique<DInputHost>(gp_idx);
				break;
			case HostDriver::Type::SWITCH:
				interface->driver = std::make_unique<SwitchWiredHost>(gp_idx);
				break;
			case HostDriver::Type::SWITCH_PRO:
				interface->driver = std::make_unique<SwitchProHost>(gp_idx);
				break;
			case HostDriver::Type::N64:
				interface->driver = std::make_unique<N64Host>(gp_idx);
				break;
			case HostDriver::Type::PSCLASSIC:
				interface->driver = std::make_unique<PSClassicHost>(gp_idx);
				break;
			case HostDriver::Type::XBOXOG:
				interface->driver = std::make_unique<XboxOGHost>(gp_idx);
				break;
			case HostDriver::Type::XBOXONE:
				interface->driver = std::make_unique<XboxOneHost>(gp_idx);
				break;
			case HostDriver::Type::XBOX360:
				interface->driver = std::make_unique<Xbox360Host>(gp_idx);
				break;
			case HostDriver::Type::XBOX360W: //Composite device, takes up all 4 gamepads when mounted
				interface->driver = std::make_unique<Xbox360WHost>(gp_idx);
				break;
			default:
				if (is_hid_gamepad(report_desc, desc_len))
				{
					interface->driver = std::make_unique<HIDHost>(gp_idx);
				}
				else
				{
					return false;
				}
				break;
		}

		host_slot->address = address;
		host_slot->driver_class = driver_class;

		interface->gamepad_idx = gp_idx;
		interface->gamepad = gamepads_[gp_idx];
		interface->driver->initialize(*interface->gamepad, host_slot->address, instance, report_desc, desc_len);

		if (OGXMini::update_tuh_status)
		{
			OGXMini::update_tuh_status(true);
		}

		return true;
	}

	inline void process_report(DriverClass driver_class, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
	{
		for (auto& host_slot : host_slots_)
		{
			if (host_slot.address == address && host_slot.driver_class == driver_class && host_slot.interfaces[instance].driver != nullptr)
			{
				host_slot.interfaces[instance].driver->process_report(*host_slot.interfaces[instance].gamepad, address, instance, report, len);
			}
		}
	}

	//Call on a timer
	inline void send_feedback()
	{
		for (auto& host_slot : host_slots_)
		{
			if (host_slot.address == INVALID_IDX)
			{
				continue;
			}
			for (uint8_t i = 0; i < host_slot.interfaces.size(); ++i)
			{
				if (host_slot.interfaces[i].driver != nullptr)
				{
					host_slot.interfaces[i].driver->send_feedback(*host_slot.interfaces[i].gamepad, host_slot.address, i);
					tuh_task();
				}
			}
		}
	}

    void deinit_driver(DriverClass driver_class, uint8_t address, uint8_t instance)
	{
		for (auto& host_slot : host_slots_)
		{
			if (host_slot.driver_class == driver_class && host_slot.address == address)
			{
				host_slot.reset();
			}
			// if (host_slot.address == address)
			// {
			// 	host_slot.reset();
			// }
		}

		if (OGXMini::update_tuh_status)
		{
			OGXMini::update_tuh_status(any_mounted());
		}
	}

	static inline HostDriver::Type get_type(const HardwareID& ids)
	{
		for (const auto& map : HOST_TYPE_MAP)
		{
			for (size_t i = 0; i < map.num_ids; i++)
			{
				if (ids.pid == map.ids[i].pid && ids.vid == map.ids[i].vid)
				{
					return map.type;
				}
			}
		}
		return HostDriver::Type::UNKNOWN;
	}

	static inline HostDriver::Type get_type(const tuh_xinput::DevType xinput_type)
	{
		switch (xinput_type)
		{
			case tuh_xinput::DevType::XBOXONE:
				return HostDriver::Type::XBOXONE;
			case tuh_xinput::DevType::XBOX360W:
				return HostDriver::Type::XBOX360W;
			case tuh_xinput::DevType::XBOX360:
				return HostDriver::Type::XBOX360;
			// case tuh_xinput::DevType::XBOX360_CHATPAD:
			// 	return HostDriver::Type::XBOX360_CHATPAD;
			case tuh_xinput::DevType::XBOXOG:
				return HostDriver::Type::XBOXOG;
			default:
				return HostDriver::Type::UNKNOWN;
		}
	}

	inline uint8_t get_gamepad_idx(DriverClass driver_class, uint8_t address, uint8_t instance)
	{
		for (auto& host_slot : host_slots_)
		{
			if (host_slot.driver_class == driver_class && host_slot.address == address && instance < host_slot.interfaces.size())
			{
				return host_slot.interfaces[instance].gamepad_idx;
			}
		}
		return INVALID_IDX;
	}

private:
	static constexpr uint8_t INVALID_IDX = 0xFF;

	struct HostSlot
	{
		DriverClass driver_class{DriverClass::NONE};
		uint8_t address{INVALID_IDX};

		struct Interface
		{
			std::unique_ptr<HostDriver> driver{nullptr};
			uint8_t gamepad_idx{INVALID_IDX};
			Gamepad* gamepad{nullptr};

			inline void reset()
			{
				driver.reset();
				gamepad_idx = INVALID_IDX;
				gamepad->reset_pad();
				gamepad = nullptr;
			}
		};

		std::array<Interface, MAX_GAMEPADS>interfaces;

		inline void reset()
		{
			address = INVALID_IDX;

			for (auto& interface : interfaces)
			{
				interface.reset();
			}
			
			driver_class = DriverClass::NONE;
		}
	};

	std::array<HostSlot, MAX_GAMEPADS> host_slots_;
	std::array<Gamepad*, MAX_GAMEPADS> gamepads_;

    HostManager() {}

	inline uint8_t find_free_host_slot()
	{
		for (uint8_t i = 0; i < host_slots_.size(); ++i)
		{
			if (host_slots_[i].address == INVALID_IDX)
			{
				return i;
			}
		}
		return INVALID_IDX;
	}

	inline uint8_t find_free_gamepad()
	{
		uint8_t count = 0;

		for (auto& host_slot : host_slots_)
		{
			for (auto& interface : host_slot.interfaces)
			{
				if (interface.gamepad_idx != INVALID_IDX)
				{
					++count;
				}
			}
		}
		return (count < MAX_GAMEPADS) ? count : INVALID_IDX;
	}

	inline uint8_t get_host_slot(DriverClass host_class, uint8_t address)
	{
		for (uint8_t i = 0; i < host_slots_.size(); ++i)
		{
			if (host_slots_[i].driver_class == host_class &&
				host_slots_[i].address == address)
			{
				return i;
			}
		}
		return INVALID_IDX;
	}

	inline DriverClass determine_driver_class(HostDriver::Type host_type)
	{
		switch (host_type)
		{
			case HostDriver::Type::XBOXOG:
			case HostDriver::Type::XBOXONE:
			case HostDriver::Type::XBOX360:
			case HostDriver::Type::XBOX360W:
				return DriverClass::XINPUT;
			default:
				return DriverClass::HID;
		}
	}

	bool is_hid_gamepad(const uint8_t* report_desc, uint16_t desc_len)
	{
		std::array<uint8_t, 6> start_bytes = { 0x05, 0x01, 0x09, 0x05, 0xA1, 0x01 };
		if (desc_len < start_bytes.size())
		{
			return false;
		}
		for (size_t i = 0; i < start_bytes.size(); ++i)
		{
			if (report_desc[i] != start_bytes[i])
			{
				return false;
			}
		}
		return true;
	}

	inline bool any_mounted() 
	{ 
		for (auto& host_slot : host_slots_)
		{
			if (host_slot.address != INVALID_IDX)
			{
				return true;
			}
		}
		return false;
	}
};

#endif // _HOST_MANAGER_H_