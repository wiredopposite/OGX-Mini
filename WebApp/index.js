import { populate_ui_elements, update_ui_elements, create_packet_from_ui } from './ui_modules.js';
import { PACKET_IDS, new_packet } from './constants.js';

async function send_report(writer, in_packet)
{
    const packet = new Uint8Array([
        in_packet.packet_id,
        in_packet.input_mode,
        in_packet.max_gamepads,
        in_packet.player_idx,

        in_packet.profile.id,

        in_packet.profile.dz_trigger_l,
        in_packet.profile.dz_trigger_r,

        in_packet.profile.dz_joystick_l,
        in_packet.profile.dz_joystick_r,

        in_packet.profile.invert_ly ? 1 : 0,
        in_packet.profile.invert_ry ? 1 : 0,

        in_packet.profile.dpad_up,
        in_packet.profile.dpad_down,
        in_packet.profile.dpad_left,
        in_packet.profile.dpad_right,

        in_packet.profile.button_a & 0xff,
        in_packet.profile.button_a >> 8,
        in_packet.profile.button_b & 0xff,
        in_packet.profile.button_b >> 8,
        in_packet.profile.button_x & 0xff,
        in_packet.profile.button_x >> 8,
        in_packet.profile.button_y & 0xff,
        in_packet.profile.button_y >> 8,
        in_packet.profile.button_l3 & 0xff,
        in_packet.profile.button_l3 >> 8,
        in_packet.profile.button_r3 & 0xff,
        in_packet.profile.button_r3 >> 8,
        in_packet.profile.button_back & 0xff,
        in_packet.profile.button_back >> 8,
        in_packet.profile.button_start & 0xff,
        in_packet.profile.button_start >> 8,
        in_packet.profile.button_lb & 0xff,
        in_packet.profile.button_lb >> 8,
        in_packet.profile.button_rb & 0xff,
        in_packet.profile.button_rb >> 8,
        in_packet.profile.button_sys & 0xff,
        in_packet.profile.button_sys >> 8,
        in_packet.profile.button_misc & 0xff,
        in_packet.profile.button_misc >> 8,

        in_packet.profile.analog_enabled ? 1 : 0,

        in_packet.profile.analog_off_up,
        in_packet.profile.analog_off_down,
        in_packet.profile.analog_off_left,
        in_packet.profile.analog_off_right,
        in_packet.profile.analog_off_a,
        in_packet.profile.analog_off_b,
        in_packet.profile.analog_off_x,
        in_packet.profile.analog_off_y,
        in_packet.profile.analog_off_lb,
        in_packet.profile.analog_off_rb,
    ]);

    await writer.write(packet);
    console.log("Packet sent, length:", packet.length);
}

function in_packet_from_in_report(data) 
{
    const in_packet = new_packet();

    in_packet.packet_id = data[0],   
    in_packet.input_mode = data[1],  
    in_packet.max_gamepads = data[2],
    in_packet.player_idx = data[3],         

    in_packet.profile = {
        id: data[4],                      // uint8_t

        dz_trigger_l: data[5],             // uint8_t
        dz_trigger_r: data[6],             // uint8_t

        dz_joystick_l: data[7],          // uint8_t
        dz_joystick_r: data[8],          // uint8_t

        invert_ly: data[9],               // uint8_t
        invert_ry: data[10],              // uint8_t

        dpad_up: data[11],                     // uint8_t
        dpad_down: data[12],                   // uint8_t
        dpad_left: data[13],                   // uint8_t
        dpad_right: data[14],                  // uint8_t

        button_a: data[15] | (data[16] << 8),    // uint16_t
        button_b: data[17] | (data[18] << 8),    // uint16_t
        button_x: data[19] | (data[20] << 8),    // uint16_t
        button_y: data[21] | (data[22] << 8),    // uint16_t
        button_l3: data[23] | (data[24] << 8),   // uint16_t
        button_r3: data[25] | (data[26] << 8),   // uint16_t
        button_back: data[27] | (data[28] << 8), // uint16_t
        button_start: data[29] | (data[30] << 8),// uint16_t
        button_lb: data[31] | (data[32] << 8),   // uint16_t
        button_rb: data[33] | (data[34] << 8),   // uint16_t
        button_sys: data[35] | (data[36] << 8),  // uint16_t
        button_misc: data[37] | (data[38] << 8), // uint16_t

        analog_enabled: data[39],         // uint8_t

        analog_off_up: data[40],          // uint8_t
        analog_off_down: data[41],        // uint8_t
        analog_off_left: data[42],        // uint8_t
        analog_off_right: data[43],       // uint8_t
        analog_off_a: data[44],           // uint8_t
        analog_off_b: data[45],           // uint8_t
        analog_off_x: data[46],           // uint8_t
        analog_off_y: data[47],           // uint8_t
        analog_off_lb: data[48],          // uint8_t
        analog_off_rb: data[49],          // uint8_t
    }
    return in_packet;
}

