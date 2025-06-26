export class Gamepad {
    static #REPORT_MAP = [
        { key: "dpad", size: 1 },
        { key: "buttons", size: 2 },
        { key: "triggerL", size: 1 },
        { key: "triggerR", size: 1 },
        { key: "joystickLx", size: 2 }, //int16_t
        { key: "joystickLy", size: 2 }, //int16_t
        { key: "joystickRx", size: 2 }, //int16_t
        { key: "joystickRy", size: 2 }, //int16_t
        { key: "analogUp", size: 1 },
        { key: "analogDown", size: 1 },
        { key: "analogLeft", size: 1 },
        { key: "analogRight", size: 1 },
        { key: "analogA", size: 1 },
        { key: "analogB", size: 1 },
        { key: "analogX", size: 1 },
        { key: "analogY", size: 1 },
        { key: "analogLb", size: 1 },
        { key: "analogRb", size: 1 },
    ];
    
    constructor() {
        this.report = {};
        for (const field of Gamepad.#REPORT_MAP) {
            this.report[field.key] = 0;
        }
    }

    setReportFromBytes(data) {
        if (!(data instanceof Uint8Array)) {
            return console.warn("Invalid data type.");
        } else if (data.length !== 23 && data.length !== 13) {
            return console.warn("Invalid data length: expected 23 bytes, actual:", data.length);
        }
    
        const dataView = new DataView(data.buffer);
    
        let offset = 0;
        for (const field of Gamepad.#REPORT_MAP) {
            const { key, size } = field;
    
            if (size === 1) {
                this.report[key] = data[offset];
            } else if (size === 2) {
                this.report[key] = dataView.getInt16(offset, true);
            }
    
            offset += size;
            if (offset > data.length) {
                break;
            }
        }
    }    

    scaledJoystickLx() {
        const value = this.report.joystickLx / 32767;
        return (value > 1) ? 1 : (value < -1) ? -1 : value;
    }
    scaledJoystickLy() {
        const value = this.report.joystickLy / 32767;
        return (value > 1) ? 1 : (value < -1) ? -1 : value;
    }
    scaledJoystickRx() {
        const value = this.report.joystickRx / 32767;
        return (value > 1) ? 1 : (value < -1) ? -1 : value;
    }
    scaledJoystickRy() {
        const value = this.report.joystickRy / 32767;
        return (value > 1) ? 1 : (value < -1) ? -1 : value;
    }
    scaledTriggerL() {
        const value = this.report.triggerL / 255;
        return (value > 1) ? 1 : value;
    }
    scaledTriggerR() {
        const value = this.report.triggerR / 255;
        return (value > 1) ? 1 : value;
    }
}