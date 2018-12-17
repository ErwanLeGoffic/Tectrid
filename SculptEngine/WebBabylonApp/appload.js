var Module = null;

function InitModule()
{
    Module = {};
    Module.onRuntimeInitialized = function () {
        window.onerror = null;
        console.log("Emscripten library initalisation ended.");
        let appScript = document.createElement('script');
        appScript.src = "src/app.js";
        appScript.onload = function () {
            console.log("APP loaded.");
            StartAppMain();
        }
        document.head.appendChild(appScript);
    }
}

if(1) {
    console.log("Start load WASM library.");
    InitModule();
    let request = new XMLHttpRequest();
    request.open('GET', 'src/cpplib_wasm.wasm');
    request.responseType = 'arraybuffer';
    request.send();
    request.onload = function () {
        Module.wasmBinary = request.response;
        let cpplibwasmScript = document.createElement('script');
        cpplibwasmScript.src = "src/cpplib_wasm.js";
        document.head.appendChild(cpplibwasmScript);
        window.onerror = function myErrorHandler(errorMsg, url, lineNumber) {
            //alert("WASM failed to initialize, fallback to ASM.JS library.");
            console.log("WASM failed to initialize, fallback to ASM.JS library.");
            InitModule();
            let cpplibasmjsScript = document.createElement('script');
            cpplibasmjsScript.src = "src/cpplib.js";
            document.head.appendChild(cpplibasmjsScript);
            window.onerror = null;
            return true;
        }
    }
}
else {
    InitModule();
    let cpplibScript = document.createElement('script');
    cpplibScript.src = "src/cpplib.js";
    document.head.appendChild(cpplibScript);
}