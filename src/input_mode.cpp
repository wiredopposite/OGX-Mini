#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hardware/flash.h"
#include "hardware/sync.h"

#include "input_mode.h"

#define AIRCR_Register (*((volatile uint32_t*)(PPB_BASE + 0x0ED0C)))

#define EEPROM_SIZE 1
#define EEPROM_FLASH_TARGET_OFFSET (2 * 1024 * 1024 - EEPROM_SIZE)

void store_input_mode(enum InputMode mode) 
{
    uint8_t data = (uint8_t)mode;

    uint32_t flash_offset = EEPROM_FLASH_TARGET_OFFSET % FLASH_SECTOR_SIZE;
    uint32_t flash_sector_base = EEPROM_FLASH_TARGET_OFFSET - flash_offset;

    uint8_t new_sector_content[FLASH_SECTOR_SIZE];

    memcpy(new_sector_content, (const void *)(XIP_BASE + flash_sector_base), FLASH_SECTOR_SIZE);

    new_sector_content[flash_offset] = data;

    flash_range_erase(flash_sector_base, FLASH_SECTOR_SIZE);
    flash_range_program(flash_sector_base, new_sector_content, FLASH_SECTOR_SIZE);
}

void change_input_mode(Gamepad previous_gamepad)
{
    if (!previous_gamepad.state.start)
    {
        return;
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
    // else if (previous_gamepad.state.b)
    // {
    //     new_mode = INPUT_MODE_USBSERIAL;
    // }
    else
    {
        return;
    }

    store_input_mode(new_mode);
    sleep_ms(200);

    // restart the rp2040
    AIRCR_Register = 0x5FA0004;
    sleep_ms(200);
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