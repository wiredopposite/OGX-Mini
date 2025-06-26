export class JoystickSettings {
    static SETTINGS = Object.freeze([
        { type: "float", key: 'dzInner',              min: 0.0, max: 0.5,  def: 0.0, label: "Deadzone Inner",           tip: "Value of desired circular deadzone." },
        { type: "float", key: 'dzOuter',              min: 0.6, max: 1.0,  def: 1.0, label: "Deadzone Outer",           tip: "Value of the maximum range you want to limit stick output." },
        { type: "float", key: 'antiDzInnerC',         min: 0.0, max: 0.5,  def: 0.0, label: "Anti-Deadzone Circular",   tip: "Value of the game's circular deadzone. Common sizes are 0.2 to 0.25." },
        { type: "float", key: 'antiDzInnerCYScale',   min: 0.0, max: 0.5,  def: 0.0, label: "Anti-Deadzone Circular Y", tip: "Only change if deadzone is ellipical.  When changed, antiCircleDeadzone will control width and this the height." },
        { type: "float", key: 'antiDzSquare',         min: 0.0, max: 0.5,  def: 0.0, label: "Anti-Deadzone Square",     tip: "Value of the game's square/axial deadzone. Common sizes are 0.2 to 0.25." },
        { type: "float", key: 'antiDzSquareYScale',   min: 0.0, max: 0.5,  def: 0.0, label: "Anti-Deadzone Square Y",   tip: "Only change if deadzone is rectangular.  When changed, antiSquareDeadzone will control the width and this the height." },
        { type: "float", key: 'antiDzAngular',        min: 0.0, max: 0.44, def: 0.0, label: "Anti-Deadzone Angular",    tip: "Use to counter reticted diagonal movement around the axes based on angle." }, 
        { type: "float", key: 'antiDzOuter',          min: 0.5, max: 1.0,  def: 1.0, label: "Anti-Deadzone Outer",      tip: "Once reached, stick outputs 100%. Useful if user's stick is unable to reach maximum magnitudes." },
        { type: "float", key: 'axialRestrict',        min: 0.0, max: 0.49, def: 0.0, label: "Axial Restrict",           tip: "Restrict diagonal movement based on distance from the axis." }, 
        { type: "float", key: 'angularRestrict',      min: 0.0, max: 0.44, def: 0.0, label: "Angular Restrict",         tip: "Restrict diagonal movement around axis based on angle." }, 
        { type: "float", key: 'diagonalScaleMin',     min: 0.5, max: 1.42, def: 1.0, label: "Diagonal Scale Inner",     tip: "Use to warp lower magnitude diagonal values." }, 
        { type: "float", key: 'diagonalScaleMax',     min: 0.5, max: 1.42, def: 1.0, label: "Diagonal Scale Outer",     tip: "Use to warp higher magnitude diagonal values." }, 
        { type: "float", key: 'curve',                min: 0.3, max: 3.0,  def: 1.0, label: "Curve",                    tip: "Value of the game's curve to cancel it into a linear curve.  Larger values result in faster starting movement." }, 
        { type: "bool",  key: 'uncapRadius',          def: true,  label: "Uncap Radius", tip: "Uncap joystick position so it can move beyond the circular radius." }, 
        { type: "bool",  key: 'invertY',              def: false, label: "Invert Y",     tip: "Invert Y axis." }, 
        { type: "bool",  key: 'invertX',              def: false, label: "Invert X",     tip: "Invert X axis." }, 
    ]);

    constructor() {
        this.resetAll();;
    }

    resetAll() {
        JoystickSettings.SETTINGS.forEach((setting) => {
            this[setting.key] = setting.def;
        });
    }

    getDefaultValue(key) {
        const intSetting = JoystickSettings.SETTINGS.find(s => s.key === key);
        if (intSetting !== undefined) {
            return intSetting.def;
        }
        console.warn(`Invalid key: ${key}`);
        return null;
    }
};