function process_in_packet(data) 
{
    if (data.length != 50) 
    {
        console.error("Received packet length does not match.");
        return;
    }

    if (data[0] === PACKET_IDS.RESPONSE_OK) 
    {
        console.log("Packet OK.");
        update_ui_elements(in_packet_from_in_report(data));
    } 
    else if (data[0] === PACKET_IDS.RESPONSE_ERROR) 
    {
        console.error("Packet error.");
    } 
    else 
    {
        console.error("Unknown packet ID.");
    }
}

async function connect_to_device()
{
    if (!("serial" in navigator)) 
    {
        console.log("Web Serial API not supported.");
        return;
    }

    populate_ui_elements();

    try 
    {
        const filters = [{ usbVendorId: 0xCafe }];
        const port = await navigator.serial.requestPort({  });
        await port.open({ baudRate: 9600 });

        const reader = port.readable.getReader();
        const writer = port.writable.getWriter();

        const init_packet = new_packet();
        init_packet.packet_id = PACKET_IDS.INIT_READ;
        await send_report(writer, init_packet);

        document.getElementById('connect-ui').classList.add('hide');
        document.getElementById('settings-ui').classList.remove('hide');

        async function read_serial_data() 
        {
            let in_data = new Uint8Array();
            try 
            {
                while (true) 
                {
                    const { value, done } = await reader.read();
        
                    if (done) 
                    {
                        console.log("Stream closed.");
                        break;
                    }
        
                    if (value) 
                    {
                        // console.log("Chunk received, length:", value.length);
        
                        let temp_data = new Uint8Array(in_data.length + value.length);
                        temp_data.set(in_data);
                        temp_data.set(value, in_data.length);
                        in_data = temp_data;
        
                        while (in_data.length >= 50) 
                        {
                            // console.log("Packet received, length:", in_data.length);
        
                            let packet = in_data.slice(0, 50);  
                            process_in_packet(packet);
        
                            in_data = in_data.slice(50);
                        }
                    }
                }
            } 
            catch (error) 
            {
                console.error("Read error:", error);
                window.location.reload();
            }
        }
        
        read_serial_data();

        document.getElementById('profile-id').addEventListener('change', async () =>
        {
            const in_packet = create_packet_from_ui();
            in_packet.packet_id = PACKET_IDS.READ_PROFILE;
            console.log("Loading profile: ", in_packet.profile.id);
            await send_report(writer, in_packet);
        });

        document.getElementById('reload').addEventListener('click', async () =>
        {
            const in_packet = create_packet_from_ui();
            in_packet.packet_id = PACKET_IDS.READ_PROFILE;
            console.log("Loading profile: ", in_packet.profile.id);
            await send_report(writer, in_packet);
        });

        document.getElementById('save').addEventListener('click', async () =>
        {
            const in_packet = create_packet_from_ui();
            in_packet.packet_id = PACKET_IDS.WRITE_PROFILE;
            await send_report(writer, in_packet);
        });

        document.getElementById('disconnect').addEventListener('click', async () => 
        {
            await handle_disconnect(reader, writer, port);
        });

        reader.closed.then(async () => 
        {
            console.log("Device disconnected.");
            await handle_disconnect(reader, writer, port);
        });
    } 
    catch (error) 
    {
        console.error('Connection error:', error);
    }
}

async function handle_disconnect(reader, writer, port) 
{
    if (reader) reader.cancel();
    if (writer) writer.releaseLock();
    if (port) await port.close();

    console.log("Port closed. Reloading page...");
    window.location.reload();
}

document.getElementById('load-default').addEventListener('click', async () =>
{
    const in_packet = new_packet();
    in_packet.profile.id = parseInt(document.getElementById('profile-id').value);
    update_ui_elements(in_packet);
});

document.getElementById('connect').addEventListener('click', async () => 
{
    await connect_to_device();
});

document.getElementById('settings-ui').classList.add('hide');