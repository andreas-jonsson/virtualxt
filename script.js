// Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
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
//    Portions Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

const defaultTargetFreq = 4.772726;
const defaultBin = "virtualxt.wasm";
const defaultDiskImage = "freedos_web_hd.img";
const defaultCanvas = "virtualxt-canvas";

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

const vkbToXt = {
    "{esc}": 1,
    "1": 2, "!": 2,
    "2": 3, "@": 3,
    "3": 4, "#": 4,
    "4": 5, "$": 5,
    "5": 6, "%": 6,
    "6": 7, "^": 7,
    "7": 8, "&": 8,
    "8": 9, "*": 9,
    "9": 10, "(": 10,
    "0": 11, ")": 11,
    "-": 12, "_": 12,
    "=": 13, "+": 13,
    "{bksp}": 14,
    "{tab}": 15,
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
    "{enter}": 28,
    "ctrl": 29,
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
    "shift": 42,
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
    "print": 55,
    "{alt}": 56,
    " ": 57,
    "{lock}": 58,
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
    "num": 69,
    "{lock}": 70,
    "home": 71,
    "up": 72,
    "pgup": 73,
    // VXTP_SCAN_KP_MINUS
    "left": 75,
    // VXTP_SCAN_KP_5
    "right": 77,
    // VXTP_SCAN_KP_PLUS
    "end": 79,
    "down": 80,
    "pgdown": 81,
    "ins": 82,
    "{delete}": 83
};

const modelFLayout = {
    'default': [
        'F1 F2 F3 F4 F5 F6 F7 F8 F9 F10 scrl num',
        '{esc} 1 2 3 4 5 6 7 8 9 0 - = {bksp}',
        '{tab} q w e r t y u i o p [ ] {enter}',
        '{lock} a s d f g h j k l ; \' ~ print',
        'shift \\ z x c v b n m , . /',
        'ctrl {alt} {space} {delete}'
    ],
    'shift': [
        'up down left right pgup pgdown home end ins scrl num',
        '{esc} ! @ # $ % ^ &amp; * ( ) _ + {bksp}',
        '{tab} Q W E R T Y U I O P { } {enter}',
        '{lock} A S D F G H J K L : " ` print',
        'shift | Z X C V B N M &lt; &gt; ?',
        'ctrl {alt} {space} {delete}'
    ]
}

const urlParams = new URLSearchParams(window.location.search);

var targetFreq = urlParams.get("freq") || defaultTargetFreq;
var diskData = null;
var invalidateWindowSize = false;
var halted = false;
var dbQueue = null;

var memory = new WebAssembly.Memory({
    initial: 350, // Pages
    maximum: 350
});

var importObject = {
    env: {
        memory: memory,
        js_puts: printCString,
        js_shutdown: shutdown,
        js_disk_read: diskReadData,
        js_disk_write: diskWriteData,
        js_disk_size: () => { return diskData.length; },
        js_ustimer: () => { return performance.now() * 1000; },
        js_speaker_callback: (freq) => { oscillator.frequency.setValueAtTime(freq, null); },
        js_set_border_color: (color) => { document.body.style.background = "#" + ("00000" + (color | 0).toString(16)).substr(-6); }
    }
};

function shutdown() {
    if (!halted) {
        halted = true;
        if (urlParams.has("ret")) {
            window.open(urlParams.get("ret"), "_self");
        } else {
            targetFreq = 1 / 1000000;
            document.body.style = "background-color: black; color: lightgray;";
            document.body.innerHTML = "System halted!";
        }
    }
}

function isTouchDevice() {
    if (urlParams.has("touch"))
        return urlParams.get("touch") != "0";
    return ('ontouchstart' in window) ||
        (navigator.maxTouchPoints > 0) ||
        (navigator.msMaxTouchPoints > 0);
}

function diskReadData(ptr, size, head) {
    const view = new Uint8Array(memory.buffer, ptr, size);
    for (let i = 0; i < size; i++) {
        view[i] = diskData[head + i];
    }
}

