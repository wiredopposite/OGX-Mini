#ifndef _HOST_MANAGER_H_
#define _HOST_MANAGER_H_

#include <cstdint>
#include <memory>	
#include <hardware/regs/usb.h>
#include <hardware/irq.h>
#include <hardware/structs/usb.h>
#include <hardware/resets.h>

#include "Board/Config.h"
#include "Board/board_api.h"
#include "Board/ogxm_log.h"
#if defined(CONFIG_EN_USB_HOST)
#include "pio_usb.h"
#endif
#include "UserSettings/UserSettings.h"
#include "USBDevice/DeviceDriver/DeviceDriverTypes.h"
#include "USBHost/HardwareIDs.h"

#if defined(CONFIG_OGXM_DEBUG)
#define debug_printf OGXM_LOG
#else
#define debug_printf(...) ((void)0)
#endif
#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostDriver/HostDriver.h"
#include "USBHost/HostDriver/PS5/PS5.h"
#include "USBHost/HostDriver/PS4/PS4.h"
#include "USBHost/HostDriver/PS3/PS3.h"
#include "USBHost/HostDriver/PSClassic/PSClassic.h"
#include "USBHost/HostDriver/DInput/DInput.h"
#include "USBHost/HostDriver/SwitchWired/SwitchWired.h"
#include "USBHost/HostDriver/SwitchPro/SwitchPro.h"
#include "USBHost/HostDriver/SwitchPro/Switch2ProHost.h"
#include "USBHost/HostDriver/XInput/XboxOne.h"
#include "USBHost/HostDriver/XInput/Xbox360.h"
#include "USBHost/HostDriver/XInput/Xbox360W.h"
#include "USBHost/HostDriver/XInput/XboxOG.h"
#include "USBHost/HostDriver/N64/N64.h"
#include "USBHost/HostDriver/HIDGeneric/HIDGeneric.h"

/** Per USB device: TinyUSB HID instance indices and XInput instance indices are separate namespaces
 *  (both often start at 0). Reserve [0 .. MAX_GAMEPADS-1] for HID and [MAX_GAMEPADS ..] for XInput.
 *  OGXM_TUH_XINPUT_INSTANCES is 4 when MAX_GAMEPADS < 4 (360 wireless receiver). */
