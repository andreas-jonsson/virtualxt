var memory = new WebAssembly.Memory({
    initial: 700 /* pages */,
    maximum: 700 /* pages */,
});

var importObject = {
    env: {
        js_puts: printCString,
        js_ustimer: () => { return performance.now() * 1000; },
        memory: memory,
    },
};

function printCString(ptr, len) {
    const view = new Uint8Array(memory.buffer, ptr, len);
    let string = '';
    for (let i = 0; i < len; i++) {
        string += String.fromCharCode(view[i]);
    }
    console.log(string);
}

WebAssembly.instantiateStreaming(fetch("virtualxt.wasm"), importObject).then((result) => {
    const wasmMemoryArray = new Uint8Array(memory.buffer);

    const canvas = document.getElementById("virtualxt-canvas");
    const context = canvas.getContext("2d");

    var imageData = context.createImageData(canvas.width, canvas.height);
    context.clearRect(0, 0, canvas.width, canvas.height);

    //const urlParams = new URLSearchParams(window.location.search);
    //const cDrive = urlParams.get('c');

    const renderFrame = () => {
        const width = result.instance.exports.wasm_video_width();
        const height = result.instance.exports.wasm_video_height();

        if ((imageData.width != width) || (imageData.height != height)) {
            canvas.width = width; canvas.height = height;
            imageData = context.createImageData(width, height);
        }

        const bufferOffset = result.instance.exports.wasm_video_rgba_memory_pointer();
        const imageDataArray = wasmMemoryArray.slice(
            bufferOffset,
            bufferOffset + width * height * 4
        );

        imageData.data.set(imageDataArray);
        context.putImageData(imageData, 0, 0);
    };

    result.instance.exports.wasm_initialize_emulator();

    renderFrame();
    setInterval(renderFrame, 32);
    setInterval(() => { result.instance.exports.wasm_step_emulation(100000); }, 1);
});