function diskWriteData(ptr, size, head) {
    const view = new Uint8Array(memory.buffer, ptr, size);
    const message = { sector: head / 512, data: new Array(512) };

    for (let i = 0; i < size; i++) {
        const data = view[i];
        diskData[head + i] = data;
        message.data[i] = data;
    }
    if (dbQueue) {
        dbQueue.push(message);
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

function handleVirtualKeyEvent(button, keyboard, mask, callback) {
    if (mask == 0 && (button == "{lock}")) { // || button == "{shift}")) {
        keyboard.setOptions({
            layoutName: (keyboard.options.layoutName == "shift") ? "default" : "shift"
        });
    }

    const scan = vkbToXt[button];
    if (!scan) {
        console.log("Unknown virtual key:", button);
        return;
    }
    callback(scan | mask)
}

function initLocalStorage() {
    if (!window.localStorage) {
        console.error("Local storage not supported!");
        return;
    }

    console.log("Initialize local storage");
    dbQueue = [];

    if (urlParams.get("storage") == "clear") {
        console.log("Clear local storage!");
        window.localStorage.clear();
    }

    const calc_crc32 = (r) => {
        for (var a, o = [], c = 0; c < 256; c++) {
            a = c;
            for (var f = 0; f < 8; f++) {
                a = (1 & a) ? 3988292384 ^ a >>> 1 : a >>> 1;
            }
            o[c] = a;
        }
        for (var n = -1, t = 0; t < r.length; t++) {
            n = n >>> 8 ^ o[255 & (n ^ r[t])];
        }
        return (-1 ^ n) >>> 0;
    };
    
    const crc_str = calc_crc32(diskData).toString();
    for (var key of Object.keys(window.localStorage)) {
        const pair = key.split(" ", 2);
        if (pair[0] != crc_str) {
            continue;
        }
        const sector = parseInt(pair[1]);
        const data = JSON.parse(window.localStorage.getItem(key));
        diskData.set(new Uint8Array(data), sector * 512);
        console.log("Restore sector:", sector);
    }

    setInterval(() => {
        var i = 0;
        for (; i < dbQueue.length; i++) {
            const msg = dbQueue.shift();
            const str = JSON.stringify(msg.data);

            try { window.localStorage.setItem(crc_str + " " + msg.sector, str); } catch {
                console.error("Local storager is full!");
                alert("Local storager is full! All persistant data will be deleted.");
                window.localStorage.clear(); // This will remove all data including CRC.
            }
        }
        if (i > 0) {
            console.log(i + " sectors written to local storage!");
        }
    }, 1000);
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

function getCanvas() {
    return document.getElementById(urlParams.get("canvas") || defaultCanvas);
}

function startEmulator(binary) {
    WebAssembly.instantiateStreaming(binary, importObject).then((result) => {
        const C = result.instance.exports;
        const wasmMemoryArray = new Uint8Array(memory.buffer);

        const audioCtx = new (window.AudioContext || window.webkitAudioContext)({latencyHint: "interactive"});

        oscillator = audioCtx.createOscillator();
        oscillator.type = "square";
        oscillator.frequency = 1;
        oscillator.connect(audioCtx.destination);
        oscillator.start();

        document.body.style.padding = 0;
        document.body.style.margin = 4;

        const crtAspect = 4 / 3;
        const fixedWidth = urlParams.get("width");
        
        const canvas = getCanvas();
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
            C.wasm_initialize_emulator((urlParams.get("v20") == 1) ? 1 : 0);

            if (urlParams.has("storage") && (urlParams.get("storage") != "0")) {
                initLocalStorage();
            }

            window.addEventListener("keyup", e => { handleKeyEvent(e, 0x80, C.wasm_send_key); }, true);
            window.addEventListener("keydown", e => { handleKeyEvent(e, 0x0, C.wasm_send_key); }, true);
            window.addEventListener("resize", () => { invalidateWindowSize = true; });

            if (isTouchDevice()) {
                const kb = new window.SimpleKeyboard.default({
                    onKeyReleased: button => { handleVirtualKeyEvent(button, kb, 0x80, C.wasm_send_key); },
                    onKeyPress: button => { handleVirtualKeyEvent(button, kb, 0x0, C.wasm_send_key); },
                    physicalKeyboardHighlightPress: false,
                    layout: modelFLayout
                });

                const kbDiv = document.getElementById("keyboard");
                if (kbDiv) {
                    const handler = () => {
                        if (kbDiv.style.getPropertyValue("display") == "none")
                            kbDiv.style.removeProperty("display");
                        else
                            kbDiv.style.setProperty("display", "none");
                    }
                    canvas.addEventListener("pointerdown", handler, true);
                    canvas.addEventListener("touchstart", handler, true);
                }
            } else if (urlParams.get("mouse") != "0") {
                const handler = (e) => {
                    if (halted) {
                        return;
                    }
                    const hidden = (document.pointerLockElement == document.body);
                    if (hidden && ((e.buttons & 4) != 0)) {
                        document.body.exitPointerLock();
                    } else if (!hidden && e.buttons != 0) {
                        document.body.requestPointerLock();
                    } else if (hidden) {
                        C.wasm_send_mouse(e.movementX, e.movementY, e.buttons);
                    }
                };

                window.addEventListener("pointermove", handler);
                window.addEventListener("pointerdown", handler);
                window.addEventListener("pointerup", handler);
            }

            window.requestAnimationFrame(renderFrame);

            const cycleCap = targetFreq * 15000;
            var stepper = { t: performance.now(), c: 0 };

            setInterval(() => {
                const t = performance.now();
                const delta = t - stepper.t;

                var cycles = delta * targetFreq * 1000;
                if (cycles > cycleCap) {
                    cycles = cycleCap;
                }

                stepper.c += C.wasm_step_emulation(cycles);
                stepper.t = t;
            }, 1);

            setInterval(() => {
                console.log("Frequency:", stepper.c / 1000000, "Mhz");
                stepper.c = 0;

                if (audioCtx.state == "suspended") {
                    audioCtx.resume();
                }
            }, 1000);
        };

        const waitForDisk = () => {
            if (!diskData) {
                setTimeout(waitForDisk);
                return;
            }
            initialize();
        };
        waitForDisk();
    });
}

// Get wasm binary asap.
const wasmBinary = fetch(urlParams.get("bin") || defaultBin);

// If we are NOT uploading we can start load directly.
const doUpload = (urlParams.get("img") == "upload");
if (!doUpload) {
    loadDiskImage(urlParams.get("img") || defaultDiskImage);
}

window.onload = () => {
    const canvas = getCanvas();
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
    
    if (doUpload) {
        canvas.hidden = true;
        btn.innerText = "Upload Disk Image";
        btn.onclick = () => {
            uploadDiskImage(() => {
                startEmulator(wasmBinary);
                document.body.removeChild(div);
                canvas.hidden = false;
            });
        };
        document.body.appendChild(div);
    } else if (urlParams.has("start")) {
        canvas.hidden = true;
        btn.innerText = urlParams.get("start");
        btn.onclick = () => {
            document.body.removeChild(div);
            canvas.hidden = false;
            startEmulator(wasmBinary);
        };
        document.body.appendChild(div);
    } else {
        startEmulator(wasmBinary);
    }
};
