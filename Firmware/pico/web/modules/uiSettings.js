import { JoystickSettings } from "./joystick/joystickSettings.js";
import { JoystickVisualizer } from "./joystick/joystickVisualizer.js";
import { TriggerSettings } from "./trigger/triggerSettings.js";
import { TriggerVisualizer } from "./trigger/triggerVisualizer.js";
import { UserSettings } from "./userSettings.js";

const SliderSnapThreshold = 0.02;

function uiCreateSlider(settings, visualizer, prefix, side, { key, label, min, max, def, tip }) {
    const wrapper = document.createElement("div");
    wrapper.classList.add("settingsSubPair");

    const elementId = `slider-${prefix}-${key}-${side}`;

    const labelElement = document.createElement("label");
    labelElement.setAttribute("for", elementId);
    labelElement.textContent = label;

    const slider = document.createElement("input");
    slider.type = "range";
    slider.id = elementId;
    slider.min = parseFloat(min) * 100;
    slider.max = parseFloat(max) * 100;
    slider.step = 1;
    slider.value = parseFloat(def) * 100;
    slider.title = tip;

    const valueDisplay = document.createElement("span");
    valueDisplay.id = `${elementId}-value`;
    valueDisplay.textContent = (parseFloat(def)).toFixed(2);

    slider.addEventListener("input", () => {
        let value = (slider.value / 100).toFixed(2);
        if (Math.abs(value - def) <= SliderSnapThreshold) {
            value = def;
        }
        valueDisplay.textContent = (value * 1).toFixed(2)
        slider.value = value * 100;
        settings[key] = parseFloat(value);
        visualizer.drawSettings(settings);
    });

    wrapper.appendChild(labelElement);
    wrapper.appendChild(slider);
    wrapper.appendChild(valueDisplay);

    return wrapper;
}

function uiCreateCheckbox(settings, visualizer, prefix, suffix, { key, label, def, tip }) {
    const wrapper = document.createElement("div");
    wrapper.classList.add("settingsSubPair");

    const elementId = `checkbox-${prefix}-${key}-${suffix}`;

    const labelElement = document.createElement("label");
    labelElement.setAttribute("for", elementId);
    labelElement.textContent = label;

    const checkbox = document.createElement("input");
    checkbox.type = "checkbox";
    checkbox.id = elementId;
    checkbox.checked = def;
    checkbox.title = tip;

    const valueDisplay = document.createElement("span");
    valueDisplay.id = `${elementId}-value`;
    valueDisplay.textContent = "";

    checkbox.addEventListener("change", () => {
        const isChecked = checkbox.checked;
        settings[key] = isChecked;
        visualizer.drawSettings(settings);
    });

    wrapper.appendChild(labelElement);
    wrapper.appendChild(checkbox);
    wrapper.appendChild(valueDisplay);

    return wrapper;
}

function uiCreateResetButton(settings, visualizer, fields, prefix, suffix) {
    const wrapper = document.createElement("div");
    const button = document.createElement("button");
    button.textContent = "Reset";

    button.addEventListener("click", () => {
        settings.resetAll();

        fields.forEach(field => {
            const elementId = field.type === "float"
                ? `slider-${prefix}-${field.key}-${suffix}`
                : `checkbox-${prefix}-${field.key}-${suffix}`;

            const element = document.getElementById(elementId);
            const valueDisplay = document.getElementById(`${elementId}-value`);

            if (element) {
                if (field.type === "float") {
                    element.value = field.def * 100;
                    if (valueDisplay) {
                        valueDisplay.textContent = field.def.toFixed(2);
                    }
                } else if (field.type === "bool") {
                    element.checked = field.def;
                }
            } else {
                console.warn(`Element not found: ${elementId}`);
            }
        });

        visualizer.drawSettings(settings);
    });

    wrapper.appendChild(button);

    return wrapper;
}

function uiGenerateJoystickSettings(settings, visualizer, side, containerPrefix) {
    const container = document.getElementById(`${containerPrefix}-${side}`);
    if (!container) {
        console.warn(`Container not found: ${containerPrefix}-${side}`);
        return;
    }

    const prefix = "joystick";

    JoystickSettings.SETTINGS.forEach(field => {
        let element;
        if (field.type === "float") {
            // const elementId = `slider-${prefix}-${setting.key}-${side}`;
            element = uiCreateSlider(settings, visualizer, prefix, side, field);
        } else {
            element = uiCreateCheckbox(settings, visualizer, prefix, side, field);
        }

        container.appendChild(element);
    });

    const resetButton = uiCreateResetButton(settings, visualizer, JoystickSettings.SETTINGS, prefix, side);
    container.appendChild(resetButton);
}

