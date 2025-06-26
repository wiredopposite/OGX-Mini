import { DrawUtils } from "./../drawUtils.js";

export class JoystickVisualizer {
    #settings = null;
    #cursor = null;

    constructor(backgroundCanvas, settingsCanvas, cursorCanvas) {
        this.#settings = new Settings(settingsCanvas, backgroundCanvas);
        this.#cursor = new Cursor(cursorCanvas);
    }

    init(joystickSettings) {
        this.#settings.init(joystickSettings);
        this.#cursor.draw(0, 0);
    }

    drawSettings(joystickSettings) {
        this.#settings.draw(joystickSettings);
    }

    drawInput(joyX, joyY, settings = null, preview = false) {
        this.#cursor.draw(joyX, joyY, settings, preview);
    }
}

class Cursor {
    constructor(canvas) {
        this.canvas = canvas;
        this.context = canvas.getContext('2d');
        this.maxRadius = canvas.width / 2;
        this.centerX = canvas.width / 2;
        this.centerY = canvas.height / 2;

        this.colors = {
            cursor: 'rgba(0, 0, 255, 0.91)',
        }
        this.lastJoy = { x: -2, y: -2 };
    }

    draw(joyX, joyY, settings = null, preview = false) {
        const { canvas, context, centerX, centerY, maxRadius, colors, lastJoy } = this;

        if (preview && settings) {
            const scaled = applyJoystickSettings(
                joyX, joyY,
                settings.dzInner, settings.antiDzInnerC, 
                settings.antiDzInnerCYScale, settings.antiDzSquare, settings.antiDzSquareYScale,
                settings.antiDzOuter, settings.dzOuter, settings.curve,
                settings.axialRestrict * 100, settings.angularRestrict * 100, settings.antiDzAngular * 100,
                settings.diagonalScaleMin, settings.diagonalScaleMax, settings.uncapRadius,
                settings.invertY, settings.invertX
            );
            joyX = scaled.x;
            joyY = scaled.y;

            // console.log(`Preview: ${joyX}, ${joyY}`);
        }

        if (joyX !== lastJoy.x || joyY !== lastJoy.y) {
            lastJoy.x = joyX;
            lastJoy.y = joyY;

            context.clearRect(0, 0, canvas.width, canvas.height);
        
            let canvasJoyX = joyX * maxRadius;
            let canvasJoyY = joyY * maxRadius;
            
            canvasJoyX += centerX;
            canvasJoyY += centerY;
        
            context.beginPath();
            context.arc(canvasJoyX, canvasJoyY, 5, 0, Math.PI * 2);
            context.fillStyle = colors.cursor;
            context.fill();
            context.closePath();
        }
    }
}

class SettingsBackground {
    constructor(canvas) {
        this.canvas = canvas;
        this.context = canvas.getContext('2d');
        this.maxRadius = canvas.width / 2;
        this.centerX = canvas.width / 2;
        this.centerY = canvas.height / 2;

        this.colors = {
            background: 'rgb(249, 249, 249)',
            grid: 'rgba(0, 0, 0, 0.1)',
            gridMid: 'rgba(0, 0, 0, 0.7)',  
            strokeColor: 'rgba(0, 0, 0, 0.75)',
        };
        this.strokeWidth = {
            outer: 1,
            inner: 2,
        }
    }

    draw(joystickSettings) {
        const { canvas, context, centerX, centerY, maxRadius, colors } = this;
        const strokeWidth = this.strokeWidth;

        context.clearRect(0, 0, canvas.width, canvas.height);
    
        context.beginPath();
        context.fillStyle = colors.background;
        context.fill();
        context.closePath();
    
        const gridSpacing = canvas.width / 20;
        const mid = gridSpacing * 10;
    
        context.lineWidth = strokeWidth.inner;
    
        for (let x = 0; x < canvas.width; x += gridSpacing) {
            if (x !== mid) {
                context.strokeStyle = colors.grid;
            } else {
                context.strokeStyle = colors.mid;
            }
            context.beginPath();
            context.moveTo(x, 0);
            context.lineTo(x, canvas.height);
            context.stroke();
            context.closePath();
        }
    
        for (let y = 0; y < canvas.height; y += gridSpacing) {
            if (y !== mid) {
                context.strokeStyle = colors.grid;
            } else {
                context.strokeStyle = colors.mid;
            }
            context.beginPath();
            context.moveTo(0, y);
            context.lineTo(canvas.width, y);
            context.stroke();
            context.closePath();
        }

        DrawUtils.drawScaledDiagonalCircleEraseOuter(context, centerX, centerY, maxRadius, joystickSettings.diagonalScaleMax);
    }
}

