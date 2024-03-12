#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/flash.h"

#include "tusb.h"

#include "input_mode.h"

#define AIRCR_REG (*((volatile uint32_t *)(0xE000ED0C))) // Address of the AIRCR register
#define AIRCR_SYSRESETREQ (1 << 2) // Position of SYSRESETREQ bit in AIRCR
#define AIRCR_VECTKEY (0x5FA << 16) // VECTKEY value

#define FLASH_TARGET_OFFSET (256 * 1024)
#define FLASH_SIZE_BYTES (2 * 1024 * 1024)

void system_reset() {
    AIRCR_REG = AIRCR_VECTKEY | AIRCR_SYSRESETREQ;
    while(1);
}

bool store_input_mode(enum InputMode new_mode) 
{
    int buf[FLASH_PAGE_SIZE/sizeof(int)];
    memset(buf, 0xFF, FLASH_PAGE_SIZE);
    int saved_mode = new_mode; // changed to uint8?

    buf[0] = saved_mode;

    uint32_t saved_interrupts = save_and_disable_interrupts();

    flash_range_erase((FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
    flash_range_program((FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE), (uint8_t *)buf, FLASH_PAGE_SIZE);

    restore_interrupts(saved_interrupts);

    return true;

    // const uint8_t *flash_target_contents = (const uint8_t *)(XIP_BASE + FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE);

    // if ((uint8_t)saved_mode != *flash_target_contents) 
    // {
    //     return false;
    // }

    // return true;
}

bool change_input_mode(Gamepad previous_gamepad)
{
    if (!previous_gamepad.state.start)
    {
        return false;
    }

    InputMode new_mode;

    if (previous_gamepad.state.up)
    {
        new_mode = INPUT_MODE_XINPUT;
    }
    else if (previous_gamepad.state.left)
    {
        new_mode = INPUT_MODE_HID;
    }
    else if (previous_gamepad.state.right)
    {
        new_mode = INPUT_MODE_XBOXORIGINAL;
    }
    else if (previous_gamepad.state.down)
    {
        new_mode = INPUT_MODE_SWITCH;
    }
    else if (previous_gamepad.state.a)
    {
        new_mode = INPUT_MODE_PSCLASSIC;
    }

    bool mode_stored = false;

    if (new_mode)
    {
        // tinyusb needs to be kaput in order to write to the flash
        // just hangs otherwise

        tud_disconnect();
        sleep_ms(300);
        multicore_reset_core1(); // stop tusb host

        if (store_input_mode(new_mode))
        {
            system_reset(); // reset rp2040
            mode_stored = true;
        }
    }

    return mode_stored;
}

enum InputMode get_input_mode()
{
    #ifdef HOST_DEBUG
        return INPUT_MODE_USBSERIAL;
    #endif

    const uint8_t *stored_value = (const uint8_t *)(XIP_BASE + FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE);

    if (*stored_value >= INPUT_MODE_XINPUT && *stored_value <= INPUT_MODE_XBOXORIGINAL)
    {
        return(enum InputMode)*stored_value;
    } 
    else 
    {
        return INPUT_MODE_XBOXORIGINAL;
    }
}