function uiGenerateTriggerSettings(settings, visualizer, suffix, containerPrefix) {
    const container = document.getElementById(`${containerPrefix}-${suffix}`);
    if (!container) {
        console.warn(`Container not found: ${containerPrefix}-${suffix}`);
        return;
    }

    const prefix = "trigger";

    TriggerSettings.SETTINGS.forEach(field => {
        let element;
        if (field.type === "float") {
            element = uiCreateSlider(settings, visualizer, prefix, suffix, field);
        } else if (field.type === "bool") {
            element = uiCreateCheckbox(settings, visualizer, prefix, suffix, field);
        }

        container.appendChild(element);
    });

    const resetButton = uiCreateResetButton(settings, visualizer, TriggerSettings.SETTINGS, prefix, suffix);
    container.appendChild(resetButton);
}

function uiCreateButtonDropdown(userSettings, field, fields) {
    const wrapper = document.createElement("div");
    wrapper.classList.add("dropdownSubPair");

    const labelElement = document.createElement("label");
    labelElement.setAttribute("for", `dropdown-${field.key}`);
    labelElement.textContent = field.label;

    const dropdown = document.createElement("select");
    dropdown.id = `dropdown-${field.key}`;

    fields.forEach((option) => {
        const optionElement = document.createElement("option");
        optionElement.value = option.def;
        optionElement.textContent = option.label;
        dropdown.appendChild(optionElement);

        if (option.def === userSettings.profile[field.key]) {
            optionElement.selected = true;
        }
    });

    dropdown.addEventListener("change", () => {
        const value = parseInt(dropdown.value, 10);
        userSettings.profile[field.key] = value;
    });

    wrapper.appendChild(labelElement);
    wrapper.appendChild(dropdown);
    return wrapper;
}

function uiGenerateButtonSettings(userSettings) {
    const dpadFields = userSettings.getDpadFields();
    const dpadContainerPrefix = "digitalDpadInput";
    let containerSuffix = "left";

    userSettings.getDpadFields().forEach(field => {
        const element = uiCreateButtonDropdown(userSettings, field, dpadFields);
        const container = document.getElementById(`${dpadContainerPrefix}-${containerSuffix}`);
        container.appendChild(element);
        if (containerSuffix === "left") {
            containerSuffix = "right";
        } else {
            containerSuffix = "left";
        }
    });

    const buttonFields = userSettings.getButtonFields();
    const buttonContainerPrefix = "digitalButtonInput";
    containerSuffix = "left";

    userSettings.getButtonFields().forEach(field => {
        const element = uiCreateButtonDropdown(userSettings, field, buttonFields);
        const container = document.getElementById(`${buttonContainerPrefix}-${containerSuffix}`);
        container.appendChild(element);
        if (containerSuffix === "left") {
            containerSuffix = "right";
        } else {
            containerSuffix = "left";
        }
    });

    const analogFields = userSettings.getAnalogFields();
    const analogContainerPrefix = "analogButtonsInput";
    containerSuffix = "left";

    userSettings.getAnalogFields().forEach(field => {
        const element = uiCreateButtonDropdown(userSettings, field, analogFields);
        const container = document.getElementById(`${analogContainerPrefix}-${containerSuffix}`);
        container.appendChild(element);
        if (containerSuffix === "left") {
            containerSuffix = "right";
        } else {
            containerSuffix = "left";
        }
    });
}