#define MAX_INTERFACES (MAX_GAMEPADS + OGXM_TUH_XINPUT_INSTANCES)

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

	inline void initialize(Gamepad (&gamepads)[MAX_GAMEPADS]) 
	{ 
		for (size_t i = 0; i < MAX_GAMEPADS; ++i)
		{
			gamepads_[i] = &gamepads[i];
		}
		last_usb_host_input_ms_ = board_api::ms_since_boot();
	}

	//XInput doesn't need report_desc or desc_len
	inline bool setup_driver(const HostDriverType driver_type, DriverClass dclass, const uint8_t address, const uint8_t instance, uint8_t const* report_desc = nullptr, uint16_t desc_len = 0)
	{
		const uint8_t si = host_storage_index(dclass, instance);
		if (si == INVALID_IDX || si >= MAX_INTERFACES)
		{
			return false;
		}

		//Check if this device is already mounted
		uint8_t dev_idx = get_device_slot(address);
		//If not, find a free device slot
		if (dev_idx == INVALID_IDX && ((dev_idx = find_free_device_slot()) == INVALID_IDX)) 
		{
			return false;
		}

		Device& device_slot = device_slots_[dev_idx];

		/* Xbox 360 wireless receiver: one HostManager driver, four tuh_xinput instances. */
		if (driver_type == HostDriverType::XBOX360W && device_slot.address == address)
		{
			for (uint8_t i = 0; i < MAX_INTERFACES; ++i)
			{
				if (i == si)
				{
					continue;
				}
				const Interface& other = device_slot.interfaces[i];
				if (other.driver && other.host_driver_type == HostDriverType::XBOX360W)
				{
					return true;
				}
			}
		}

		/* One physical pad on a composite that exposes both XInput and HID (e.g. some third-party
		 * controllers): share gamepad index. Xbox 360 wireless receiver: 4 XInput interfaces share
		 * one gamepad when MAX_GAMEPADS is 1. */
		uint8_t gp_idx = INVALID_IDX;
		if (device_slot.address == address)
		{
			for (uint8_t i = 0; i < MAX_INTERFACES; ++i)
			{
				if (i == si)
				{
					continue;
				}
				const Interface& other = device_slot.interfaces[i];
				if (!other.driver || other.gamepad_idx == INVALID_IDX || other.driver_class == DriverClass::NONE)
				{
					continue;
				}
				const bool complementary =
					(dclass == DriverClass::HID && other.driver_class == DriverClass::XINPUT) ||
					(dclass == DriverClass::XINPUT && other.driver_class == DriverClass::HID);
				const bool xbox360w_sibling =
					(driver_type == HostDriverType::XBOX360W &&
					 other.host_driver_type == HostDriverType::XBOX360W);
				if (complementary || xbox360w_sibling)
				{
					gp_idx = other.gamepad_idx;
					break;
				}
			}
		}
		if (gp_idx == INVALID_IDX)
		{
			gp_idx = find_free_gamepad();
		}
		if (gp_idx == INVALID_IDX)
		{
			return false;
		}

		Interface& interface = device_slot.interfaces[si];

		debug_printf("Attempting to allocate driver for index %d\n", gp_idx);

		switch (driver_type)
		{
			case HostDriverType::PS5:
				debug_printf("PS5 Loaded\n"); fflush(stdout);
				interface.driver = std::make_unique<PS5Host>(gp_idx);
				break;
			case HostDriverType::PS4:
				debug_printf("PS4 Loaded\n"); fflush(stdout);
				interface.driver = std::make_unique<PS4Host>(gp_idx);
				break;
			case HostDriverType::PS3:
				debug_printf("PS3 Loaded\n"); fflush(stdout);
				interface.driver = std::make_unique<PS3Host>(gp_idx);
				break;
			case HostDriverType::DINPUT:
				debug_printf("DINPUT Loaded\n"); fflush(stdout);
				interface.driver = std::make_unique<DInputHost>(gp_idx);
				break;
			case HostDriverType::SWITCH:
				debug_printf("SWITCH Loaded\n"); fflush(stdout);
				interface.driver = std::make_unique<SwitchWiredHost>(gp_idx);
				break;
			case HostDriverType::SWITCH_PRO_2:
				debug_printf("SWITCH PRO 2 Loaded\n"); fflush(stdout);
				interface.driver = std::make_unique<Switch2ProHost>(gp_idx);
				break;
			case HostDriverType::SWITCH_PRO:
				debug_printf("SWITCH PRO Loaded\n"); fflush(stdout);
				interface.driver = std::make_unique<SwitchProHost>(gp_idx);
				break;
			case HostDriverType::N64:
				debug_printf("N64 Loaded\n"); fflush(stdout);
				interface.driver = std::make_unique<N64Host>(gp_idx);
				break;
			case HostDriverType::PSCLASSIC:
				debug_printf("PSCLASSIC Loaded\n"); fflush(stdout);
				interface.driver = std::make_unique<PSClassicHost>(gp_idx);
				break;
			case HostDriverType::XBOXOG:
				debug_printf("XBOXOG Loaded\n"); fflush(stdout);
				interface.driver = std::make_unique<XboxOGHost>(gp_idx);
				break;
			case HostDriverType::XBOXONE:
				debug_printf("XBOXONE Loaded\n"); fflush(stdout);
				interface.driver = std::make_unique<XboxOneHost>(gp_idx);
				break;
			case HostDriverType::XBOX360:
				debug_printf("XBOX360 Loaded\n"); fflush(stdout);
				interface.driver = std::make_unique<Xbox360Host>(gp_idx);
				break;
			case HostDriverType::XBOX360W: //Composite device, takes up all 4 gamepads when mounted
				debug_printf("XBOX360W Loaded\n"); fflush(stdout);
				interface.driver = std::make_unique<Xbox360WHost>(gp_idx);
				break;
			default:
				if (is_hid_gamepad(report_desc, desc_len))
				{
					interface.driver = std::make_unique<HIDHost>(gp_idx);
					debug_printf("HIDHOST Loaded\n"); fflush(stdout);
				}
				else
				{
					return false;
				}
				break;
		}

		{
			HostDriverType stored_type = driver_type;
			if (stored_type == HostDriverType::UNKNOWN && dclass == DriverClass::HID && interface.driver)
			{
				stored_type = HostDriverType::HID_GENERIC;
			}
			interface.host_driver_type = stored_type;
		}

		device_slot.address = address;
		interface.driver_class = dclass;
		interface.usb_instance = instance;
		interface.gamepad_idx = gp_idx;
		interface.gamepad = gamepads_[gp_idx];
		// Wii U GC adapter: Xbox controllers report positive Y for up; Nintendo use negative
		const bool xbox_stick_y = (driver_type == HostDriverType::XBOXONE || driver_type == HostDriverType::XBOX360
			|| driver_type == HostDriverType::XBOX360W || driver_type == HostDriverType::XBOXOG);
		interface.gamepad->set_stick_y_positive_is_up(xbox_stick_y);
		interface.driver->initialize(*interface.gamepad, device_slot.address, instance, report_desc, desc_len);

		record_usb_host_input_activity();
		return true;
	}

	inline void process_report(DriverClass dclass, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
	{
		record_usb_host_input_activity();

		/* Same physical pad often exposes XInput + PS HID. Both were calling set_pad_in() on one Gamepad:
		 * inverted sticks, missed input, and flaky state. Prefer HID for DS4/DS5; drop XInput *processing*
		 * only (still re-arm IN below). */
		if (dclass == DriverClass::XINPUT)
		{
			for (const auto& device_slot : device_slots_)
			{
				if (device_slot.address != address)
				{
					continue;
				}
				for (const auto& iface : device_slot.interfaces)
				{
					if (iface.driver && iface.driver_class == DriverClass::HID &&
					    (iface.host_driver_type == HostDriverType::PS4 || iface.host_driver_type == HostDriverType::PS5))
					{
						(void)report;
						(void)len;
						tuh_xinput::receive_report(address, instance);
						return;
					}
				}
				break;
			}
		}

		const uint8_t si = host_storage_index(dclass, instance);
		if (si >= MAX_INTERFACES)
		{
			return;
		}
		for (auto& device_slot : device_slots_)
		{
			if (device_slot.address != address)
			{
				continue;
			}
			HostDriver* driver = device_slot.interfaces[si].driver.get();
			Gamepad* gamepad = device_slot.interfaces[si].gamepad;
			if (!driver || !gamepad)
			{
				/* 360 wireless receiver: one HostManager driver serves all TinyUSB instances. */
				for (const auto& iface : device_slot.interfaces)
				{
					if (iface.driver && iface.gamepad &&
					    iface.driver_class == DriverClass::XINPUT &&
					    iface.host_driver_type == HostDriverType::XBOX360W)
					{
						driver = iface.driver.get();
						gamepad = iface.gamepad;
						break;
					}
				}
			}
			if (driver && gamepad)
			{
				driver->process_report(*gamepad, address, instance, report, len);
			}
			break;
		}
	}

	inline void connect_cb(DriverClass dclass, uint8_t address, uint8_t instance)
	{
		const uint8_t si = host_storage_index(dclass, instance);
		if (si >= MAX_INTERFACES)
		{
			return;
		}
		for (auto& device_slot : device_slots_)
		{
			if (device_slot.address != address)
			{
				continue;
			}
			HostDriver* driver = device_slot.interfaces[si].driver.get();
			Gamepad* gamepad = device_slot.interfaces[si].gamepad;
			if (!driver || !gamepad)
			{
				for (const auto& iface : device_slot.interfaces)
				{
					if (iface.driver && iface.gamepad &&
					    iface.driver_class == DriverClass::XINPUT &&
					    iface.host_driver_type == HostDriverType::XBOX360W)
					{
						driver = iface.driver.get();
						gamepad = iface.gamepad;
						break;
					}
				}
			}
			if (driver && gamepad)
			{
				driver->connect_cb(*gamepad, address, instance);
			}
			break;
		}
	}

	inline void disconnect_cb(DriverClass dclass, uint8_t address, uint8_t instance)
	{
		const uint8_t si = host_storage_index(dclass, instance);
		if (si >= MAX_INTERFACES)
		{
			return;
		}
		for (auto& device_slot : device_slots_)
		{
			if (device_slot.address != address)
			{
				continue;
			}
			HostDriver* driver = device_slot.interfaces[si].driver.get();
			Gamepad* gamepad = device_slot.interfaces[si].gamepad;
			if (!driver || !gamepad)
			{
				for (const auto& iface : device_slot.interfaces)
				{
					if (iface.driver && iface.gamepad &&
					    iface.driver_class == DriverClass::XINPUT &&
					    iface.host_driver_type == HostDriverType::XBOX360W)
					{
						driver = iface.driver.get();
						gamepad = iface.gamepad;
						break;
					}
				}
			}
			if (driver && gamepad)
			{
				driver->disconnect_cb(*gamepad, address, instance);
			}
			break;
		}
	}

	//Call on a timer
	inline void send_feedback()
	{
		const uint32_t now_ms = board_api::ms_since_boot();
		for (auto& device_slot : device_slots_)
		{
			if (device_slot.address == INVALID_IDX)
			{
				continue;
			}
			/* Composite "PS4 mode" pads often expose both XInput (GIP) and HID. Sending Xbox rumble +
			 * PS4 HID output together breaks many third-party firmwares (input dies after a few seconds). */
			bool ps_style_hid = false;
			bool has_xinput = false;
			for (const auto& iface : device_slot.interfaces)
			{
				if (!iface.driver)
				{
					continue;
				}
				if (iface.driver_class == DriverClass::XINPUT)
				{
					has_xinput = true;
				}
				if (iface.driver_class == DriverClass::HID &&
				    (iface.host_driver_type == HostDriverType::PS4 || iface.host_driver_type == HostDriverType::PS5))
				{
					ps_style_hid = true;
				}
			}
			for (uint8_t i = 0; i < MAX_INTERFACES; ++i)
			{
				Interface& iface = device_slot.interfaces[i];
				if (!iface.driver)
				{
					continue;
				}
				if (iface.host_driver_type == HostDriverType::XBOX360W)
				{
					tuh_xinput::service_wireless_ports(device_slot.address);
				}
				if (iface.driver_class == DriverClass::XINPUT &&
				    iface.host_driver_type == HostDriverType::XBOXONE)
				{
					tuh_xinput::service_gip(device_slot.address, iface.usb_instance);
				}
				/* Pure first-party DS4 (HID only): 200 ms OUT refresh. Was incorrectly gated on
				 * !ps_style_hid, which is never true for a PS4 iface — so pure DS4 never got periodic
				 * OUT and relied on rare composite keepalive / rumble only (#47 Pico W). */
				const bool ps4_hid_periodic =
					iface.driver_class == DriverClass::HID && iface.host_driver_type == HostDriverType::PS4 &&
					!has_xinput;
				/* PS3: invoke send_feedback on timer; driver rate-limits OUT to 1 Hz on PIO USB. */
				const bool ps3_hid_periodic =
					iface.driver_class == DriverClass::HID && iface.host_driver_type == HostDriverType::PS3;
				/* Switch Pro / Pro 2: init steps and idle rumble keepalive need periodic OUT on PIO USB. */
				const bool switch_pro_hid_periodic =
					iface.driver_class == DriverClass::HID &&
					(iface.host_driver_type == HostDriverType::SWITCH_PRO ||
					 iface.host_driver_type == HostDriverType::SWITCH_PRO_2);
				/* Composite XInput+PS4 HID: rare keepalive only (200 ms OUT chokes many third-party pads). */
				bool ps4_composite_keepalive = false;
				if (has_xinput && ps_style_hid && iface.driver_class == DriverClass::HID &&
				    iface.host_driver_type == HostDriverType::PS4 && iface.gamepad_idx < MAX_GAMEPADS)
				{
					const uint8_t gi = iface.gamepad_idx;
					if (now_ms >= ps4_composite_next_keepalive_ms_[gi])
					{
						ps4_composite_next_keepalive_ms_[gi] = now_ms + 4000u;
						ps4_composite_keepalive = true;
					}
				}
				if (!(iface.gamepad->new_pad_out() || iface.gamepad->has_rumble() || ps4_hid_periodic ||
				      ps3_hid_periodic || switch_pro_hid_periodic || ps4_composite_keepalive))
				{
					continue;
				}
				if (ps_style_hid && iface.driver_class == DriverClass::XINPUT)
				{
					continue;
				}
				iface.driver->send_feedback(*iface.gamepad, device_slot.address, iface.usb_instance);
				tuh_task();
#if defined(CONFIG_EN_USB_HOST)
				/* Nested tuh_task during OUT must not starve PIO USB SOF or wired IN dies in ~1–2 s. */
				pio_usb_host_frame();
#endif
			}
		}
	}

    void deinit_driver(DriverClass driver_class, uint8_t address, uint8_t instance)
	{
		const uint8_t si = host_storage_index(driver_class, instance);
		if (si >= MAX_INTERFACES)
		{
			return;
		}
		for (auto& device_slot : device_slots_)
		{
			if (device_slot.address != address)
			{
				continue;
			}
			Interface& iface = device_slot.interfaces[si];
			const uint8_t cleared_gamepad_idx = iface.gamepad_idx;
			if (iface.driver && iface.gamepad)
			{
				iface.driver->disconnect_cb(*iface.gamepad, address, instance);
			}
			iface.driver.reset();
			iface.driver_class = DriverClass::NONE;
			iface.host_driver_type = HostDriverType::UNKNOWN;
			iface.gamepad_idx = INVALID_IDX;
			iface.gamepad = nullptr;
			iface.usb_instance = INVALID_IDX;

			bool any_driver = false;
			for (auto& i : device_slot.interfaces)
			{
				if (i.driver)
				{
					any_driver = true;
					break;
				}
			}
			if (!any_driver)
			{
				if (cleared_gamepad_idx < MAX_GAMEPADS)
				{
					ps4_composite_next_keepalive_ms_[cleared_gamepad_idx] = 0;
				}
				device_slot.reset();
			}
			return;
		}
	}

	static inline HostDriverType get_type(const HardwareID& ids)
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
		return HostDriverType::UNKNOWN;
	}

	static inline HostDriverType get_type(const tuh_xinput::DevType xinput_type)
	{
		switch (xinput_type)
		{
			case tuh_xinput::DevType::XBOXONE:
				return HostDriverType::XBOXONE;
			case tuh_xinput::DevType::XBOX360W:
				return HostDriverType::XBOX360W;
			case tuh_xinput::DevType::XBOX360:
				return HostDriverType::XBOX360;
			// case tuh_xinput::DevType::XBOX360_CHATPAD:
			// 	return HostDriverType::XBOX360_CHATPAD;
			case tuh_xinput::DevType::XBOXOG:
				return HostDriverType::XBOXOG;
			default:
				return HostDriverType::UNKNOWN;
		}
	}

	inline uint8_t get_gamepad_idx(DriverClass driver_class, uint8_t address, uint8_t instance)
	{
		const uint8_t si = host_storage_index(driver_class, instance);
		if (si >= MAX_INTERFACES)
		{
			return INVALID_IDX;
		}
		for (auto& device_slot : device_slots_)
		{
			if (device_slot.address != address)
			{
				continue;
			}
			if (device_slot.interfaces[si].gamepad_idx != INVALID_IDX)
			{
				return device_slot.interfaces[si].gamepad_idx;
			}
			for (const auto& iface : device_slot.interfaces)
			{
				if (iface.driver_class == DriverClass::XINPUT &&
				    iface.host_driver_type == HostDriverType::XBOX360W &&
				    iface.gamepad_idx != INVALID_IDX)
				{
					return iface.gamepad_idx;
				}
			}
			break;
		}
		return INVALID_IDX;
	}

	/** Live Switch Pro / Switch 2 Pro host driver for async bulk bring-up callbacks. */
	inline SwitchProHost* get_switch_pro_host(uint8_t address, uint8_t instance) const
	{
		const uint8_t si = host_storage_index(DriverClass::HID, instance);
		if (si >= MAX_INTERFACES)
		{
			return nullptr;
		}
		for (const auto& device_slot : device_slots_)
		{
			if (device_slot.address != address)
			{
				continue;
			}
			const Interface& iface = device_slot.interfaces[si];
			if (!iface.driver ||
			    (iface.host_driver_type != HostDriverType::SWITCH_PRO &&
			     iface.host_driver_type != HostDriverType::SWITCH_PRO_2))
			{
				return nullptr;
			}
			return static_cast<SwitchProHost*>(iface.driver.get());
		}
		return nullptr;
	}

	inline bool any_mounted() 
	{ 
		for (auto& device_slot : device_slots_)
		{
			if (device_slot.address != INVALID_IDX)
			{
				return true;
			}
		}
		return false;
	}

	/** Last time a USB IN report reached process_report (Pico W unplug watchdog when D+/D− GPIO lies under PIO). */
	inline void record_usb_host_input_activity()
	{
		last_usb_host_input_ms_ = board_api::ms_since_boot();
	}

	inline uint32_t usb_host_input_idle_ms() const
	{
		return board_api::ms_since_boot() - last_usb_host_input_ms_;
	}

	/** True if this device address already has any host driver (e.g. HID). Used to prefer XInput over HID for the same device. */
	inline bool address_has_driver(uint8_t address) const
	{
		for (const auto& device_slot : device_slots_)
		{
			if (device_slot.address != address)
				continue;
			for (const auto& iface : device_slot.interfaces)
			{
				if (iface.driver != nullptr)
					return true;
			}
			return false;
		}
		return false;
	}

