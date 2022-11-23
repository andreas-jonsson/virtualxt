// Copyright (c) 2019-2022 Andreas T Jonsson <mail@andreasjonsson.se>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software in
//    a product, an acknowledgment (see the following) in the product
//    documentation is required.
//
//    Portions Copyright (c) 2019-2022 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

const defaultTargetFreq = 4.77;
const defaultBin = "virtualxt.wasm";
const defaultDiskImage = "freedos_web_hd.img";

const jsToXt = {
    "Escape": 1,
    "1": 2, "!": 2,
    "2": 3, "@": 3,
    "3": 4, "#": 4,
    "4": 5, "$": 5,
    "5": 6, "%": 6, // Patched
    "6": 7, "^": 7,
    "7": 8, "&": 8,
    "8": 9, "*": 9,
    "9": 10, "(": 10,
    "0": 11, ")": 11,
    "-": 12, "_": 12,
    "=": 13, "+": 13,
    "Backspace": 14,
    "Tab": 15,
    "q": 16, "Q": 16,
    "w": 17, "W": 17,
    "e": 18, "E": 18,
    "r": 19, "R": 19,
    "t": 20, "T": 20,
    "y": 21, "Y": 21,
    "u": 22, "U": 22,
    "i": 23, "I": 23,
    "o": 24, "O": 24,
    "p": 25, "P": 25,
    "[": 26, "{": 26,
    "]": 27, "}": 27,
    "Enter": 28,
    "Control": 29,
    "a": 30, "A": 30,
    "s": 31, "S": 31,
    "d": 32, "D": 32,
    "f": 33, "F": 33,
    "g": 34, "G": 34,
    "h": 35, "H": 35,
    "j": 36, "J": 36,
    "k": 37, "K": 37,
    "l": 38, "L": 38,
    ";": 39, ":": 39,
    "'": 40, "\"": 40,
    "`": 41,
    "Shift": 42, // Patched
    "\\": 43, "|": 43,
    "z": 44, "Z": 44,
    "x": 45, "X": 45,
    "c": 46, "C": 46,
    "v": 47, "V": 47,
    "b": 48, "B": 48,
    "n": 49, "N": 49,
    "m": 50, "M": 50,
    ",": 51, "<": 51,
    ".": 52, ">": 52,
    "/": 53, "?": 53,
    // VXTP_SCAN_RSHIFT
    "PrintScreen": 55,
    "Alt": 56,
    " ": 57,
    "CapsLock": 58,
    "F1": 59,
    "F2": 60,
    "F3": 61,
    "F4": 62,
    "F5": 63,
    "F6": 64,
    "F7": 65,
    "F8": 66,
    "F9": 67,
    "F10": 68,
    "NumLock": 69,
    "ScrollLock": 70,
    "Home": 71,
    "ArrowUp": 72,
    "PageUp": 73,
    "Subtract": 74,
    "ArrowLeft": 75,
    // VXTP_SCAN_KP_5
    "ArrowRight": 77,
    "Add": 78,
    "End": 79,
    "ArrowDown": 80,
    "PageDown": 81,
    "Insert": 82,
    "Delete": 83
};

const urlParams = new URLSearchParams(window.location.search);

var diskData = null;
var invalidateWindowSize = false;

var memory = new WebAssembly.Memory({
    initial: 350, // Pages
    maximum: 350
});

var importObject = {
    env: {
        memory: memory,
        js_puts: printCString,
        js_disk_read: diskReadData,
        js_disk_write: diskWriteData,
        js_disk_size: () => { return diskData.length; },
        js_ustimer: () => { return performance.now() * 1000; },
        js_speaker_callback: (freq) => { oscillator.frequency.setValueAtTime(freq, null); },
        js_set_border_color: (color) => { document.body.style.background = '#' + ('00000' + (color | 0).toString(16)).substr(-6); }
    }
};

function diskReadData(ptr, size, head) {
    const view = new Uint8Array(memory.buffer, ptr, size);
    for (let i = 0; i < size; i++) {
        view[i] = diskData[head + i];
    }
}

function diskWriteData(ptr, size, head) {
    const view = new Uint8Array(memory.buffer, ptr, size);
    for (let i = 0; i < size; i++) {
        diskData[head + i] = view[i];
    }
}

function printCString(ptr, len) {
    const view = new Uint8Array(memory.buffer, ptr, len);
    let string = '';
    for (let i = 0; i < len; i++) {
        string += String.fromCharCode(view[i]);
    }
    console.log(string);
}

function handleKeyEvent(event, mask, callback) {
    if (event.defaultPrevented) {
        return;
    }

    if (event.repeat) {
        event.preventDefault();
        return;
    }

    var scan = jsToXt[event.key];
    if (!scan) {
        console.log("Unknown key:", event.key);
        return;
    }

    // Special case for shift & numpad 5.
    if ((scan == 42) && (event.location == KeyboardEvent.DOM_KEY_LOCATION_RIGHT)) {
        scan = 54;
    } else if ((scan == 6) && (event.location == KeyboardEvent.DOM_KEY_LOCATION_NUMPAD)) {
        scan = 76;
    }

    callback(scan | mask)
    event.preventDefault();
}

function uploadDiskImage(afterPickCallback) {
    var input = document.createElement("input");
    input.type = "file";
    input.accept = ".img";
    input.multiple = false;
    input.onchange = () => {
        file = input.files[0];
        console.log("Selected: " + file.name);
        if (afterPickCallback) {
            afterPickCallback(file);
        }
        var reader = new FileReader();
        reader.onload = () => { diskData = new Uint8Array(reader.result); };
        reader.readAsArrayBuffer(file);
    };
    input.click();
}