function uiSetupGeneralSettings(userSettings) {
    const elementProfileId = document.getElementById("dropdown-profileId");
    if (!elementProfileId) {
        console.warn("Profile dropdown not found.");
        return;
    }

    UserSettings.PROFILE_ID_OPTIONS.fields.forEach(field => {
        const optionElement = document.createElement("option");
        optionElement.value = field.value;
        optionElement.textContent = field.label;
        elementProfileId.appendChild(optionElement);

        if (field.value === userSettings.profile.profileId) {
            optionElement.selected = true;
        }
    });

    elementProfileId.addEventListener("change", () => {
        userSettings.profile.profileId = parseInt(elementProfileId.value, 10);
    });

    const elementPlayerIdx = document.getElementById("dropdown-playerIdx");
    if (!elementPlayerIdx) {
        console.warn("Player index dropdown not found.");
        return;
    }

    UserSettings.PLAYER_IDX_OPTIONS.fields.forEach(field => {
        const optionElement = document.createElement("option");
        optionElement.value = field.value;
        optionElement.textContent = field.label;
        elementPlayerIdx.appendChild(optionElement);

        if (field.value === userSettings.playerIdx) {
            optionElement.selected = true;
        }
    });

    if (userSettings.maxGamepads < 2) {
        elementPlayerIdx.disabled = true;
    }

    elementPlayerIdx.addEventListener("change", () => {
        userSettings.playerIdx = parseInt(elementPlayerIdx.value, 10);
    });

    const elementDeviceMode = document.getElementById("dropdown-deviceMode");
    if (!elementDeviceMode) {
        console.warn("Device mode dropdown not found.");
        return;
    }

    UserSettings.DEVICE_MODE_OPTIONS.fields.forEach(field => {
        const optionElement = document.createElement("option");
        optionElement.value = field.value;
        optionElement.textContent = field.label;
        elementDeviceMode.appendChild(optionElement);

        if (field.value === userSettings.deviceMode) {
            optionElement.selected = true;
        }
    });

    elementDeviceMode.addEventListener("change", () => {
        userSettings.deviceMode = parseInt(elementDeviceMode.value, 10);
    });

    const elementEnableAnalog = document.getElementById("checkbox-analogEnabled");
    if (!elementEnableAnalog) {
        console.warn("Analog button checkbox not found.");
        return;
    }
    elementEnableAnalog.checked = userSettings.profile.analogEnabled;
    elementEnableAnalog.addEventListener("change", () => {
        userSettings.profile.analogEnabled = elementEnableAnalog.checked;
    });
}

function uiSetupHidePanelListeners() {
    function togglePanel(panel, button) {
        panel.classList.toggle("hidden");
        if (panel.classList.contains("hidden")) {
            button.textContent = "Show";
        } else {
            button.textContent = "Hide";
        }
    }

    const axisPanelButton = document.getElementById("button-toggleAxisSettings");
    axisPanelButton.addEventListener("click", () => {
        const panel = document.getElementById("axisSettingsPanel");
        togglePanel(panel, axisPanelButton);
    });
    
    const digitalPanelButton = document.getElementById("button-toggleDigitalButtons");
    digitalPanelButton.addEventListener("click", () => {
        const panel = document.getElementById("digitalButtonsPanel")
        togglePanel(panel, digitalPanelButton);
    });

    const analogPanelButton = document.getElementById("button-toggleAnalogButtons");
    analogPanelButton.addEventListener("click", () => {
        const panel = document.getElementById("analogButtonsPanel");
        togglePanel(panel, analogPanelButton);
    });
}

function uiSetupAxisPreviewCheckboxes(userSettings) {
    const tip = "To get an accurate preview, reset the profile to defaults and save it before enabling this feature.";

    let elements = [];
    elements.push(document.getElementById("checkbox-previewJoy-left"));
    elements.push(document.getElementById("checkbox-previewJoy-right"));
    elements.push(document.getElementById("checkbox-previewTrigger-left"));
    elements.push(document.getElementById("checkbox-previewTrigger-right"));

    elements.forEach(element => {
        element.title = tip;
    });
}