private:
	static constexpr uint8_t INVALID_IDX = 0xFF;

	static inline uint8_t host_storage_index(DriverClass cls, uint8_t tinusb_instance)
	{
		if (cls == DriverClass::XINPUT)
		{
			if (tinusb_instance >= OGXM_TUH_XINPUT_INSTANCES)
			{
				return INVALID_IDX;
			}
			return static_cast<uint8_t>(MAX_GAMEPADS + tinusb_instance);
		}
		if (tinusb_instance >= MAX_GAMEPADS)
		{
			return INVALID_IDX;
		}
		return tinusb_instance;
	}

	struct Interface
	{
		std::unique_ptr<HostDriver> driver{nullptr};
		Gamepad* gamepad{nullptr};
		uint8_t gamepad_idx{INVALID_IDX};
		/** TinyUSB HID or XInput instance number for control/out transfers */
		uint8_t usb_instance{INVALID_IDX};
		DriverClass driver_class{DriverClass::NONE};
		HostDriverType host_driver_type{HostDriverType::UNKNOWN};
	};
	struct Device
	{
		uint8_t address{INVALID_IDX};
		Interface interfaces[MAX_INTERFACES];

		void reset()
		{
			address = INVALID_IDX;
			for (auto& interface : interfaces)
			{
				interface.driver.reset();
				interface.driver_class = DriverClass::NONE;
				interface.host_driver_type = HostDriverType::UNKNOWN;
				interface.gamepad_idx = INVALID_IDX;
				interface.gamepad = nullptr;
				interface.usb_instance = INVALID_IDX;
			}
		}
	};

	Device device_slots_[MAX_GAMEPADS];
	Gamepad* gamepads_[MAX_GAMEPADS];
	/** PS4+GIP composite: wall-time spacing for optional lightbar/rumble OUT refresh (see send_feedback). */
	uint32_t ps4_composite_next_keepalive_ms_[MAX_GAMEPADS]{};
	uint32_t last_usb_host_input_ms_{0};

    HostManager() {}

	inline uint8_t find_free_device_slot()
	{
		for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
		{
			if (device_slots_[i].address == INVALID_IDX)
			{
				return i;
			}
		}
		return INVALID_IDX;
	}

	inline uint8_t find_free_gamepad()
	{
		bool used[MAX_GAMEPADS] = {false};
		UserSettings& us = UserSettings::get_instance();
		DeviceDriverType dr = us.get_current_driver();
		HostInputSource src = us.get_input_source();
		if ((dr == DeviceDriverType::PS1PS2 && src == HostInputSource::PSX_GPIO) ||
		    (dr == DeviceDriverType::GAMECUBE && src == HostInputSource::GAMECUBE_GPIO) ||
		    (dr == DeviceDriverType::DREAMCAST && src == HostInputSource::DREAMCAST_GPIO) ||
		    (src == HostInputSource::N64_GPIO))
		{
			used[0] = true;
		}
		for (auto& device_slot : device_slots_)
		{
			for (auto& interface : device_slot.interfaces)
			{
				if (interface.gamepad_idx != INVALID_IDX)
				{
					used[interface.gamepad_idx] = true;
				}
			}
		}
		for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
		{
			if (!used[i])
			{
				return i;
			}
		}
		return INVALID_IDX;
	}

	inline uint8_t get_device_slot(uint8_t address)
	{
		if (address > MAX_GAMEPADS)
		{
			return INVALID_IDX;
		}
		return address - 1;
	}

	// inline DriverClass determine_driver_class(HostDriver::Type host_type)
	// {
	// 	switch (host_type)
	// 	{
	// 		case HostDriver::Type::XBOXOG:
	// 		case HostDriver::Type::XBOXONE:
	// 		case HostDriver::Type::XBOX360:
	// 		case HostDriver::Type::XBOX360W:
	// 			return DriverClass::XINPUT;
	// 		default:
	// 			return DriverClass::HID;
	// 	}
	// }

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

	inline HostDriver* get_driver_by_gamepad(uint8_t gamepad_idx)
	{
		for (const auto& device_slot : device_slots_)
		{
			for (const auto& interface : device_slot.interfaces)
			{
				if (interface.gamepad_idx == gamepad_idx)
				{
					return interface.driver.get();
				}
			}
		}
		return nullptr;
	}
};

