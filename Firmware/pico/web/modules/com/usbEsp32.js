import { ESPLoader, Transport } from "https://unpkg.com/esptool-js@0.5.1/bundle.js";
import { UIProgram } from './../uiProgram.js';
import { UI } from './../uiSettings.js';

const REPO_OWNER = "wiredopposite";
const REPO_NAME = "OGX-Mini-WebApp";
const BAUDRATE = 115200;
const LOG_MAX_LEN = 100;
const LOG = document.getElementById("programLog");
const SERIAL_LIB = !navigator.serial && navigator.usb ? serial : navigator.serial;

const terminal = {
    clean() {
        LOG.innerHTML = "";
    },
    writeLine(data)  {
        this.write(data + "\n");
    },
    write(data) {
        LOG.innerHTML += data;
        if (LOG.textContent.split("\n").length > LOG_MAX_LEN + 1) {
            let logLines = LOG.innerHTML.replace(/(\n)/gm, "").split("<br>");
            LOG.innerHTML = logLines.splice(-LOG_MAX_LEN).join("<br>\n");
        }
        LOG.scrollTop = LOG.scrollHeight;
    },
};

async function getRepoContents(owner, repo, path = "", branch = "master") {
    const url = `https://api.github.com/repos/${owner}/${repo}/contents/${path}?ref=${branch}`;

    try {
        const response = await fetch(url);
        if (!response.ok) {
            throw new Error(`Error fetching repo content: ${response.status} ${response.statusText}`);
        }
        const data = await response.json();;
        return data;

    } catch (error) {
        console.error(error);
        return null;
    }
}

async function getFwFiles() {
    const contents = await getRepoContents(REPO_OWNER, REPO_NAME, "firmware/PICO_ESP32", "master");
    if (!contents) {
        return null;
    }

    const files = {
        uf2: contents.find(file => file.name.startsWith("OGX-Mini") && file.name.endsWith(".uf2")),
        bootloader: contents.find(file => file.name.startsWith("bootloader") && file.name.endsWith(".bin")),
        partitionTable: contents.find(file => file.name.startsWith("partition") && file.name.endsWith(".bin")),
        firmware: contents.find(file => file.name.startsWith("OGX-Mini") && file.name.endsWith(".bin")),
    };

    if (!files.uf2 || !files.bootloader || !files.partitionTable || !files.firmware) {
        console.error("Error: Firmware files not found.");
        return null;
    }

    return files;
}

async function copyUf2ToPico(dirHandle, uf2File) {
    if (!uf2File) {
        return false;
    }

    try {
        const response = await fetch(uf2File.path);
        if (!response.ok) {
            console.error(`Error fetching UF2: ${response.status} ${response.statusText}`);
            return false;
        }
        const uf2Content = await response.blob();
        const fileHandle = await dirHandle.getFileHandle(uf2File.name, { create: true });
        const writable = await fileHandle.createWritable();

        terminal.writeLine("Copying UF2 file...");

        await writable.write(uf2Content);
        await writable.close();

        terminal.writeLine("UF2 file copied.");

        return true;
    } catch (error) {
        console.error("Error: " + error.message);
    }
    return false;
} 

async function programEsp32(files) {
    terminal.writeLine("Requesting port...");

    let device = await SERIAL_LIB.requestPort({});
    let transport = new Transport(device, true);

    const loaderOptions = {
        transport: transport,
        baudrate: BAUDRATE,
        terminal: terminal,
        debugLogging: false,
    }

    terminal.writeLine("Connecting to ESP32...");

    let espLoader = new ESPLoader(loaderOptions);
    let reset_mode = "no_reset";
    let chip = await espLoader.main(reset_mode)

    terminal.writeLine("Connected to ESP32.");

    const fetchBinaryString = async (url) => {
        const response = await fetch(url);
        if (!response.ok) {
            throw new Error(`Failed to fetch firmware file from ${url}: ${response.status}`);
        }
        const arrayBuffer = await response.arrayBuffer();
        const uint8Array = new Uint8Array(arrayBuffer);
        let binaryString = "";
        for (let i = 0; i < uint8Array.length; i++) {
            binaryString += String.fromCharCode(uint8Array[i]);
        }
        return binaryString;
    };
    
    const bootloaderData = await fetchBinaryString(files.bootloader.download_url);
    const partitionTableData = await fetchBinaryString(files.partitionTable.download_url);
    const firmwareData = await fetchBinaryString(files.firmware.download_url);
    
    terminal.writeLine("Firmware files found:")
    terminal.writeLine(" - Bootloader: " + files.bootloader.name);
    terminal.writeLine(" - Partition Table: " + files.partitionTable.name);
    terminal.writeLine(" - Firmware: " + files.firmware.name);

    const flashOptions = {
        fileArray: [
            { data: bootloaderData, address: 0x1000 },
            { data: partitionTableData, address: 0x8000 },
            { data: firmwareData, address: 0x10000 },
        ],
        flashSize: "keep",
        eraseAll: false,
        compress: true,
        reportProgress: (fileIndex, write, total) => {
            // console.log(`Flashing ${fileIndex + 1} of 3: ${Math.round((write / total) * 100)}%`);
        },
        calculateMD5Hash: (image) => CryptoJS.MD5(CryptoJS.enc.Latin1.parse(image)),
    };

    terminal.writeLine("\nFlashing firmware...");

    await espLoader.writeFlash(flashOptions);

    const completeFlag = "PROGRAMMING_COMPLETE"
    const encoder = new TextEncoder();
    const flagBytes = encoder.encode(completeFlag);

    terminal.writeLine("Sending programming complete flag: " + completeFlag);

    try {
        await transport.write(flagBytes);
    } catch (error) {
        console.error("Error: " + error.message);
        return false;
    }

    await transport.disconnect();
    await transport.waitForUnlock();

    terminal.writeLine("Flashing complete.");
    return true;
}

export const UsbEsp32 = {

    async connect() {
        UIProgram.enableProgramEsp32Button(false);
        UIProgram.toggleConnected(true);
        UI.setSubheaderText("Update");

        try {
            let dirHandle = null;
            let fwFiles = null;

            UIProgram.addProgramPicoCallback(async () => {
                UIProgram.enableProgramPicoButton(false);
                try {
                    terminal.clean();
                    terminal.writeLine("Select the drive named RPI-RP2");

                    dirHandle = await window.showDirectoryPicker();
                    if (!dirHandle) {
                        return;
                    }

                    fwFiles = await getFwFiles();
                    if (!fwFiles) {
                        throw new Error("Error: Firmware files not found.");
                    }

                    if (!await copyUf2ToPico(dirHandle, fwFiles.uf2)) {
                        throw new Error("Error: UF2 not copied to Pico.");
                    }

                    UIProgram.enableProgramEsp32Button(true);

                    terminal.writeLine("Click 'Program Part 2' to finish programming you adapter.");

                } catch (error) {
                    console.warn("Error: " + error.message);
                    UIProgram.enableProgramPicoButton(true);
                }
            });

            UIProgram.addProgramEsp32Callback(async () => {
                UIProgram.enableProgramEsp32Button(false);
                if (!fwFiles) {
                    throw new Error("Error: Firmware files not found.");
                }

                try {
                    await programEsp32(fwFiles);
                } catch (error) {
                    console.warn("Error: " + error.message);
                    UIProgram.enableProgramEsp32Button(true);
                }
            });

        } catch (error) {
            console.error("Error: " + error.message);
        }
    }
};