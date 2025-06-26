import { DrawUtils } from "./../drawUtils.js";

export class TriggerVisualizer {
    #settings = null;
    #input = null;

    constructor(backgroundCanvas, settingsCanvas, inputCanvas) {
        this.#settings = new Settings(settingsCanvas, backgroundCanvas);
        this.#input = new Input(inputCanvas);
    }

    init(triggerSettings) {
        this.#settings.init(triggerSettings);
        this.#input.draw(-1);
    }

    drawSettings(triggerSettings) {
        this.#settings.draw(triggerSettings);
    }

    drawInput(value, settings = null, preview = false) {
        this.#input.draw(value, settings, preview);
    }
}

class Input {
    constructor (canvas) {
        this.canvas = canvas;
        this.context = canvas.getContext('2d');

        this.colors = {
            inputBar: 'rgba(44, 44, 255, 0.91)',
            stroke: 'rgba(0, 0, 0, 0.72)',
        }
        this.strokeWidth = 4;
        this.barWidth = 4;
        this.lastValue = -2;
    }

    draw(value, settings, preview) {
        let { canvas, context, colors, lastValue, barWidth, strokeWidth } = this;

        if (settings && preview) {
            value = applySettings(value, settings.dzInner, settings.dzOuter, settings.antiDzInner, settings.antiDzOuter, settings.curve);
        }

        if (value !== lastValue) {
            lastValue = value;

            const width = canvas.width - barWidth;

            context.clearRect(0, 0, canvas.width, canvas.height);
            const x = value * width;
            const y = 2;

            DrawUtils.drawRectangle(context, x, 0, barWidth, canvas.height, colors.inputBar);
        }

        DrawUtils.drawRectangle(context, 0, 0, canvas.width, canvas.height, null, colors.stroke, strokeWidth);
    }
}

class Background {
    constructor(canvas) {
        this.canvas = canvas;
        this.context = canvas.getContext('2d');
        this.colors = {
            background: 'rgb(249, 249, 249)',
            stroke: 'rgba(0, 0, 0, 0.32)',
        }
        this.strokeWidth = 1
    }

    draw(triggersettings) {
        const { canvas, context, colors, strokeWidth } = this;

        context.clearRect(0, 0, canvas.width, canvas.height);

        context.beginPath();
        context.fillStyle = colors.background;
        context.fill();
        context.closePath();

        const gridSpacing = canvas.width / 10;
        context.lineWidth = strokeWidth;
        context.strokeStyle = colors.stroke;

        for (let x = 0; x < canvas.width; x += gridSpacing) {
            context.beginPath();
            context.moveTo(x, 0);
            context.lineTo(x, canvas.height);
            context.stroke();
            context.closePath();
        }
    }
}

class Settings {
    #background = null;

    constructor(settingsCanvas, backgroundCanvas) {
        this.#background = new Background(backgroundCanvas); 
        this.canvas = settingsCanvas;
        this.context = this.canvas.getContext('2d');
        this.colors = {
            stroke: 'rgba(0, 0, 0, 0.72)',
            antiDeadzone: 'rgba(0, 255, 0, 0.73)',
            deadzone: 'rgba(255, 0, 0, 0.6)',
        }
        this.strokeWidth = 4;
    }

    init(triggerSettings) {
        this.#background.draw(triggerSettings);
        this.draw(triggerSettings);
    }

    draw(triggerSettings) {
        const { canvas, context, background, colors, strokeWidth } = this;

        const { dzInner, dzOuter, antiDzInner, antiDzOuter } = triggerSettings;
        const dzInnerWidth = dzInner * canvas.width;
        const dzOuterWidth = (1 - dzOuter) * canvas.width;
        const antiDzInnerWidth = antiDzInner * canvas.width;
        const antiDzOuterWidth = (1 - antiDzOuter) * canvas.width;

        context.clearRect(0, 0, canvas.width, canvas.height);

        if (antiDzInnerWidth) {
            DrawUtils.drawRectangle(context, 0, 0, antiDzInnerWidth, canvas.height, colors.antiDeadzone);
        }
        if (antiDzOuterWidth) {
            DrawUtils.drawRectangle(context, canvas.width - antiDzOuterWidth, 0, antiDzOuterWidth, canvas.height, colors.antiDeadzone);
        }
        if (dzInnerWidth) {
            DrawUtils.drawRectangle(context, 0, 0, dzInnerWidth, canvas.height, colors.deadzone);
        }
        if (dzOuterWidth) {
            DrawUtils.drawRectangle(context, canvas.width - dzOuterWidth, 0, dzOuterWidth, canvas.height, colors.deadzone);
        }
    }
}

function applySettings(value, dzInner, dzOuter, antiDzInner, antiDzOuter, curve) {
    const clamp = (v, min, max) => Math.max(min, Math.min(max, v));

    const absValue = Math.abs(value);
    if (absValue < dzInner) {
        return 0.0;
    }

    let normalizedValue = (absValue - dzInner) / (dzOuter - dzInner);
    normalizedValue = clamp(normalizedValue, 0.0, 1.0);

    if (antiDzInner > 0.0) {
        normalizedValue = antiDzInner + (1.0 - antiDzInner) * normalizedValue;
    }

    if (curve !== 1.0) {
        normalizedValue = Math.pow(normalizedValue, 1.0 / curve);
    }

    if (antiDzOuter < 1.0) {
        const scaleFactor = 1.0 / (1.0 - antiDzOuter);
        normalizedValue = clamp(normalizedValue * scaleFactor, 0.0, 1.0);
    }

    normalizedValue *= dzOuter;

    return normalizedValue;
}