#endif // _HOST_MANAGER_H_

// #ifndef _HOST_MANAGER_H_
// #define _HOST_MANAGER_H_

// #include <cstdint>
// #include <memory>	

// #include "board_config.h"
// #include "Gamepad.h"
// #include "OGXMini/OGXMini.h"
// #include "USBHost/HardwareIDs.h"
// #include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
// #include "USBHost/HostDriver/HostDriver.h"
// // #include "USBHost/HostDriver/PS5/PS5.h"
// // #include "USBHost/HostDriver/PS4/PS4.h"
// // #include "USBHost/HostDriver/PS3/PS3.h"
// // #include "USBHost/HostDriver/PSClassic/PSClassic.h"
// // #include "USBHost/HostDriver/DInput/DInput.h"
// // #include "USBHost/HostDriver/SwitchWired/SwitchWired.h"
// // #include "USBHost/HostDriver/SwitchPro/SwitchPro.h"
// #include "USBHost/HostDriver/XInput/XboxOne.h"
// #include "USBHost/HostDriver/XInput/Xbox360.h"
// #include "USBHost/HostDriver/XInput/Xbox360W.h"
// #include "USBHost/HostDriver/XInput/XboxOG.h"
// // #include "USBHost/HostDriver/N64/N64.h"
// // #include "USBHost/HostDriver/HIDGeneric/HIDGeneric.h"

