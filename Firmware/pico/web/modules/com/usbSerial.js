import { USBInterface } from "./usbInterface.js";
import { Gamepad } from "../gamepad.js";
import { UI } from "../uiSettings.js";
import { UserSettings } from "../userSettings.js";

class USBManager {
    static #PACKET_LENGTH = Object.freeze(64);
    static #HEADER_LENGTH = Object.freeze(9);
    static #BAUDRATE = Object.freeze(9600); 
    static #BUFFER_LEN = Object.freeze(1024);

    static #PACKET_ID = Object.freeze({
        NONE: 0,
        GET_PROFILE_BY_ID: 0x50,
        GET_PROFILE_BY_IDX: 0x55,
        SET_PROFILE_START: 0x60,
        SET_PROFILE: 0x61,
        SET_GP_IN: 0x80,
        SET_GP_OUT: 0x81,
        RESP_ERROR: 0xFF
    });

    static #PACKET_HEADER = Object.freeze([
        { key: "packetLen",   size: 1 },
        { key: "packetId",    size: 1 },
        { key: "deviceMode",  size: 1 },
        { key: "maxGamepads", size: 1 },
        { key: "playerIdx",   size: 1 },
        { key: "profileId",   size: 1 },
        { key: "chunksTotal", size: 1 },
        { key: "chunkIdx",    size: 1 },
        { key: "chunkLen",    size: 1 },
    ]);

    #interface = null;
    #currentBufferInOffset = 0;
    #bufferIn = null;
    #userSettings = null;

    constructor() {
        this.#interface = new USBInterface();
        this.#bufferIn = new Uint8Array(USBManager.#BUFFER_LEN);
    }

    async init(userSettings) {
        try {
            this.#userSettings = userSettings;

            if (await this.#interface.connect(USBManager.#BAUDRATE)) {
                this.#interface.registerDisconnectCb(() => {
                    window.location.reload();
                });

                this.#interface.readTask(USBManager.#PACKET_LENGTH, this.#processPacketIn.bind(this));
                await this.#sleep(1000);
                return true;
            }
        } catch (error) {
            console.warn('Connection error:', error);
            await this.#interface.disconnect();
            return false;
        }
    }

    async saveProfile() {
        let header = this.#headerFromUi(USBManager.#PACKET_ID.SET_PROFILE_START);
        await this.#writeToDevice(
            header, 
            new Uint8Array([0xFF])
        );
        await this.#sleep(100);

        const data = this.#userSettings.getProfileBytes();
        header.packetId = USBManager.#PACKET_ID.SET_PROFILE;
        await this.#writeToDevice(
            header, 
            data
        );
    }

    async getProfileById() {
        let header = this.#headerFromUi(USBManager.#PACKET_ID.GET_PROFILE_BY_ID);
        await this.#writeToDevice(
            header, 
            new Uint8Array([0xFF])
        );
    }

    async getProfileByIdx() {
        let header = this.#headerFromUi(USBManager.#PACKET_ID.GET_PROFILE_BY_IDX);
        await this.#writeToDevice(
            header, 
            new Uint8Array([0xFF])
        );
    }

    async disconnect() {
        await this.#interface.disconnect();
    }

    #headerFromUi(packetId) {
        return {
            packetLen: USBManager.#PACKET_LENGTH,
            packetId: packetId,
            deviceMode: UI.getSelectedDeviceMode(),
            maxGamepads: this.#userSettings.maxGamepads,
            playerIdx: UI.getSelectedPlayerIdx(),
            profileId: UI.getSelectedProfileId(),
            chunksTotal: 1,
            chunkIdx: 0,
            chunkLen: 0
        }
    }

    #deserializeHeader(packetData) {
        if (packetData.length < USBManager.#HEADER_LENGTH) {
            console.error("Invalid packet data length.");
            return;
        }
        const header = {};
        let offset = 0;
        USBManager.#PACKET_HEADER.forEach(field => {
            header[field.key] = packetData[offset];
            offset += field.size;
        });
        return header;
    }

    #serializeHeader(header) {
        const buffer = new Uint8Array(USBManager.#HEADER_LENGTH);
        let offset = 0;
        USBManager.#PACKET_HEADER.forEach(field => {
            buffer[offset] = header[field.key];
            offset += field.size;
        });
        return buffer;
    }

    #processPacketInData(header, bufferIn, dataLen) {
        switch (header.packetId) {
            case USBManager.#PACKET_ID.GET_PROFILE_BY_IDX:   
                console.log("Received profile data.");
                this.#userSettings.setProfileFromBytes(bufferIn.subarray(0, dataLen));
                this.#userSettings.maxGamepads = header.maxGamepads;
                this.#userSettings.playerIdx = header.playerIdx;
                this.#userSettings.deviceMode = header.deviceMode;
                UI.updateAll(this.#userSettings);
                break;

            case USBManager.#PACKET_ID.GET_PROFILE_BY_ID:
                console.log("Received profile data.");
                this.#userSettings.setProfileFromBytes(bufferIn.subarray(0, dataLen));
                this.#userSettings.maxGamepads = header.maxGamepads;
                this.#userSettings.deviceMode = header.deviceMode;
                UI.updateAll(this.#userSettings);
                break; 

            case USBManager.#PACKET_ID.SET_GP_IN:
                const gamepad = new Gamepad();
                gamepad.setReportFromBytes(bufferIn.subarray(0, dataLen));
                UI.drawGamepadInput(gamepad, this.#userSettings);
                break;

            default:
                console.warn(`Unknown packet ID: ${header.packetId}`);
                break;
        }
    }

    #processPacketIn(data) {
        if (data[0] !== USBManager.#PACKET_LENGTH) {
            console.warn(`Invalid packet length: ${data[0]}`);
            return;
        }
        const header = this.#deserializeHeader(data);

        this.#bufferIn.set(
            data.subarray(
                USBManager.#HEADER_LENGTH, 
                USBManager.#HEADER_LENGTH + header.chunkLen
            ), this.#currentBufferInOffset
        );

        this.#currentBufferInOffset += header.chunkLen;

        console.log("Received packet: " + (header.chunkIdx + 1) + " of " + header.chunksTotal);    

        if (header.chunkIdx + 1 === header.chunksTotal) {
            this.#processPacketInData(header, this.#bufferIn, this.#currentBufferInOffset);
            this.#currentBufferInOffset = 0;
        }
    }

    async #writeToDevice(header, data) {
        const dataLen = data.length;
        const lenLimit = USBManager.#PACKET_LENGTH - USBManager.#HEADER_LENGTH;
        const chunksTotal = Math.ceil(dataLen / lenLimit);;
        let currentOffset = 0;

        header.chunksTotal = chunksTotal;

        for (let i = 0; i < chunksTotal; i++) {
            const isLastChunk = (i === chunksTotal - 1);
            const chunkLen = isLastChunk ? dataLen - currentOffset : lenLimit;
            header.chunkIdx = i;
            header.chunkLen = chunkLen;

            const buffer = new Uint8Array(USBManager.#PACKET_LENGTH);

            buffer.set(this.#serializeHeader(header), 0);
            buffer.set(
                data.subarray(currentOffset, currentOffset + chunkLen), 
                USBManager.#HEADER_LENGTH
            );

            console.log("Writing packet: " + (i + 1) + " of " + chunksTotal);

            await this.#interface.write(buffer);
            currentOffset += chunkLen;
        }
    }

    async #sleep(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
}

export const USB = {

    async connect() {
        if (!("serial" in navigator)) {
            console.error("Web Serial API not supported.");
            return;
        }

        const userSettings = new UserSettings();
        UI.init(userSettings);
        const usbManager = new USBManager();

        try {
            UI.connectButtonsEnabled(false);

            if (!(await usbManager.init(userSettings))) {
                throw new Error("Connection failed.");
            }

            await usbManager.getProfileByIdx();

            UI.updateAll(userSettings);
            UI.toggleConnected(true);
            UI.setSubheaderText("Settings");

            UI.addCallbackLoadProfile(async () => {
                await usbManager.getProfileById();
                UI.updateAll(userSettings);
            }, userSettings);

            UI.addCallbackSaveProfile(async () => {
                await usbManager.saveProfile();
            }, userSettings);

            UI.addCallbackDisconnect(async () => {
                await usbManager.disconnect();
                UI.toggleConnected(false);
                window.location.reload();
            });

        } catch (error) {
            console.warn('Connection error:', error);
            usbManager.disconnect();
            window.location.reload();
        }
    }
};