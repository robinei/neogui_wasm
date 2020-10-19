function isInteger(value: any) {
    return typeof value === "number" && isFinite(value) && Math.floor(value) === value;
}

const VERTEX_SHADER_CODE = `
attribute vec4 a_position;
attribute vec2 a_texcoord;
 
uniform mat4 u_matrix;
 
varying vec2 v_texcoord;
 
void main() {
    gl_Position = u_matrix * a_position;
    v_texcoord = a_texcoord;
}
`;

const FRAGMENT_SHADER_CODE = `
precision mediump float;
     
varying vec2 v_texcoord;
 
uniform sampler2D u_texture;
 
void main() {
    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);//texture2D(u_texture, v_texcoord);
}
`;

export class Renderer {
    private wantRedraw: boolean = false;
    private redrawScheduled: boolean = false;

    private gl: WebGLRenderingContext | undefined;

    private removeEventListeners: () => void;
    private deleteGLHandles?: () => void;
    private doDraw?: () => void;

    constructor(private readonly canvas: HTMLCanvasElement) {
        const onError = (e: WebGLContextEvent) => {
            console.log("Error creating WebGL context: " + e.statusMessage);
        };
        const onLost = (e: WebGLContextEvent) => {
            console.log("Lost WebGL context");
            this.doDestroy();
            e.preventDefault();
        };
        const onRestore = (e: WebGLContextEvent) => {
            console.log("Restoring WebGL context");
            this.createContext();
        };

        this.removeEventListeners = () => {
            canvas.removeEventListener("webglcontextcreationerror", onError as EventListener);
            canvas.removeEventListener("webglcontextlost", onLost as EventListener);
            canvas.removeEventListener("webglcontextrestored", onRestore as EventListener);
        };

        canvas.addEventListener("webglcontextcreationerror", onError as EventListener, false);
        canvas.addEventListener("webglcontextlost", onLost as EventListener, false);
        canvas.addEventListener("webglcontextrestored", onRestore as EventListener, false);

        this.createContext();

        if (!this.gl) {
            this.removeEventListeners();
        }
    }

    destroy() {
        this.removeEventListeners();
        this.doDestroy();
    }

    private doDestroy(): void {
        if (this.deleteGLHandles) {
            this.deleteGLHandles();
            this.deleteGLHandles = undefined;
        }
        this.doDraw = undefined;
        this.gl = undefined;
    }

    private createContext(): void {
        const { canvas } = this;
        const gl = this.gl = <WebGLRenderingContext>(canvas.getContext("webgl") || canvas.getContext("experimental-webgl"));
        if (!gl) {
            console.log("Error getting WebGL context.");
            return;
        }

        const vertexShader = compileShader(gl, VERTEX_SHADER_CODE, gl.VERTEX_SHADER);
        const fragmentShader = compileShader(gl, FRAGMENT_SHADER_CODE, gl.FRAGMENT_SHADER);
        const shaderProgram = createProgram(gl, vertexShader, fragmentShader);
        gl.useProgram(shaderProgram);

        const matrixUniform = gl.getUniformLocation(shaderProgram, "u_matrix");
        checkGlError(gl, "getUniformLocation");

        const vertexBuffer = gl.createBuffer();
        checkGlError(gl, "createBuffer");
        gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
        checkGlError(gl, "bindBuffer");
        const vertexBufferArray = new Float32Array([
            0, 0, 0, 1,
            0, 100, 0, 1,
            100, 0, 0, 1,
        ]);
        gl.bufferData(gl.ARRAY_BUFFER, vertexBufferArray, gl.STATIC_DRAW);
        checkGlError(gl, "bufferData");

        const positionAttribute = gl.getAttribLocation(shaderProgram, "a_position");
        checkGlError(gl, "getAttribLocation");
        gl.enableVertexAttribArray(positionAttribute);
        checkGlError(gl, "enableVertexAttribArray");
        gl.vertexAttribPointer(positionAttribute, 4, gl.FLOAT, false, 0, 0);
        checkGlError(gl, "vertexAttribPointer");

        gl.clearColor(0, 0, 0, 1);
        checkGlError(gl, "clearColor");
        gl.disable(gl.DEPTH_TEST);
        checkGlError(gl, "disable");
        gl.disable(gl.CULL_FACE);
        checkGlError(gl, "disable");
        gl.depthMask(false);
        checkGlError(gl, "depthMask");

        this.deleteGLHandles = () => {
            gl.deleteProgram(shaderProgram);
            gl.deleteShader(vertexShader);Float32Array
            gl.deleteShader(fragmentShader);
            gl.deleteBuffer(vertexBuffer);
        };

        this.doDraw = () => {
            const screenWidth = this.canvas.width;
            const screenHeight = this.canvas.height;
            gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
            gl.uniformMatrix4fv(matrixUniform, false, orthographicMatrix(0, screenWidth, screenHeight, 0, -1, 1));
            gl.drawArrays(gl.TRIANGLES, 0, 3);
        };

        this.reshape();
    }