// #define MAX_INTERFACES MAX_GAMEPADS //This may change if support is added for audio or other chatpads beside 360 wireless

// class HostManager 
// {
// public:
// 	enum class DriverClass { NONE, HID, XINPUT };

// 	HostManager(HostManager const&) = delete;
// 	void operator=(HostManager const&)  = delete;

//     static HostManager& get_instance() 
//     {
// 		static HostManager instance;
// 		return instance;
// 	}

// 	inline void initialize(Gamepad (&gamepads)[MAX_GAMEPADS]) 
// 	{ 
// 		for (size_t i = 0; i < MAX_GAMEPADS; ++i)
// 		{
// 			gamepads_[i] = &gamepads[i];
// 		}
// 	}

// 	inline bool setup_driver(const HostDriver::Type driver_type, const uint8_t address, const uint8_t instance, uint8_t const* report_desc = nullptr, uint16_t desc_len = 0)
// 	{
// 		uint8_t gp_idx = find_free_gamepad();
// 		if (gp_idx == INVALID_IDX)
// 		{
// 			return false;
// 		}

// 		DriverClass driver_class = determine_driver_class(driver_type);
// 		uint8_t hs_idx = get_host_slot(driver_class, address);

// 		if (hs_idx == INVALID_IDX) //This is a new device, else it's another interface on an already mounted device
// 		{
// 			if ((hs_idx = find_free_host_slot()) == INVALID_IDX)
// 			{
// 				return false;
// 			}
// 		}

