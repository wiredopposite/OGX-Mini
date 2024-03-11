#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hardware/watchdog.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

#include "input_mode.h"

#define EEPROM_SIZE 1
#define EEPROM_FLASH_TARGET_OFFSET (2 * 1024 * 1024 - EEPROM_SIZE)

void software_reset()
{
    watchdog_enable(1, 1);
    while(1);
}

void store_input_mode(enum InputMode new_mode) 
{
    uint8_t data = (uint8_t)new_mode;

    uint32_t flash_offset = EEPROM_FLASH_TARGET_OFFSET % FLASH_SECTOR_SIZE;
    uint32_t flash_sector_base = EEPROM_FLASH_TARGET_OFFSET - flash_offset;

    uint8_t new_sector_content[FLASH_SECTOR_SIZE];

    memcpy(new_sector_content, (const void *)(XIP_BASE + flash_sector_base), FLASH_SECTOR_SIZE);

    new_sector_content[flash_offset] = data;

    flash_range_erase(flash_sector_base, FLASH_SECTOR_SIZE);
    flash_range_program(flash_sector_base, new_sector_content, FLASH_SECTOR_SIZE);
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

    if (new_mode)
    {
        store_input_mode(new_mode);
        sleep_ms(800);
        software_reset();

        return true;
    }

    return false;
}

enum InputMode get_input_mode()
{
    #ifdef HOST_DEBUG
        return INPUT_MODE_USBSERIAL;
    #endif

    const uint8_t* flash_addr = (const uint8_t*)(XIP_BASE + EEPROM_FLASH_TARGET_OFFSET);
    uint8_t stored_value = *flash_addr;

    if (stored_value >= INPUT_MODE_XINPUT && stored_value <= INPUT_MODE_XBOXORIGINAL) 
    {
        return(enum InputMode)stored_value;
    } 
    else 
    {
        return INPUT_MODE_XBOXORIGINAL;
    }
}