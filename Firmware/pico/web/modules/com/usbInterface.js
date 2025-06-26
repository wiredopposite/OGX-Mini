export class USBInterface {
    constructor() {
        this.reader = null;
        this.writer = null;
        this.port = null;
        this.inBuffer = new Uint8Array();
        this.disconnectCb = null;
    }

    async connect(baudrate, filters = null) {
        if (!("serial" in navigator)) {
            console.warn("Web Serial API not supported.");
            return false;
        }
        if (!filters) {
            this.port = await navigator.serial.requestPort({ });
        } else {
            this.port = await navigator.serial.requestPort({ filters });
        }
        
        await this.port.open({ baudRate: baudrate });
        this.reader = this.port.readable.getReader();
        this.writer = this.port.writable.getWriter();
        return true;
    }

    registerDisconnectCb(callback) {
        this.disconnectCb = callback;
    }

    async write(data) {
        if (!this.writer) {
            console.warn("Writer not initialized.");
            return;
        }
        return await this.writer.write(data);
    }

    async readTask(length, processCallback) {
        if (!this.reader) {
            console.warn("Reader not initialized.");
            return;
        }
        let inData = new Uint8Array();
        try {
            while (true) {
                const { value, done } = await this.reader.read();
                if (done) {
                    console.log("Stream closed.");
                    break;
                }
                if (value) {
                    let tempData = new Uint8Array(inData.length + value.length);
                    tempData.set(inData);
                    tempData.set(value, inData.length);
                    inData = tempData;

                    while (inData.length >= length) {
                        processCallback(inData.slice(0, length));
                        inData = inData.slice(length);
                    }
                }
            }
        } catch (error) {
            console.warn("Read error:", error.message || error);
            console.warn("Stack trace:", error.stack);
            await this.disconnect();
        }
    }

    async disconnect() {
        try {
            if (this.reader) await this.reader.cancel();
            if (this.writer) await this.writer.releaseLock();
            if (this.port) await this.port.close();
        } catch (error) {
            console.warn("Error disconnecting:", error);
        }
        this.reader = null;
        this.writer = null;
        this.port = null;
        if (this.disconnectCb) this.disconnectCb();
    }

    isConnected() {
        return this.port && this.port.readable && this.port.writable;
    }
}