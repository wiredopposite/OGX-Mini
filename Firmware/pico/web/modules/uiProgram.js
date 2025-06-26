export const UIProgram = {

    addProgramPicoCallback(callback) {
        const buttonProgramPico = document.getElementById("button-programPico");
        buttonProgramPico.addEventListener("click", async () => {
            await callback();
        });
    },

    addProgramEsp32Callback(callback) {  
        const buttonProgramEsp32 = document.getElementById("button-programEsp32");
        buttonProgramEsp32.addEventListener("click", async () => {
            await callback();
        });
    },

    enableProgramPicoButton(enabled) {
        const buttonProgramPico = document.getElementById("button-programPico");
        buttonProgramPico.disabled = !enabled;
    },

    enableProgramEsp32Button(enabled) {
        const buttonProgramEsp32 = document.getElementById("button-programEsp32");
        buttonProgramEsp32.disabled = !enabled;
    },

    toggleConnected(connected) {
        const connectPanel = document.getElementById("connectPanel");
        const programPanel = document.getElementById("programPanel");

        if (connected) {
            if (programPanel.classList.contains("hidden")) {
                programPanel.classList.remove("hidden");
                connectPanel.classList.add("hidden");
                programPanel.classList.add("show");
            }
        } else {
            if (connectPanel.classList.contains("hidden")) {
                connectPanel.classList.remove("hidden");
                programPanel.classList.add("hidden");
                connectPanel.classList.add("show");
            }
        }
    }

};