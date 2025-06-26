import { JoystickSettings } from "./joystick/joystickSettings.js"; 
import { TriggerSettings } from "./trigger/triggerSettings.js";

export class UserSettings {
    static PROFILE_LENGTH = Object.freeze(190);
    static #FIX16_LENGTH = Object.freeze(4);

    static SIDES = Object.freeze(["left", "right"]);

    static PLAYER_IDX_OPTIONS = Object.freeze({
        key: "playerIdx", 
        def: 0, 
        fields: [
            { label: "Player 1", value: 0 },
            { label: "Player 2", value: 1 },
            { label: "Player 3", value: 2 },
            { label: "Player 4", value: 3 },
        ]
    });

    static DEVICE_MODE_OPTIONS = Object.freeze({
        key: "deviceMode", 
        def: 100, 
        fields: [
            { label: "Xbox OG",                  value: 1 },
            { label: "Xbox OG: Steel Battalion", value: 2 },
            { label: "Xbox OG: XRemote",         value: 3 },
            { label: "XInput",                   value: 4 },
            { label: "PS3",                      value: 5 },
            { label: "DInput",                   value: 6 },
            { label: "PS Classic",               value: 7 },
            { label: "Switch",                   value: 8 },
            { label: "WebApp",                   value: 100 },
        ]
    });

    static DEVICE_MODE_OPTIONS_MULTI_GP = Object.freeze({
        key: "deviceMode",
        def: 100,
        fields: [
            { label: "DInput", value: 6 },
            { label: "Switch", value: 8 },
            { label: "WebApp", value: 100 },
        ]
    });

    static DEVICE_MODE_OPTIONS_4CH = Object.freeze({
        key: "deviceMode",
        def: 100,
        fields: [
            { label: "Xbox OG",                  value: 1 },
            { label: "Xbox OG: Steel Battalion", value: 2 },
            { label: "Xbox OG: XRemote",         value: 3 },
            { label: "XInput",                   value: 4 },
            { label: "PS3",                      value: 5 },
            { label: "PS Classic",               value: 7 },
            { label: "WebApp",                   value: 100 },
        ]
    });

    static PROFILE_ID_OPTIONS = Object.freeze({ 
        key: "profile.profileId", 
        def: 1, 
        fields: [
            { label: "Profile 1", value: 1 },
            { label: "Profile 2", value: 2 },
            { label: "Profile 3", value: 3 },
            { label: "Profile 4", value: 4 },
            { label: "Profile 5", value: 5 },
            { label: "Profile 6", value: 6 },
            { label: "Profile 7", value: 7 },
            { label: "Profile 8", value: 8 },
        ]
    });