export const UI = {

    Joysticks: new Map(),
    Triggers: new Map(),

    init(userSettings) {
        UserSettings.SIDES.forEach(side => {
            const canvasJoystickBackground = document.getElementById(`canvasJoystickBackground-${side}`);
            const canvasJoystickSettings = document.getElementById(`canvasJoystickSettings-${side}`);
            const canvasJoystickCursor = document.getElementById(`canvasJoystickCursor-${side}`);

            this.Joysticks.set(side, {
                visualizer: new JoystickVisualizer(
                    canvasJoystickBackground, 
                    canvasJoystickSettings, 
                    canvasJoystickCursor
                )
            });

            const joystickVisualizer = this.Joysticks.get(side).visualizer;
            const joystickSettings = userSettings.profile[`joystickSettings-${side}`];

            uiGenerateJoystickSettings(joystickSettings, joystickVisualizer, side, "joystickInput");
            joystickVisualizer.init(joystickSettings);

            const canvasTriggerBackground = document.getElementById(`canvasTriggerBackground-${side}`);
            const canvasTriggerSettings = document.getElementById(`canvasTriggerSettings-${side}`);
            const canvasTriggerCursor = document.getElementById(`canvasTriggerCursor-${side}`);

            this.Triggers.set(side, {
                visualizer: new TriggerVisualizer(
                    canvasTriggerBackground, 
                    canvasTriggerSettings, 
                    canvasTriggerCursor
                )
            });

            const triggerVisualizer = this.Triggers.get(side).visualizer;
            const triggerSettings = userSettings.profile[`triggerSettings-${side}`];

            uiGenerateTriggerSettings(triggerSettings, triggerVisualizer, side, "triggerInput");
            triggerVisualizer.init(triggerSettings);
        });

        uiSetupGeneralSettings(userSettings);
        uiGenerateButtonSettings(userSettings);
        uiSetupAxisPreviewCheckboxes(userSettings);
        uiSetupHidePanelListeners();

        const loadDefaultButton = document.getElementById("button-loadDefaults");
        if (loadDefaultButton) {
            loadDefaultButton.addEventListener("click", () => {
                userSettings.resetProfile();
                UI.updateAll(userSettings);
            });
        }
    },

    addCallbackSaveProfile(listenerFunc, userSettings) {
        const saveButton = document.getElementById("button-saveProfile");
        if (saveButton) {
            saveButton.addEventListener("click", () => listenerFunc());
        } else {
            console.warn("Save button not found.");
        }
    },

    addCallbackLoadProfile(listenerFunc, userSettings) {
        if (!userSettings) {
            console.error("User settings not provided.");
        }

        const reloadButton = document.getElementById("button-reloadProfile");
        if (reloadButton) {
            reloadButton.addEventListener("click", () => {
                const profileIdDropdown = document.getElementById("dropdown-profileId");
                if (profileIdDropdown) {
                    userSettings.profile.profileId = parseInt(profileIdDropdown.value, 10);
                    listenerFunc();
                } else {
                    console.warn("Profile ID dropdown not found.");
                }
            });
        } else {
            console.warn("Reload button not found.");
        }

        const profileIdDropdown = document.getElementById("dropdown-profileId");
        if (profileIdDropdown) {
            profileIdDropdown.addEventListener("change", () => {
                userSettings.profile.profileId = parseInt(profileIdDropdown.value, 10);
                listenerFunc();
            });
        } else {
            console.warn("Profile ID dropdown not found.");
        }
    },

    addCallbackDisconnect(listenerFunc) {
        const disconnectButton = document.getElementById("button-disconnect");
        if (disconnectButton) {
            disconnectButton.addEventListener("click", () => listenerFunc());
        } else {
            console.warn("Disconnect button not found.");
        }
    },

    toggleConnected(connected) {
        const settingsElement = document.getElementById("settingsPanel");
        const connectElement = document.getElementById("connectPanel");
        if (connected) {
            if (settingsElement.classList.contains("hidden")) {
                settingsElement.classList.remove("hidden");
                connectElement.classList.add("hidden");
                settingsElement.classList.add("show");
            }
        } else {
            if (connectElement.classList.contains("hidden")) {
                connectElement.classList.remove("hidden");
                settingsElement.classList.add("hidden");
                connectElement.classList.add("show");
            }
        }
    },

    updateAll(userSettings) {
        const elementProfileId = document.getElementById("dropdown-profileId");
        const elementPlayerIdx = document.getElementById("dropdown-playerIdx");
        const elementDeviceMode = document.getElementById("dropdown-deviceMode");
    
        if (elementProfileId) {
            elementProfileId.value = userSettings.profile.profileId;
        }
        if (elementPlayerIdx) {
            elementPlayerIdx.value = userSettings.playerIdx;
        }
        if (elementDeviceMode) {
            elementDeviceMode.value = userSettings.deviceMode;
        }
    
        UserSettings.SIDES.forEach(side => {
            const prefix = "joystick";
            const joystickSettings = userSettings.profile[`joystickSettings-${side}`];
            const visualizer = this.Joysticks.get(side).visualizer;
    
            JoystickSettings.SETTINGS.forEach(field => {
                const elementId = field.type === "float"
                    ? `slider-${prefix}-${field.key}-${side}`
                    : `checkbox-${prefix}-${field.key}-${side}`;
    
                const element = document.getElementById(elementId);
    
                if (element) {
                    const value = joystickSettings[field.key];
    
                    if (field.type === "float") {
                        element.value = value * 100;
                        const valueDisplay = document.getElementById(`${elementId}-value`);
                        if (valueDisplay) {
                            valueDisplay.textContent = value.toFixed(2);
                        }

                    } else if (field.type === "bool") {
                        element.checked = value;
                    }
                }
            });

            if (!visualizer) {
                console.warn("Visualizer not found for joystick.");
            }
    
            visualizer.drawSettings(joystickSettings);
        });
    
        UserSettings.SIDES.forEach(side => {
            const prefix = "trigger";
            const triggerSettings = userSettings.profile[`triggerSettings-${side}`];
    
            TriggerSettings.SETTINGS.forEach(field => {
                const elementId = field.type === "float" 
                    ? `slider-${prefix}-${field.key}-${side}` 
                    : `checkbox-${prefix}-${field.key}-${side}`;
    
                const element = document.getElementById(elementId);
    
                if (element) {
                    const value = triggerSettings[field.key];
    
                    if (field.type === "float") {
                        element.value = value * 100;
                        const valueDisplay = document.getElementById(`${elementId}-value`);
                        if (valueDisplay) {
                            valueDisplay.textContent = value.toFixed(2);
                        }
                    } else if (field.type === "bool") {
                        element.checked = value;
                    }
                }
            });
    
            this.Triggers.get(side).visualizer.drawSettings(triggerSettings);
        });
    
        const updateButtonDropdowns = (fields, prefix) => {
            fields.forEach(field => {
                const dropdown = document.getElementById(`dropdown-${field.key}`);
                if (dropdown) {
                    dropdown.value = userSettings.profile[field.key];
                }
            });
        };
    
        updateButtonDropdowns(userSettings.getDpadFields(), "digitalDpadInput");
        updateButtonDropdowns(userSettings.getButtonFields(), "digitalButtonInput");
        updateButtonDropdowns(userSettings.getAnalogFields(), "analogButtonsInput");
    
        const elementEnableAnalog = document.getElementById("checkbox-analogEnabled");
        if (elementEnableAnalog) {
            elementEnableAnalog.checked = userSettings.profile.analogEnabled;
        }
    },

    drawGamepadInput(gamepad, userSettings) {
        const joyVisualizerL = this.Joysticks.get("left").visualizer;
        const joyVisualizerR = this.Joysticks.get("right").visualizer;
        const trigVisualizerL = this.Triggers.get("left").visualizer;
        const trigVisualizerR = this.Triggers.get("right").visualizer;

        let joyLx = gamepad.scaledJoystickLx();
        let joyLy = gamepad.scaledJoystickLy();
        let joyRx = gamepad.scaledJoystickRx();
        let joyRy = gamepad.scaledJoystickRy();
        let triggerL = gamepad.scaledTriggerL();
        let triggerR = gamepad.scaledTriggerR();

        const prevJoyL = document.getElementById("checkbox-previewJoy-left").checked;
        const prevJoyR = document.getElementById("checkbox-previewJoy-right").checked;
        const prevTrigL = document.getElementById("checkbox-previewTrigger-left").checked;
        const prevTrigR = document.getElementById("checkbox-previewTrigger-right").checked;

        joyVisualizerL.drawInput(joyLx, joyLy, userSettings.profile[`joystickSettings-left`], prevJoyL);
        joyVisualizerR.drawInput(joyRx, joyRy, userSettings.profile[`joystickSettings-right`], prevJoyR);
        trigVisualizerL.drawInput(triggerL, userSettings.profile[`triggerSettings-left`], prevTrigL);
        trigVisualizerR.drawInput(triggerR, userSettings.profile[`triggerSettings-right`], prevTrigR);
    },

    connectButtonsEnabled(enabled) {
        const connectButtonUsb = document.getElementById("connectUsb");
        if (connectButtonUsb) {
            connectButtonUsb.disabled = !enabled;
        } else {
            console.warn("Connect button not found.");
        }
        const connectButtonBt = document.getElementById("connectBt");
        if (connectButtonBt) {
            connectButtonBt.disabled = !enabled;
        } else {
            console.warn("Connect button not found.");
        }
        const programButton = document.getElementById("connectOgxmW");
        if (programButton) {
            programButton.disabled = !enabled;
        } else {
            console.warn("Connect button not found.");
        }
    },

    setSubheaderText(text) {    
        const subheader = document.getElementById("subheader");
        if (subheader) {
            subheader.querySelector("h3").textContent = text;
        } else {
            console.warn("Subheader not found.");
        }
    },

    getSelectedProfileId() {
        const elementProfileId = document.getElementById("dropdown-profileId");
        if (elementProfileId) {
            return parseInt(elementProfileId.value, 10);
        } else {
            console.warn("Profile ID dropdown not found.");
            return -1;
        }
    },

    getSelectedPlayerIdx() {
        const elementPlayerIdx = document.getElementById("dropdown-playerIdx");
        if (elementPlayerIdx) {
            return parseInt(elementPlayerIdx.value, 10);
        } else {
            console.warn("Player index dropdown not found.");
            return -1;
        }
    },

    getSelectedDeviceMode() {
        const elementDeviceMode = document.getElementById("dropdown-deviceMode");
        if (elementDeviceMode) {
            return parseInt(elementDeviceMode.value, 10);
        } else {
            console.warn("Device mode dropdown not found.");
            return -1;
        }
    }
};