    reshape() {
        const width = window.innerWidth;
        const height = window.innerHeight;

        if (isInteger(window.devicePixelRatio)) {
            this.canvas.classList.add("pixelated-canvas");
        } else {
            this.canvas.classList.remove("pixelated-canvas");
        }

        this.canvas.width = width;
        this.canvas.height = height;
        console.log(`Reshaped display: ${width} x ${height}`);

        if (this.gl) {
            this.gl.viewport(0, 0, width, height);
            checkGlError(this.gl, "viewport");
        }

        this.redraw();
    }

    redraw() {
        if (this.redrawScheduled) {
            this.wantRedraw = true;
        } else {
            this.scheduleRedraw();
        }
    }

    private scheduleRedraw() {
        this.redrawScheduled = true;
        window.requestAnimationFrame(() => {
            this.redrawScheduled = false;
            if (this.doDraw) {
                this.doDraw();
            }
            if (this.wantRedraw) {
                this.wantRedraw = false;
                this.scheduleRedraw();
            }
        });
    }
}


function compileShader(gl: WebGLRenderingContext, shaderSource: string, shaderType: number): WebGLShader {
    const shader = gl.createShader(shaderType);
    if (!shader) {
        throw new Error("error creating shader");
    }
    gl.shaderSource(shader, shaderSource);
    gl.compileShader(shader);
    const success = gl.getShaderParameter(shader, gl.COMPILE_STATUS);
    if (!success) {
        throw new Error("could not compile shader: " + gl.getShaderInfoLog(shader));
    }
    return shader;
}

function createProgram(gl: WebGLRenderingContext, vertexShader: WebGLShader, fragmentShader: WebGLShader): WebGLProgram {
    const program = gl.createProgram();
    if (!program) {
        throw new Error("error creating program");
    }
    gl.attachShader(program, vertexShader);
    gl.attachShader(program, fragmentShader);
    gl.linkProgram(program);
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
        throw new Error("program filed to link: " + gl.getProgramInfoLog (program));
    }
    return program;
}

function checkGlError(gl: WebGLRenderingContext, operation: string): void {
    const error = gl.getError();
    if (error !== gl.NO_ERROR && error !== gl.CONTEXT_LOST_WEBGL) {
        const msg = `WebGL error (${operation}): ${error}`;
        console.log(msg);
        throw new Error(msg);
    }
}

function makeTexture(gl: WebGLRenderingContext): WebGLTexture {
    const texture = gl.createTexture();
    if (!texture) {
        throw new Error("error creating texture");
    }
    checkGlError(gl, "createTexture");
    gl.bindTexture(gl.TEXTURE_2D, texture);
    checkGlError(gl, "bindTexture");
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    checkGlError(gl, "texParameteri");
    return texture;
}

function orthographicMatrix(left: number, right: number, bottom: number, top: number, near: number, far: number) {
    const lr = 1 / (left - right);
    const bt = 1 / (bottom - top);
    const nf = 1 / (near - far);
      
    const row4col1 = (left + right) * lr;
    const row4col2 = (top + bottom) * bt;
    const row4col3 = (far + near) * nf;
    
    return [
         -2 * lr,        0,        0, 0,
               0,  -2 * bt,        0, 0,
               0,        0,   2 * nf, 0,
        row4col1, row4col2, row4col3, 1
    ];
}