    static #PROFILE_SETTINGS = Object.freeze([
        { type: "int", key: "profileId",         size: 1, def: 1 },
        { type: "map", key: "joystickSettings-left",  size: 55, map: JoystickSettings.SETTINGS },
        { type: "map", key: "joystickSettings-right", size: 55, map: JoystickSettings.SETTINGS },
        { type: "map", key: "triggerSettings-left",  size: 20, map: TriggerSettings.SETTINGS },
        { type: "map", key: "triggerSettings-right", size: 20, map: TriggerSettings.SETTINGS },
        { type: "int", key: "dpadUp",            size: 1, def: 0x01,   label: "Up"    },
        { type: "int", key: "dpadDown",          size: 1, def: 0x02,   label: "Down"  },
        { type: "int", key: "dpadLeft",          size: 1, def: 0x04,   label: "Left"  },
        { type: "int", key: "dpadRight",         size: 1, def: 0x08,   label: "Right" },
        { type: "int", key: "buttonA",           size: 2, def: 0x0001, label: "A"     },
        { type: "int", key: "buttonB",           size: 2, def: 0x0002, label: "B"     },
        { type: "int", key: "buttonX",           size: 2, def: 0x0004, label: "X"     },
        { type: "int", key: "buttonY",           size: 2, def: 0x0008, label: "Y"     },
        { type: "int", key: "buttonL3",          size: 2, def: 0x0010, label: "L3"    },
        { type: "int", key: "buttonR3",          size: 2, def: 0x0020, label: "R3"    },
        { type: "int", key: "buttonBack",        size: 2, def: 0x0040, label: "Back"  },
        { type: "int", key: "buttonStart",       size: 2, def: 0x0080, label: "Start" },
        { type: "int", key: "buttonLb",          size: 2, def: 0x0100, label: "LB"    },
        { type: "int", key: "buttonRb",          size: 2, def: 0x0200, label: "RB"    },
        { type: "int", key: "buttonSys",         size: 2, def: 0x0400, label: "Sys"   },
        { type: "int", key: "buttonMisc",        size: 2, def: 0x0800, label: "Misc"  },
        { type: "bool",key: "analogEnabled",     size: 1, def: true },
        { type: "int", key: "analogOffUp",       size: 1, def: 0, label: "Up"    },
        { type: "int", key: "analogOffDown",     size: 1, def: 1, label: "Down"  },
        { type: "int", key: "analogOffLeft",     size: 1, def: 2, label: "Left"  },
        { type: "int", key: "analogOffRight",    size: 1, def: 3, label: "Right" },
        { type: "int", key: "analogOffA",        size: 1, def: 4, label: "A"     },
        { type: "int", key: "analogOffB",        size: 1, def: 5, label: "B"     },
        { type: "int", key: "analogOffX",        size: 1, def: 6, label: "X"     },
        { type: "int", key: "analogOffY",        size: 1, def: 7, label: "Y"     },
        { type: "int", key: "analogOffLb",       size: 1, def: 8, label: "LB"    },
        { type: "int", key: "analogOffRb",       size: 1, def: 9, label: "RB"    },
    ]);

    /*  Each joystickSettings-SIDE and triggerSettings-SIDE are referenced 
        by the UI, don't replace them with a new object after calling UI.init() */
    constructor() {
        this.profile = {};
        for (const field of UserSettings.#PROFILE_SETTINGS) {
            const { type, key, size, map } = field;

            if (type === "map") {
                if (key.toLowerCase().includes("joystick")) {
                    this.profile[key] = new JoystickSettings();
                } else if (key.toLowerCase().includes("trigger")) {
                    this.profile[key] = new TriggerSettings();
                }
            } else {
                this.profile[key] = field.def;
            }
        }

        this.playerIdx = UserSettings.PLAYER_IDX_OPTIONS.def;
        this.deviceMode = UserSettings.DEVICE_MODE_OPTIONS.def;
        this.maxGamepads = 1;
    }

    resetProfile() {
        UserSettings.#PROFILE_SETTINGS.forEach(field => {
            if (field.type !== "map" && field.key !== "profileId") {
                this.profile[field.key] = field.def;
            } else if (field.type === "map") {
                this.profile[field.key].resetAll();
            }
        });
    }

    getDeviceModeOptions(maxGamepads) {
        if (maxGamepads === 1) {
            return UserSettings.DEVICE_MODE_OPTIONS;
        } else if (maxGamepads === 4) {
            return UserSettings.DEVICE_MODE_OPTIONS_4CH;
        }
        return UserSettings.DEVICE_MODE_OPTIONS_MULTI_GP;
    }

    setProfileFromBytes(buffer) {
        if (!(buffer instanceof Uint8Array)) {
            throw new Error("Invalid buffer type.");
        } else if (buffer.length !== UserSettings.PROFILE_LENGTH) {
            throw new Error(`Invalid buffer length: expected ${UserSettings.PROFILE_LENGTH} bytes`);
        }
    
        let offset = 0;
    
        for (const field of UserSettings.#PROFILE_SETTINGS) {
            const { type, key, map, size } = field;
    
            if (type === "map") {
                map.forEach(subField => {
                    const { type: subType, key: subKey, def: def } = subField;
                    if (subType === "float") {
                        this.profile[key][subKey] = this.#fix16ToFloat(buffer.slice(offset, offset + UserSettings.#FIX16_LENGTH));
                        offset += UserSettings.#FIX16_LENGTH;
                    } else if (subType === "bool") {
                        this.profile[key][subKey] = (buffer[offset] > 0);
                        offset += 1;
                    }
                });

            } else if (type === "bool") {
                this.profile[key] = buffer[offset] > 0;
                offset += 1;

            } else if (type === "int") {
                if (size === 1) {
                    this.profile[key] = buffer[offset];
                    offset += 1;
                    
                } else if (size === 2) {
                    this.profile[key] = this.#uint16ToInt(buffer[offset], buffer[offset + 1]);
                    offset += 2;

                } else {
                    throw new Error(`Unsupported size for key: ${key}`);
                }

            } else {
                throw new Error(`Unsupported size for key: ${key}`);
            }
        }

        if (offset !== UserSettings.PROFILE_LENGTH) {
            throw new Error(`Invalid profile size: expected ${UserSettings.PROFILE_LENGTH}, got ${offset}`);
        }
    }   

    getProfileBytes() {
        let data = [];
    
        for (const field of UserSettings.#PROFILE_SETTINGS) {
            const { type, key, map, size } = field;
    
            if (type === "map") {
                for (const subField of map) {
                    const { type: subType, key: subKey } = subField;
    
                    if (subType === "float") {
                        data.push(...this.#floatToFix16(this.profile[key][subKey])); // fix16_t (int32_t)
                    } else if (subType === "bool") {
                        data.push(this.profile[key][subKey] ? 1 : 0); // uint8_t
                    }
                }
            } else {
                const value = this.profile[key];
    
                if (size === 1) {
                    data.push(value); // uint8_t
                } else if (size === 2) {
                    data.push(...this.#intToUint16(value)); // uint16_t
                } else {
                    throw new Error(`Unsupported size for key: ${key}`);
                }
            }
        }
    
        const bytes = new Uint8Array(data);
    
        if (bytes.length !== UserSettings.PROFILE_LENGTH) {
            throw new Error(`Invalid profile size: expected ${UserSettings.PROFILE_LENGTH}, got ${bytes.length}`);
        }
    
        return bytes;
    }        

    getDpadFields() {
        let dpadFields = [];
        UserSettings.#PROFILE_SETTINGS.forEach((field) => {
            if (field.key.toLowerCase().startsWith("dpad")) {
                dpadFields.push(field);
            }
        });
        return dpadFields;
    }

    getButtonFields() {
        let buttonFields = [];
        UserSettings.#PROFILE_SETTINGS.forEach((field) => {
            if (field.key.toLowerCase().startsWith("button")) {
                buttonFields.push(field);
            }
        });
        return buttonFields;
    }

    getAnalogFields() {
        let analogFields = [];
        UserSettings.#PROFILE_SETTINGS.forEach((field) => {
            if (field.key.toLowerCase().startsWith("analog") 
                && field.key !== "analogEnabled") {
                analogFields.push(field);
            }
        });
        return analogFields;
    }

    #uint16ToInt(byte0, byte1) {
        return byte0 | (byte1 << 8);
    }
    
    #intToUint16(value) {
        return [value & 0xff, value >> 8];
    }

    #floatToFix16(value) {
        let temp = value * 65536;
        temp += (temp >= 0) ? 0.5 : -0.5;
    
        const rawValue = temp | 0;
    
        const buffer = new ArrayBuffer(4);
        const view = new DataView(buffer);
        view.setInt32(0, rawValue, true);
    
        return new Uint8Array(buffer);
    }

    #fix16ToFloat(bytes) {
        if (!(bytes instanceof Uint8Array) || bytes.length !== 4) {
            throw new Error("fix16_t must be a 4-byte Uint8Array.");
        }
        const int32 = new Int32Array(bytes.buffer, bytes.byteOffset, bytes.byteLength / 4);
        const rawValue = int32[0];
        return rawValue / 0x00010000;
    }
}