// 		HostSlot* host_slot = &host_slots_[hs_idx];
// 		if (instance >= MAX_INTERFACES)
// 		{
// 			return false;
// 		}

// 		HostSlot::Interface* interface = &host_slot->interfaces[instance];

// 		switch (driver_type)
// 		{
// 			// case HostDriver::Type::PS5:
// 			// 	interface->driver = std::make_unique<PS5Host>(gp_idx);
// 			// 	break;
// 			// case HostDriver::Type::PS4:
// 			// 	interface->driver = std::make_unique<PS4Host>(gp_idx);
// 			// 	break;
// 			// case HostDriver::Type::PS3:
// 			// 	interface->driver = std::make_unique<PS3Host>(gp_idx);
// 			// 	break;
// 			// case HostDriver::Type::DINPUT:
// 			// 	interface->driver = std::make_unique<DInputHost>(gp_idx);
// 			// 	break;
// 			// case HostDriver::Type::SWITCH:
// 			// 	interface->driver = std::make_unique<SwitchWiredHost>(gp_idx);
// 			// 	break;
// 			// case HostDriver::Type::SWITCH_PRO:
// 			// 	interface->driver = std::make_unique<SwitchProHost>(gp_idx);
// 			// 	break;
// 			// case HostDriver::Type::N64:
// 			// 	interface->driver = std::make_unique<N64Host>(gp_idx);
// 			// 	break;
// 			// case HostDriver::Type::PSCLASSIC:
// 			// 	interface->driver = std::make_unique<PSClassicHost>(gp_idx);
// 			// 	break;
// 			case HostDriver::Type::XBOXOG:
// 				interface->driver = std::make_unique<XboxOGHost>(gp_idx);
// 				break;
// 			case HostDriver::Type::XBOXONE:
// 				interface->driver = std::make_unique<XboxOneHost>(gp_idx);
// 				break;
// 			case HostDriver::Type::XBOX360:
// 				interface->driver = std::make_unique<Xbox360Host>(gp_idx);
// 				break;
// 			case HostDriver::Type::XBOX360W: //Composite device, takes up all 4 gamepads when mounted
// 				interface->driver = std::make_unique<Xbox360WHost>(gp_idx);
// 				break;
// 			default:
// 				// if (is_hid_gamepad(report_desc, desc_len))
// 				// {
// 				// 	interface->driver = std::make_unique<HIDHost>(gp_idx);
// 				// }
// 				// else
// 				{
// 					return false;
// 				}
// 				break;
// 		}

