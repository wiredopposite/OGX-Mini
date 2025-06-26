export class BTInterface {
    #device = null;
    #server = null;
    #service = null;
    #connected = false;

    async connect(uuid, options) {
        if (!navigator.bluetooth) {
            console.error("Web Bluetooth API is not available, try using a different browser.");
            return false;
        }

        try {
            this.#device = await navigator.bluetooth.requestDevice(options);
            this.#device.addEventListener('gattserverdisconnected', () => {
                this.disconnect();
            });

            this.#server = await this.#device.gatt.connect();
            this.#service = await this.#server.getPrimaryService(uuid);
            this.#connected = true;
        } catch (error) {
            console.error('Bluetooth connection failed:', error);
            this.disconnect();
        }
        return this.#connected;
    }

    setDisconnectCb(cb) {
        if (this.#device) {
            this.#device.addEventListener('gattserverdisconnected', cb);
        }
    }

    async disconnect() {
        if (this.#device) {
            await this.#device.gatt.disconnect();
            this.#device = null;
        }
        this.#server = null;
        this.#service = null;
        this.#connected = false;
    }

    isConnected() {
        return this.#connected && this.#device?.gatt?.connected && this.#server && this.#service;
    }
    
    async write(uuid, data) {
        if (!this.isConnected()) {
            console.error('Bluetooth device not connected.');
            return;
        }
        try {
            const characteristic = await this.#service.getCharacteristic(uuid);
            await characteristic.writeValue(data);
            return true;
        } catch (error) {
            console.error('Failed to write to Bluetooth device:', error);
        }
        return false;
    }

    async read(uuid) {
        if (!this.isConnected()) {
            console.error('Bluetooth device not connected.');
            return null;
        }
        try {
            const characteristic = await this.#service.getCharacteristic(uuid);
            const value = await characteristic.readValue();
            if (value) {
                return new Uint8Array(value.buffer);
            }
        } catch (error) {
            console.error('Failed to read from Bluetooth device:', error);
        }
        return null;
    }
}