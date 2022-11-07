var memory = new WebAssembly.Memory({
    initial: 500 /* pages */,
    maximum: 500 /* pages */,
});

var importObject = {
    env: {
        js_puts: printCString,
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
    const imageData = context.createImageData(canvas.width, canvas.height);
    context.clearRect(0, 0, canvas.width, canvas.height);

    const renderFrame = () => {
        const bufferOffset = result.instance.exports.wasm_video_rgba_memory_pointer();
        const imageDataArray = wasmMemoryArray.slice(
            bufferOffset,
            bufferOffset + 640 * 200 * 4
        );
        imageData.data.set(imageDataArray);

        context.clearRect(0, 0, canvas.width, canvas.height);
        context.putImageData(imageData, 0, 0);
    };

    const stepEmulation = () => {
        result.instance.exports.wasm_step_emulation(10000);
    };

    result.instance.exports.wasm_initialize_emulator();

    //renderFrame();
    //setInterval(renderFrame, 320);
    //setInterval(stepEmulation, 100);
});
