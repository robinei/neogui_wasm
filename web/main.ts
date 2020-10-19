import { Renderer } from "./render";

declare var Module: any; // emscripten Module
declare var _test_text: any;

const canvas = document.getElementById('canvas') as HTMLCanvasElement;
let runtimeInitialize = false;
let windowLoaded = false;
let started = false;

Module.setStatus = (status: string) => {
    if (status) {
        console.log(status);
    }
};

Module.onRuntimeInitialized = () => {
    runtimeInitialize = true;
    console.log("Runtime initialized.");
    maybeStart();
};

window.onload = () => {
    windowLoaded = true;
    console.log("Window loaded.");
    maybeStart();
};

function maybeStart() {
    if (started || !runtimeInitialize || !windowLoaded) {
        return;
    }
    started = true;
    console.log("Starting...");

    let renderer = new Renderer(canvas);
    renderer.reshape();

    window.addEventListener("resize", () => renderer.reshape());

    loadFont();
}


function loadFont() {
    const req = new XMLHttpRequest();
    req.open("GET", "/data/OpenSans-Regular.ttf", true);
    req.responseType = "arraybuffer";
    
    req.onload = (e) => {
        const arrayBuffer = req.response;
        if (arrayBuffer) {
            const byteArray = new Uint8Array(arrayBuffer);
            console.log("Got bytes: ", byteArray.byteLength);
            
            var ptr = Module._malloc(byteArray.byteLength);
            Module.HEAP8.set(byteArray, ptr);
            _test_text(ptr, byteArray.byteLength);
        }
    };
    
    req.send(null);
}

/*
const canvas = document.getElementById('canvas');
const canvasContext = canvas.getContext('2d');

function redraw() {
    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;
    if (Module.asm) {
        Module.asm.test_ui(canvas.width, canvas.height);
    }
}

window.onload = window.onresize = redraw;
*/