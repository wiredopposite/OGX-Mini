#ifndef INPUT_MODE_H_
#define INPUT_MODE_H_

#ifdef __cplusplus
extern "C" {
#endif

enum InputMode
{
    INPUT_MODE_XINPUT,
    INPUT_MODE_SWITCH,
    // INPUT_MODE_HID,
    // INPUT_MODE_KEYBOARD,
    // INPUT_MODE_PS4,
    // INPUT_MODE_XBONE,
    // INPUT_MODE_MDMINI,
    // INPUT_MODE_NEOGEO,
    // INPUT_MODE_PCEMINI,
    // INPUT_MODE_EGRET,
    // INPUT_MODE_ASTRO,
    // INPUT_MODE_PSCLASSIC,
    INPUT_MODE_XBOXORIGINAL,
    // INPUT_MODE_CONFIG,
};

// void check_and_update_input_mode (uint8_t input_id);
enum InputMode load_input_mode();

#ifdef __cplusplus
}
#endif

#endif // INPUT_MODE_H_