// 		host_slot->address = address;
// 		host_slot->driver_class = driver_class;

// 		interface->gamepad_idx = gp_idx;
// 		interface->gamepad = gamepads_[gp_idx];
// 		interface->driver->initialize(*interface->gamepad, host_slot->address, instance, report_desc, desc_len);

// 		// OGXMini::update_tuh_status(true);

// 		return true;
// 	}

// 	inline void process_report(DriverClass driver_class, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
// 	{
// 		for (auto& host_slot : host_slots_)
// 		{
// 			if (host_slot.address == address && 
// 				host_slot.driver_class == driver_class && 
// 				host_slot.interfaces[instance].driver &&
// 				host_slot.interfaces[instance].gamepad)
// 			{
// 				host_slot.interfaces[instance].driver->process_report(*host_slot.interfaces[instance].gamepad, address, instance, report, len);
// 			}
// 		}
// 	}

// 	//Call on a timer
// 	inline void send_feedback()
// 	{
// 		for (auto& host_slot : host_slots_)
// 		{
// 			if (host_slot.address == INVALID_IDX)
// 			{
// 				continue;
// 			}
// 			for (uint8_t i = 0; i < MAX_INTERFACES; ++i)
// 			{
// 				if (host_slot.interfaces[i].driver != nullptr && host_slot.interfaces[i].gamepad->new_pad_out())
// 				{
// 					host_slot.interfaces[i].driver->send_feedback(*host_slot.interfaces[i].gamepad, host_slot.address, i);
// 					tuh_task();
// 				}
// 			}
// 		}
// 	}

//     void deinit_driver(DriverClass driver_class, uint8_t address, uint8_t instance)
// 	{
// 		for (auto& host_slot : host_slots_)
// 		{
// 			if (host_slot.driver_class == driver_class && host_slot.address == address)
// 			{
// 				TU_LOG2("Deinit driver\r\n");
// 				// host_slot.reset();
// 				TU_LOG2("Driver deinitialized\r\n");
// 			}
// 		}

// 		// OGXMini::update_tuh_status(any_mounted());
// 	}

// 	static inline HostDriver::Type get_type(const HardwareID& ids)
// 	{
// 		for (const auto& map : HOST_TYPE_MAP)
// 		{
// 			for (size_t i = 0; i < map.num_ids; i++)
// 			{
// 				if (ids.pid == map.ids[i].pid && ids.vid == map.ids[i].vid)
// 				{
// 					return map.type;
// 				}
// 			}
// 		}
// 		return HostDriver::Type::UNKNOWN;
// 	}

