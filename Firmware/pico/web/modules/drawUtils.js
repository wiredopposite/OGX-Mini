export const DrawUtils = {
    drawRectangle(context, x, y, width, height, fillColor = null, strokeColor = null, strokeWidth = 1) {
        context.beginPath();
        context.rect(x, y, width, height);
        if (fillColor) {
            context.fillStyle = fillColor;
            context.fill();
        }
        if (strokeColor) {
            context.strokeStyle = strokeColor;
            context.lineWidth = strokeWidth;
            context.stroke();
        }
        context.closePath();
    },
    
    drawAngularFlare(context, centerX, centerY, radius, angle, flareAngle, color) {
        function deg2rad(degrees) {
            return (degrees * Math.PI) / 180;
        }

        const startAngle = deg2rad(angle - flareAngle / 2);
        const endAngle = deg2rad(angle + flareAngle / 2);

        context.beginPath();
        context.moveTo(centerX, centerY);
        context.arc(centerX, centerY, radius, startAngle, endAngle);
        context.closePath();
        context.fillStyle = color;
        context.fill();
    },

    drawCircleCutoutOuter(context, centerX, centerY, radius) {
        context.save();
        context.beginPath();
        context.rect(0, 0, context.canvas.width, context.canvas.height);
        context.arc(centerX, centerY, radius, 0, Math.PI * 2, true);
        context.clip('evenodd');
        context.clearRect(0, 0, context.canvas.width, context.canvas.height);
        context.restore();
    },

    drawCircleCutoutInner(context, centerX, centerY, radius, color) {
        context.beginPath();
        context.rect(0, 0, context.canvas.width, context.canvas.height);
        context.arc(centerX, centerY, radius, 0, Math.PI * 2, true);
        context.fillStyle = color;
        context.fill('evenodd');
        context.closePath();
    },

    drawCircle(context, centerX, centerY, radius, fillColor = null, strokeColor = null, strokeWidth = 1) {
        context.beginPath();
        context.arc(centerX, centerY, radius, 0, Math.PI * 2);
        if (fillColor) {
            context.fillStyle = fillColor;
            context.fill();
        }
        if (strokeColor) {
            context.strokeStyle = strokeColor;
            context.lineWidth = strokeWidth;
            context.stroke();
        }
        context.closePath();
    },

    drawScaledDiagonalCircleRaw(context, centerX, centerY, radius, scale, indentScale = 0) {
        if (scale > 1.4) {
            scale = 1.4;
        }

        if (indentScale > 0) {
            indentScale = Math.max(0, Math.min(1, indentScale));
        } else {
            indentScale = 1.0;
        }

        for (let angle = 0; angle <= 360; angle += 1) {
            const rad = (Math.PI / 180) * angle;

            let x = Math.cos(rad);
            let y = Math.sin(rad);

            let angleDeg = Math.abs(rad * (180 / Math.PI)) % 90;
            let diagonal = angleDeg > 45 ? (((angleDeg - 45) * -45) / 45) + 45 : angleDeg;

            let cScale = (diagonal * (1.0 / Math.SQRT2)) / 45.0;
            cScale = 1.0 - Math.sqrt(1.0 - Math.pow(cScale, 2));

            let dScale = ((cScale * ((scale - 1.0)) * indentScale) / 0.29289) + 1.0;
            // dScale = 1 + (dScale - 1) * indentScale;

            x *= dScale;
            y *= dScale;

            const finalX = centerX + x * radius;
            const finalY = centerY + y * radius;

            if (angle === 0) {
                context.moveTo(finalX, finalY);
            } else {
                context.lineTo(finalX, finalY);
            }
        }
    },

    drawScaledDiagonalCircle(context, centerX, centerY, radius, scale, fillColor = null, strokeColor = null, strokeWidth = 1, indentScale = 0) {
        context.beginPath();
        this.drawScaledDiagonalCircleRaw(context, centerX, centerY, radius, scale, indentScale);
        if (fillColor) {
            context.fillStyle = fillColor;
            context.fill();
        }
        if (strokeColor) {
            context.strokeStyle = strokeColor;
            context.lineWidth = strokeWidth;
            context.stroke();
        }
        context.closePath();
    },

    drawScaledDiagonalCircleEraseOuter(context, centerX, centerY, radius, scale, fillColor = null, strokeColor = null, strokeWidth = 1, indentScale = 0) {
        if (fillColor || strokeColor) {
            this.drawScaledDiagonalCircle(context, centerX, centerY, radius, scale, fillColor, strokeColor, strokeWidth * 2);
        }
        context.save();
        context.beginPath();
        context.globalCompositeOperation = 'destination-out';
        context.rect(0, 0, context.canvas.width, context.canvas.height);
        this.drawScaledDiagonalCircleRaw(context, centerX, centerY, radius, scale, indentScale);
        context.closePath();
        context.fillStyle = 'black';
        context.fill('evenodd');
        context.restore();
    },

    drawScaledDiagonalCircleEraseInner(context, centerX, centerY, radius, scale, fillColor = null, strokeColor = null, strokeWidth = 1, indentScale = 0) {
        const offscreenCanvas = document.createElement('canvas');
        offscreenCanvas.width = context.canvas.width;
        offscreenCanvas.height = context.canvas.height;
        const offscreenCtx = offscreenCanvas.getContext('2d');

        if (fillColor) {
            offscreenCtx.beginPath();
            offscreenCtx.rect(0, 0, offscreenCanvas.width, offscreenCanvas.height);
            offscreenCtx.fillStyle = fillColor;
            offscreenCtx.fill();
            offscreenCtx.closePath();
        }

        offscreenCtx.globalCompositeOperation = 'destination-out';
        offscreenCtx.beginPath();
        this.drawScaledDiagonalCircleRaw(offscreenCtx, centerX, centerY, radius, scale, indentScale);
        offscreenCtx.closePath();
        offscreenCtx.fillStyle = 'black';
        offscreenCtx.fill();

        context.drawImage(offscreenCanvas, 0, 0);

        if (strokeColor) {
            this.drawScaledDiagonalCircle(context, centerX, centerY, radius, null, strokeColor, strokeWidth, indentScale);
        }
    },

    rotateRectangle90(rect, centerX, centerY) {
        let rectCenterX = rect.startX + rect.width / 2;
        let rectCenterY = rect.startY + rect.height / 2;

        let offsetX = rectCenterX - centerX;
        let offsetY = rectCenterY - centerY;

        let rotatedX = -offsetY;
        let rotatedY = offsetX;

        rotatedX += centerX;
        rotatedY += centerY;

        return {
            startX: rotatedX - rect.height / 2,
            startY: rotatedY - rect.width / 2,
            width: rect.height,
            height: rect.width
        };
    },

    drawCutoutCross(context, centerX, centerY, maxRadius, size, fillColor, strokeColor = null, strokeWidth = 1) {
        let rects = [{
            startX: 0,
            startY: 0,
            height: size,
            width: size
        }];

        for (let i = 0; i < 3; i++) {
            rects.push(DrawUtils.rotateRectangle90(rects[i], centerX, centerY));
        }

        rects.forEach(({ startX, startY, width, height }) => {
            context.beginPath();
            context.rect(startX, startY, width, height);
            if (fillColor) {
                context.fillStyle = fillColor;
                context.fill();
            }
            if (strokeColor) {
                context.strokeStyle = strokeColor;
                context.lineWidth = strokeWidth;
                context.stroke();
            }
            context.closePath();
        });
    },

    drawScaledCross(context, centerX, centerY, maxRadius, size, fillColor, strokeColor = null, strokeWidth = 1) {    
        let rects = [{ 
            startX: maxRadius - (size / 2), 
            startY: 0, 
            height: maxRadius - (size / 2), 
            width: size 
        }];
    
        for (let i = 0; i < 3; i++) {
            rects.push(DrawUtils.rotateRectangle90(rects[i], centerX, centerY));
        }
    
        //Center rectangle
        rects.push({
            startX: rects[0].startX, 
            startY: rects[0].startY + rects[0].height, 
            width: rects[0].width, 
            height: (maxRadius - rects[0].height) * 2
        });
        
        rects.forEach(({ startX, startY, width, height }) => {
            context.beginPath();
            context.rect(startX, startY, width, height);
            context.fillStyle = fillColor;
            context.fill();
            if (strokeColor) {
                context.strokeStyle = strokeColor;
                context.lineWidth = strokeWidth;
                context.stroke();
            }
            context.closePath();
        });
    },
};