function loadDiskImage(url) {
    console.log("Loading: " + url);
    
    var xhr = new XMLHttpRequest();
    xhr.open("GET", url, true);
    xhr.responseType = "arraybuffer";
    xhr.onload = () => { diskData = (xhr.status == 200) ? new Uint8Array(xhr.response) : null; };
    xhr.send();
}

function startEmulator() {
    WebAssembly.instantiateStreaming(fetch(urlParams.get("bin") || defaultBin), importObject).then((result) => {
        const C = result.instance.exports;
        const wasmMemoryArray = new Uint8Array(memory.buffer);

        const audioCtx = new (window.AudioContext || window.webkitAudioContext)({latencyHint: "interactive"});

        oscillator = audioCtx.createOscillator();
        oscillator.type = "square";
        oscillator.connect(audioCtx.destination);
        oscillator.start();

        document.body.style.padding = 0;
        document.body.style.margin = 4;

        const crtAspect = 4 / 3;
        const fixedWidth = urlParams.get("width");
        
        const canvas = document.getElementById("virtualxt-canvas");
        canvas.style = "image-rendering: pixelated; image-rendering: crisp-edges; transform-origin: left top;";

        const context = canvas.getContext("2d");
        var imageData = context.createImageData(canvas.width, canvas.height);
        context.clearRect(0, 0, canvas.width, canvas.height);

        const renderFrame = () => {
            const width = C.wasm_video_width();
            const height = C.wasm_video_height();

            if ((imageData.width != width) || (imageData.height != height) || invalidateWindowSize) {
                invalidateWindowSize = false;

                var targetWidth = fixedWidth || document.body.clientWidth;
                if (!fixedWidth) {
                    const windowHeight = window.innerHeight - 8;
                    const targetAspect = targetWidth / windowHeight;

                    if (targetAspect > crtAspect) {
                        targetWidth = windowHeight * crtAspect;
                    }
                }

                const xScale = targetWidth / width; // Adjust for low vs high CGA resolution.
                const yScale = (width / crtAspect) / height;
                const transX = urlParams.has("translate") ? parseInt(urlParams.get("translate")) : (document.body.clientWidth * 0.5 - width * xScale * 0.5);

                canvas.width = width; canvas.height = height;
                canvas.style.setProperty("transform", "matrix(" + xScale + ",0,0," + (xScale * yScale) + "," + transX + ",0)");
                console.log("Resolution changed: " + width + "x" + height + "(" + (width * xScale).toFixed() + "x" + (height * yScale * xScale).toFixed() + ")");

                imageData = context.createImageData(width, height);
            }

            const bufferOffset = C.wasm_video_rgba_memory_pointer();
            const imageDataArray = wasmMemoryArray.slice(
                bufferOffset,
                bufferOffset + width * height * 4
            );

            imageData.data.set(imageDataArray);
            context.putImageData(imageData, 0, 0);

            window.requestAnimationFrame(renderFrame);
        };

        const initialize = () => {
            C.wasm_initialize_emulator();

            window.addEventListener("keyup", (e) => { handleKeyEvent(e, 0x80, C.wasm_send_key); }, true);
            window.addEventListener("keydown", (e) => { handleKeyEvent(e, 0x0, C.wasm_send_key); }, true);
            window.addEventListener("resize", () => { invalidateWindowSize = true; });

            window.requestAnimationFrame(renderFrame);

            const targetFreq = urlParams.get("freq") || defaultTargetFreq;
            const cycleCap = targetFreq * 15000;
            var stepper = { t: performance.now(), c: 0 };

            setInterval(() => {
                const t = performance.now();
                const delta = t - stepper.t;

                var cycles = delta * targetFreq * 1000;
                if (cycles > cycleCap) {
                    cycles = cycleCap;
                }

                C.wasm_step_emulation(cycles);
                stepper.c += cycles;
                stepper.t = t;
            }, 1);

            setInterval(() => {
                console.log("Frequency:", stepper.c / 1000000, "Mhz");
                stepper.c = 0;

                if (audioCtx.state == "suspended") {
                    audioCtx.resume();
                }
            }, 1000);
        }

        const waitForDisk = () => {
            if (!diskData) {
                setTimeout(waitForDisk);
                return;
            }
            initialize();
        }
        setTimeout(waitForDisk);
    });
}

window.onload = () => {
    const div = document.createElement("div");
    const btn = document.createElement("button");

    {
        btn.type = "button";
        btn.style = "background-color: #555555; border: none; color: white; padding: 8px 16px; text-align: center; text-decoration: none; font-size: 14px;";

        const divInner = document.createElement("div");
        divInner.style = "position: absolute; bottom: 50%;";
        divInner.appendChild(btn);

        div.style = "display: flex; justify-content: center;";
        div.appendChild(divInner);
    }
    
    const canvas = document.getElementById("virtualxt-canvas");
    const loadStart = () => {
        loadDiskImage(urlParams.get("img") || defaultDiskImage);
        startEmulator();
    };

    if (urlParams.get("upload") == "1") {
        canvas.hidden = true;
        btn.innerText = "Upload Disk Image";
        btn.onclick = () => {
            uploadDiskImage(() => {
                startEmulator();
                document.body.removeChild(div);
                canvas.hidden = false;
            });
        };
        document.body.appendChild(div);
    } else if (urlParams.has("start")) {
        canvas.hidden = true;
        btn.innerText = urlParams.get("start");
        btn.onclick = () => {
            loadStart();
            document.body.removeChild(div);
            canvas.hidden = false;
        };
        document.body.appendChild(div);
    } else {
        loadStart();
    }
};