// 	static inline HostDriver::Type get_type(const tuh_xinput::DevType xinput_type)
// 	{
// 		switch (xinput_type)
// 		{
// 			case tuh_xinput::DevType::XBOXONE:
// 				return HostDriver::Type::XBOXONE;
// 			case tuh_xinput::DevType::XBOX360W:
// 				return HostDriver::Type::XBOX360W;
// 			case tuh_xinput::DevType::XBOX360:
// 				return HostDriver::Type::XBOX360;
// 			// case tuh_xinput::DevType::XBOX360_CHATPAD:
// 			// 	return HostDriver::Type::XBOX360_CHATPAD;
// 			case tuh_xinput::DevType::XBOXOG:
// 				return HostDriver::Type::XBOXOG;
// 			default:
// 				return HostDriver::Type::UNKNOWN;
// 		}
// 	}

// 	inline uint8_t get_gamepad_idx(DriverClass driver_class, uint8_t address, uint8_t instance)
// 	{
// 		for (auto& host_slot : host_slots_)
// 		{
// 			if (host_slot.driver_class == driver_class && host_slot.address == address && instance < MAX_INTERFACES)
// 			{
// 				return host_slot.interfaces[instance].gamepad_idx;
// 			}
// 		}
// 		return INVALID_IDX;
// 	}

// 	inline bool any_mounted() 
// 	{ 
// 		for (auto& host_slot : host_slots_)
// 		{
// 			if (host_slot.address != INVALID_IDX)
// 			{
// 				return true;
// 			}
// 		}
// 		return false;
// 	}

// private:
// 	static constexpr uint8_t INVALID_IDX = 0xFF;

// 	struct HostSlot
// 	{
// 		DriverClass driver_class{DriverClass::NONE};
// 		uint8_t address{INVALID_IDX};

// 		struct Interface
// 		{
// 			std::unique_ptr<HostDriver> driver{nullptr};
// 			uint8_t gamepad_idx{INVALID_IDX};
// 			Gamepad* gamepad{nullptr};

// 			inline void reset()
// 			{
// 				// gamepad->reset_pad_in();
// 				gamepad = nullptr;
// 				gamepad_idx = INVALID_IDX;
// 				driver.reset();
// 			}
// 		};

// 		Interface interfaces[MAX_INTERFACES];

// 		inline void reset()
// 		{
// 			// address = INVALID_IDX;

// 			for (auto& interface : interfaces)
// 			{
// 				interface.reset();
// 			}
			
// 			driver_class = DriverClass::NONE;
// 		}
// 	};

// 	HostSlot host_slots_[MAX_GAMEPADS];
// 	Gamepad* gamepads_[MAX_GAMEPADS];

//     HostManager() {}

// 	inline uint8_t find_free_host_slot()
// 	{
// 		for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
// 		{
// 			if (host_slots_[i].address == INVALID_IDX)
// 			{
// 				return i;
// 			}
// 		}
// 		return INVALID_IDX;
// 	}

// 	inline uint8_t find_free_gamepad()
// 	{
// 		uint8_t count = 0;

// 		for (auto& host_slot : host_slots_)
// 		{
// 			for (auto& interface : host_slot.interfaces)
// 			{
// 				if (interface.gamepad_idx != INVALID_IDX)
// 				{
// 					++count;
// 				}
// 			}
// 		}
// 		return (count < MAX_GAMEPADS) ? count : INVALID_IDX;
// 	}

// 	inline uint8_t get_host_slot(DriverClass host_class, uint8_t address)
// 	{
// 		for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
// 		{
// 			if (host_slots_[i].driver_class == host_class &&
// 				host_slots_[i].address == address)
// 			{
// 				return i;
// 			}
// 		}
// 		return INVALID_IDX;
// 	}

// 	inline DriverClass determine_driver_class(HostDriver::Type host_type)
// 	{
// 		switch (host_type)
// 		{
// 			case HostDriver::Type::XBOXOG:
// 			case HostDriver::Type::XBOXONE:
// 			case HostDriver::Type::XBOX360:
// 			case HostDriver::Type::XBOX360W:
// 				return DriverClass::XINPUT;
// 			default:
// 				return DriverClass::HID;
// 		}
// 	}

// 	bool is_hid_gamepad(const uint8_t* report_desc, uint16_t desc_len)
// 	{
// 		std::array<uint8_t, 6> start_bytes = { 0x05, 0x01, 0x09, 0x05, 0xA1, 0x01 };
// 		if (desc_len < start_bytes.size())
// 		{
// 			return false;
// 		}
// 		for (size_t i = 0; i < start_bytes.size(); ++i)
// 		{
// 			if (report_desc[i] != start_bytes[i])
// 			{
// 				return false;
// 			}
// 		}
// 		return true;
// 	}
// };

// #endif // _HOST_MANAGER_H_