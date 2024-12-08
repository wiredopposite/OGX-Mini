import { DEVICE_MODE_KEY_VALUE, PROFILE_ID_KEY_VALUE, BUTTON_KEY_VALUE, DPAD_KEY_VALUE, ANALOG_KEY_VALUE, new_packet } from './constants.js';

function percent_to_threshold(percent, max_value)
{
    return Math.round((percent / 100) * max_value);
}

function threshold_to_percent(threshold, max_value)
{
    return Math.round((threshold / max_value) * 100);
}

function update_deadzone_slider(slider_id, display_id, value, max_value)
{
    const percentage = threshold_to_percent(value, max_value);
    const slider = document.getElementById(slider_id);
    const display = document.getElementById(display_id);
    
    if (slider && display) 
    {
        slider.value = percentage;
        display.textContent = percentage;
        // console.log(`Setting ${slider_id} to ${percentage}%`);
    } 
    else 
    {
        console.error(`Slider or display element not found for ${slider_id}`);
    }

    slider.addEventListener('input', () => 
    {
        const updatedValue = threshold_to_percent(slider.value, 100);
        display.textContent = updatedValue;
    });
}

function populate_dropdown_menus(element_prefix, key_value)
{
    for (const [key, value] of Object.entries(key_value)) 
    {
        const select_element_id = `${element_prefix}${key.replace(/\s+/g, '-')}`;
        const select_element = document.getElementById(select_element_id);
    
        for (const [key_2, value_2] of Object.entries(key_value)) 
        {
            const option = document.createElement("option");
            option.value = value_2;
            option.text = key_2;
            select_element.add(option);
        }
    }
}

function populate_dropdown_menu(element_id, key_value)
{
    const select_element = document.getElementById(element_id);
    if (select_element) 
    {
        select_element.innerHTML = '';
        for (const [key, value] of Object.entries(key_value)) 
        {
            const option = document.createElement("option");
            option.value = value;
            option.text = key;
            select_element.add(option);
        }
    } 
    else 
    {
        console.error(`Element ${element_id} not found!`);
    }
}

export function populate_ui_elements()
{
    populate_dropdown_menu('profile-id', PROFILE_ID_KEY_VALUE);
    populate_dropdown_menu('device-mode', DEVICE_MODE_KEY_VALUE);

    update_deadzone_slider('deadzone-joy-l', 'deadzone-joy-l-value', 0, 100);
    update_deadzone_slider('deadzone-joy-r', 'deadzone-joy-r-value', 0, 100);
    update_deadzone_slider('deadzone-trigger-l', 'deadzone-trigger-l-value', 0, 100);
    update_deadzone_slider('deadzone-trigger-r', 'deadzone-trigger-r-value', 0, 100);

    populate_dropdown_menus('mapping-', DPAD_KEY_VALUE);
    populate_dropdown_menus('mapping-', BUTTON_KEY_VALUE);
    populate_dropdown_menus('mapping-analog-', ANALOG_KEY_VALUE);
}

const update_dropdown = (element_id, value) => 
{
    const element = document.getElementById(element_id);
    if (element) 
    {
        element.value = value;
    } 
    else 
    {
        console.error(`Element ${element_id} not found!`);
    }
};

export function update_ui_elements(in_packet)
{
    console.log(in_packet);
    console.log("Updating UI elements...");

    update_dropdown("device-mode", in_packet.input_mode);
    update_dropdown("profile-id", in_packet.profile.id);

    update_deadzone_slider('deadzone-joy-l', 'deadzone-joy-l-value', in_packet.profile.dz_joystick_l, 0xff);
    update_deadzone_slider('deadzone-joy-r', 'deadzone-joy-r-value', in_packet.profile.dz_joystick_r, 0xff);
    update_deadzone_slider('deadzone-trigger-l', 'deadzone-trigger-l-value', in_packet.profile.dz_trigger_l, 0xff);
    update_deadzone_slider('deadzone-trigger-r', 'deadzone-trigger-r-value', in_packet.profile.dz_trigger_r, 0xff);

    function update_checkbox(checkbox_id, display_id, value) 
    {
        const checkbox = document.getElementById(checkbox_id);
        const display = document.getElementById(display_id);

        if (checkbox && display) 
        {
            checkbox.checked = value === 1;
            // console.log(`Setting ${checkbox_id} to ${value === 1}`);
        } 
        else 
        {
            console.error(`Checkbox or display element not found for ${checkbox_id}`);
        }
    }

    update_checkbox('invert-joy-l', 'invert-joy-l-value', in_packet.profile.invert_ly);
    update_checkbox('invert-joy-r', 'invert-joy-r-value', in_packet.profile.invert_ry);

    update_dropdown("mapping-Up", in_packet.profile.dpad_up);
    update_dropdown("mapping-Down", in_packet.profile.dpad_down);
    update_dropdown("mapping-Left", in_packet.profile.dpad_left);
    update_dropdown("mapping-Right", in_packet.profile.dpad_right);

    update_dropdown("mapping-A", in_packet.profile.button_a);
    update_dropdown("mapping-B", in_packet.profile.button_b);
    update_dropdown("mapping-X", in_packet.profile.button_x);
    update_dropdown("mapping-Y", in_packet.profile.button_y);
    update_dropdown("mapping-L3", in_packet.profile.button_l3);
    update_dropdown("mapping-R3", in_packet.profile.button_r3);
    update_dropdown("mapping-Back", in_packet.profile.button_back);
    update_dropdown("mapping-Start", in_packet.profile.button_start);
    update_dropdown("mapping-LB", in_packet.profile.button_lb);
    update_dropdown("mapping-RB", in_packet.profile.button_rb);
    update_dropdown("mapping-Guide", in_packet.profile.button_sys);
    update_dropdown("mapping-Misc", in_packet.profile.button_misc);

    update_checkbox("analog-enabled", "analog-enabled-value", in_packet.profile.analog_enabled);

    update_dropdown("mapping-analog-Up", in_packet.profile.analog_off_up);
    update_dropdown("mapping-analog-Down", in_packet.profile.analog_off_down);
    update_dropdown("mapping-analog-Left", in_packet.profile.analog_off_left);
    update_dropdown("mapping-analog-Right", in_packet.profile.analog_off_right);
    update_dropdown("mapping-analog-A", in_packet.profile.analog_off_a);
    update_dropdown("mapping-analog-B", in_packet.profile.analog_off_b);
    update_dropdown("mapping-analog-X", in_packet.profile.analog_off_x);
    update_dropdown("mapping-analog-Y", in_packet.profile.analog_off_y);
    update_dropdown("mapping-analog-LB", in_packet.profile.analog_off_lb);
    update_dropdown("mapping-analog-RB", in_packet.profile.analog_off_rb);
}