class Settings {
    #background = null;

    constructor(settingsCanvas, backgroundCanvas) {
        this.#background = new SettingsBackground(backgroundCanvas); 
        this.canvas = settingsCanvas;
        this.context = this.canvas.getContext('2d');
        this.maxRadius = this.canvas.width / 2;
        this.centerX = this.canvas.width / 2;
        this.centerY = this.canvas.height / 2;

        this.colors = {
            stroke: 'rgba(0, 0, 0, 0.72)',
            antiDeadzone: 'rgba(0, 255, 0, 0.73)',
            antiSquareDeadzone: 'rgba(0, 102, 255, 0.82)',
            deadzone: 'rgba(255, 0, 0, 0.6)',
            angularAnti: 'rgba(97, 0, 97, 0.62)',
            angularRestrict: 'rgba(0, 0, 255, 0.5)',
            axialRestrict: 'rgba(228, 10, 180, 0.62)',
        }

        this.strokeWidth = 2;
        this.lastDiagonalScaleMax = 1;
    }

    init(joystickSettings) {
        this.#background.draw(joystickSettings);
        this.draw(joystickSettings);
    }

    draw(joystickSettings) {
        const { canvas, context, centerX, centerY, maxRadius, colors, strokeWidth } = this;

        context.clearRect(0, 0, canvas.width, canvas.height);
    
        const antiDzInnerCRadius = joystickSettings.antiDzInnerC * maxRadius;
        const antiDzSquareSize = canvas.width * joystickSettings.antiDzSquare;
        const dzInnerRadius = joystickSettings.dzInner * maxRadius;
        const antiDzOuterRadius = joystickSettings.antiDzOuter * maxRadius;
        const dzOuterRadius = joystickSettings.dzOuter * maxRadius;
        const axialRestrictWidth = joystickSettings.axialRestrict * canvas.width;
        const antiDzSquareYScaleHeight = canvas.height * joystickSettings.antiDzSquareYScale;
        // const axialRestrictWidth = joystickSettings.axialRestrict * 100;
        const angularRestrictAngle = joystickSettings.angularRestrict * 100;
        const antiDzAngular = joystickSettings.antiDzAngular * 100;
        const diagonalScaleMax = joystickSettings.diagonalScaleMax;
        const diagonalScaleMin = joystickSettings.diagonalScaleMin;

        if (antiDzSquareSize > 0) { 
            DrawUtils.drawScaledCross(
                context,
                centerX,
                centerY,
                maxRadius,
                antiDzSquareSize, 
                colors.antiSquareDeadzone
            );
        }
        if (antiDzSquareYScaleHeight > 0) {
            DrawUtils.drawRectangle(
                context,
                0,
                canvas.height / 2 - antiDzSquareYScaleHeight / 2,
                canvas.width,
                antiDzSquareYScaleHeight,
                colors.antiSquareDeadzone
            );
        }
        if (angularRestrictAngle > 0) {
            const angles = [45, 135, 225, 315];
            angles.forEach((angle) => {
                DrawUtils.drawAngularFlare(
                    context, 
                    centerX, 
                    centerY, 
                    dzOuterRadius, 
                    angle, 
                    angularRestrictAngle, 
                    colors.angularRestrict
                );
            });
        }
        if (antiDzAngular > 0) {
            const angles = [0, 90, 180, 270];
            angles.forEach((angle) => {
                DrawUtils.drawAngularFlare(
                    context, 
                    centerX, 
                    centerY, 
                    dzOuterRadius, 
                    angle, 
                    antiDzAngular, 
                    colors.angularAnti
                );
            });
        }
        if (joystickSettings.antiDzOuter < 1) {
            DrawUtils.drawScaledDiagonalCircleEraseInner(
                context, 
                centerX, 
                centerY, 
                antiDzOuterRadius, 
                diagonalScaleMax, 
                colors.antiDeadzone
            );
        }
        if (joystickSettings.dzOuter < 1) {
            DrawUtils.drawScaledDiagonalCircleEraseInner(
                context, 
                centerX, 
                centerY, 
                dzOuterRadius, 
                diagonalScaleMax, 
                colors.deadzone
            );
        }
        if (axialRestrictWidth > 0) {
            DrawUtils.drawCutoutCross(
                context, 
                centerX, 
                centerY, 
                maxRadius, 
                axialRestrictWidth, 
                colors.axialRestrict
            );
        }
        if (antiDzInnerCRadius > 0) {
            DrawUtils.drawScaledDiagonalCircle(
                context, 
                centerX, 
                centerY, 
                antiDzInnerCRadius, 
                diagonalScaleMin, 
                colors.antiDeadzone
            );
        }
        if (dzInnerRadius > 0) {
            // drawCircleCutoutInner(context, centerX, centerY, innerDeadzoneRadius);
            DrawUtils.drawCircle(context, centerX, centerY, dzInnerRadius, colors.deadzone);
        }

        if (diagonalScaleMax < 1.4) {
            DrawUtils.drawScaledDiagonalCircleEraseOuter(
                context, 
                centerX, 
                centerY, 
                maxRadius, 
                diagonalScaleMax, 
                null, 
                colors.stroke, 
                strokeWidth
            );
        } else {
            DrawUtils.drawRectangle(context, 0, 0, context.canvas.width,context.canvas.height, null, colors.stroke, strokeWidth * 2);
        }

        if (diagonalScaleMax != this.lastDiagonalScaleMax) {
            this.#background.draw(joystickSettings);
            this.lastDiagonalScaleMax = diagonalScaleMax;
        }
    }
}

