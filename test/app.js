const antButton =
    document.getElementById("ant");

const led1 =
    document.getElementById("led1");

const led2 =
    document.getElementById("led2");

const queue =
    document.getElementById("queue");

function post(url)
{
    fetch(url,
    {
        method: "POST"
    })
    .catch(console.error);
}

document
    .getElementById("cup")
    .onclick = () => post("/api/cup");

document
    .getElementById("cdown")
    .onclick = () => post("/api/cdown");

document
    .getElementById("lup")
    .onclick = () => post("/api/lup");

document
    .getElementById("ldown")
    .onclick = () => post("/api/ldown");

document
    .getElementById("tune")
    .onclick = () => post("/api/tune");

document
    .getElementById("ant")
    .onclick = () => post("/api/ant");

function updateStatus(data)
{
    document.getElementById("ssid").textContent =
        data.ssid;

    document.getElementById("ip").textContent =
        data.ip;

    document.getElementById("clock").textContent =
        data.time;

    queue.textContent =
        data.queue;

    led1.className =
        data.led1 ?
        "led led-on" :
        "led led-off";

    led2.className =
        data.led2 ?
        "led led-on" :
        "led led-off";

    antButton.textContent =
        data.ant === 2 ?
        "ANT2" :
        "ANT1";

    antButton.className =
        data.ant === 2 ?
        "ant-btn ant2" :
        "ant-btn";
}

fetch("/api/status")
    .then(r => r.json())
    .then(updateStatus);

const ws = new WebSocket(
    `ws://${location.hostname}:81`
);

ws.onopen = () =>
{
    ws.send("status");
};

ws.onmessage = event =>
{
    try
    {
        const data =
            JSON.parse(event.data);

        updateStatus(data);
    }
    catch(ex)
    {
        console.error(ex);
    }
};