export function create_packet_from_ui()
{
    const in_packet = new_packet();

    in_packet.report_id = 0;
    in_packet.input_mode = parseInt(document.getElementById("device-mode").value);
    in_packet.max_gamepads = 0;
    in_packet.player_idx = 0;

    in_packet.profile.id = parseInt(document.getElementById("profile-id").value);

    in_packet.profile.dz_joystick_l = percent_to_threshold(document.getElementById("deadzone-joy-l").value, 0xFF);
    in_packet.profile.dz_joystick_r = percent_to_threshold(document.getElementById("deadzone-joy-r").value, 0xFF);

    in_packet.profile.dz_trigger_l = percent_to_threshold(document.getElementById("deadzone-trigger-l").value, 0xFF);
    in_packet.profile.dz_trigger_r = percent_to_threshold(document.getElementById("deadzone-trigger-r").value, 0xFF);

    in_packet.profile.invert_ly = document.getElementById("invert-joy-l").checked ? 1 : 0;
    in_packet.profile.invert_ry = document.getElementById("invert-joy-r").checked ? 1 : 0;

    in_packet.profile.dpad_up = parseInt(document.getElementById("mapping-Up").value);
    in_packet.profile.dpad_down = parseInt(document.getElementById("mapping-Down").value);
    in_packet.profile.dpad_left = parseInt(document.getElementById("mapping-Left").value);
    in_packet.profile.dpad_right = parseInt(document.getElementById("mapping-Right").value);

    in_packet.profile.button_a = parseInt(document.getElementById("mapping-A").value);
    in_packet.profile.button_b = parseInt(document.getElementById("mapping-B").value);
    in_packet.profile.button_x = parseInt(document.getElementById("mapping-X").value);
    in_packet.profile.button_y = parseInt(document.getElementById("mapping-Y").value);
    in_packet.profile.button_l3 = parseInt(document.getElementById("mapping-L3").value);
    in_packet.profile.button_r3 = parseInt(document.getElementById("mapping-R3").value);
    in_packet.profile.button_back = parseInt(document.getElementById("mapping-Back").value);
    in_packet.profile.button_start = parseInt(document.getElementById("mapping-Start").value);
    in_packet.profile.button_lb = parseInt(document.getElementById("mapping-LB").value);
    in_packet.profile.button_rb = parseInt(document.getElementById("mapping-RB").value);
    in_packet.profile.button_sys = parseInt(document.getElementById("mapping-Guide").value);
    in_packet.profile.button_misc = parseInt(document.getElementById("mapping-Misc").value);

    in_packet.profile.analog_enabled = document.getElementById("analog-enabled").checked ? 1 : 0;

    in_packet.profile.analog_off_up = parseInt(document.getElementById("mapping-analog-Up").value);
    in_packet.profile.analog_off_down = parseInt(document.getElementById("mapping-analog-Down").value);
    in_packet.profile.analog_off_left = parseInt(document.getElementById("mapping-analog-Left").value);
    in_packet.profile.analog_off_right = parseInt(document.getElementById("mapping-analog-Right").value);
    in_packet.profile.analog_off_a = parseInt(document.getElementById("mapping-analog-A").value);
    in_packet.profile.analog_off_b = parseInt(document.getElementById("mapping-analog-B").value);
    in_packet.profile.analog_off_x = parseInt(document.getElementById("mapping-analog-X").value);
    in_packet.profile.analog_off_y = parseInt(document.getElementById("mapping-analog-Y").value);
    in_packet.profile.analog_off_lb = parseInt(document.getElementById("mapping-analog-LB").value);
    in_packet.profile.analog_off_rb = parseInt(document.getElementById("mapping-analog-RB").value); 

    return in_packet;
}