function applyJoystickSettings(
    x, y,
    dzInner, antiDzInnerC, 
    antiDzInnerCYScale, antiDzSquare, antiDzSquareYScale,
    antiDzOuter, dzOuter, curve,
    axialRestrict, angularRestrict, antiDzAngular,
    diagonalScaleMin, diagonalScaleMax, uncapRadius,
    invertY, invertX
) {
    if (invertY) {
        y = -y;
    }
    if (invertX) {
        x = -x;
    }
    // Helper functions
    const rad2deg = (rad) => rad * (180 / Math.PI);
    const deg2rad = (deg) => deg * (Math.PI / 180);
    const clamp = (val, min, max) => Math.max(min, Math.min(max, val));
    const sq = (v) => v * v;
    const inv = (v) => (Math.abs(v) > 0.0001 ? 1.0 / v : 0.0);

	let rAngle = rad2deg(Math.abs(Math.atan(y/x)));
	if(Math.abs(x) < 0.0001){rAngle = 90.0;}

	//Setting up axial angle deadzone values
	let axialX, axialY;
	if(Math.abs(x) <= axialRestrict && rAngle > 45.0){axialX = 0.0;}
	else{ axialX = (Math.abs(x) - axialRestrict)/(1.0 - axialRestrict); }
	
	if(Math.abs(y) <= axialRestrict && rAngle <= 45.0){axialY = 0.0;}
	else{ axialY = (Math.abs(y) - axialRestrict)/(1.0 - axialRestrict); }
	
	let inputMagnitude  = Math.sqrt(x*x + y*y);
	let outputMagnitude;
	let angle           = rad2deg(Math.abs(Math.atan(axialY/axialX)));
	if(Math.abs(axialX) < 0.0001){angle = 90.0;}               //avoids angle change with Atan by avoiding dividing by 0

    //Checks if rectangular deadone is used.
	if(antiDzSquareYScale == 0.00){ antiDzSquareYScale = antiDzSquare; }
	
	//Ellipse warping
	if(antiDzInnerCYScale > 0.00 && antiDzInnerC > 0.00){
		antiDzInnerCYScale = antiDzInnerCYScale/antiDzInnerC;                         
		let ellipseAngle = Math.atan((1.0/antiDzInnerCYScale)*Math.tan(deg2rad(rAngle)));  //Find scaled ellipse angle
		if(ellipseAngle < 0.0){ellipseAngle = 1.570796;}                   //Capping range to avoid negative rollover
		let ellipseX = Math.cos(ellipseAngle);                                //use cos to find horizontal alligned value
		let ellipseY = Math.sqrt( sq(antiDzInnerCYScale)*(1.0 - sq(ellipseX)) );      //use ellipse to find vertical value
		antiDzInnerC = antiDzInnerC*Math.sqrt( sq(ellipseX) + sq(ellipseY));          //Find magnitude to scale antiCircleDeadzone
	}
	
	//Resizes circular antideadzone to output expected value(counters shrinkage when scaling for the square antideadzone below).
    if (antiDzInnerC > 0.0) {
        antiDzInnerC =   antiDzInnerC/( (antiDzInnerC*(1.0 - antiDzSquare/dzOuter) )/( antiDzInnerC*(1.0 - antiDzSquare) ) );
    }

	//Angular Restriction
	if(Math.abs(x) > axialRestrict && Math.abs(y) > axialRestrict){
		if((angle > 0.0) && (angle < angularRestrict/2.0)){angle = 0.00;}
		if(angle > 90.0 - angularRestrict/2.0 && angle < 90.0){angle = 90.0;}
		if((angle > angularRestrict/2.0) && (angle < (90.0 - angularRestrict/2.0))){
			angle = ((angle - angularRestrict/2.0)*(90.0))/((90.0 - angularRestrict/2.0) - angularRestrict/2.0);
		}
	}
	let refAngle = angle;
	if(refAngle < 0.001){refAngle = 0.0;}
	
	//Diagonal ScalePrep
	let diagonal = angle;
	if(diagonal > 45.0){
		diagonal = (((diagonal - 45.0)*(-45.0))/(45.0)) + 45.0;
	}
	
	//Angular Restriction Countering
	if(angle < 90.0 && angle > 0.0){
		angle = ((angle - 0.0)*((90.0 - (antiDzAngular/2.0)) - (antiDzAngular/2.0)))/(90.0) + (antiDzAngular/2.0);
	}
	
	//Flipping angles back to correct quadrant
	if(axialX < 0.0 && axialY > 0.0){angle = -angle;}
	if(axialX > 0.0 && axialY < 0.0){angle = angle - 180.0;}
	if(axialX < 0.0 && axialY < 0.0){angle = angle + 180.0;}
	
	//~~~~~~~~~~Deadzone wrap~~~~~~~~~~~~~~~~~~//
	if(inputMagnitude > dzInner){
		outputMagnitude = (inputMagnitude - dzInner)/(antiDzOuter - dzInner);

		//Circular antideadzone scaling
		outputMagnitude = ((Math.pow(outputMagnitude, (1.0/curve))*(dzOuter - antiDzInnerC) + antiDzInnerC));		

		//Capping max range
		if(outputMagnitude > dzOuter && !uncapRadius){
			outputMagnitude = dzOuter;
		}	
		
		//Diagonal Scale
		let dScale = (((outputMagnitude - antiDzInnerC)*(diagonalScaleMax - diagonalScaleMin))/(dzOuter - antiDzInnerC)) + diagonalScaleMin;		
		let cScale = (((diagonal - 0.0)*(1.0/Math.sqrt(2.0)))/(45.0));            //Both these lines scale the intensity of the warping
		cScale       = 1.0 - Math.sqrt(1.0 - cScale*cScale);                        //based on a circular curve to the perfect diagonal
		dScale       = (((cScale - 0.0)*(dScale - 1.0))/(.29289)) + 1.0;

		outputMagnitude = outputMagnitude*dScale;

		//Scaling values for square antideadzone
		let newX = Math.cos(deg2rad(angle))*outputMagnitude;
		let newY = Math.sin(deg2rad(angle))*outputMagnitude;
		
		//Magic angle wobble fix by user ME.
		// if (angle > 45.0 && angle < 225.0) {
		// 	newX = inv(Math.sin(deg2rad(angle - 90.0)))*outputMagnitude;
		// 	newY = inv(Math.cos(deg2rad(angle - 270.0)))*outputMagnitude;
		// }
		
		//Square antideadzone scaling
		let outputX = Math.abs(newX)*(1.0 - antiDzSquare/dzOuter) + antiDzSquare;
		if(x < 0.0){outputX = outputX*(-1.0);}
		if(refAngle == 90.0){outputX = 0.0;}
		
		let outputY = Math.abs(newY)*(1.0 - antiDzSquareYScale/dzOuter) + antiDzSquareYScale;
		if(y < -0.0){outputY = outputY*(-1.0);}
		if(refAngle == 0.0){outputY = 0.0;}

        return { x: clamp(outputX, -dzOuter, dzOuter),
                 y: clamp(outputY, -dzOuter, dzOuter) };
    } else {
        return { x: 0.0, y: 0.0 };
    }
}