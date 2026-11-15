const antButton =
    document.getElementById("ant");

const led1 =
    document.getElementById("led1");

const led2 =
    document.getElementById("led2");

const queue =
    document.getElementById("queue");

let currentTime = null;

function post(url)
{
    fetch(url,
    {
        method: "POST"
    })
    .catch(console.error);
}

document.getElementById("cup").onclick = () => post("/api/cup");
document.getElementById("cdown").onclick = () => post("/api/cdown");
document.getElementById("lup").onclick = () => post("/api/lup");
document.getElementById("ldown").onclick = () => post("/api/ldown");
document.getElementById("tune").onclick = () => post("/api/tune");
document.getElementById("ant").onclick = () => post("/api/ant");

function updateStatus(data)
{
    document.getElementById("ssid").textContent = data.ssid;
    document.getElementById("ip").textContent = data.ip;

    currentTime = data.time;
    document.getElementById("clock").textContent = currentTime;

    queue.textContent = data.queue;

    led1.className = data.led1 ? "led led-on" : "led led-off";
    led2.className = data.led2 ? "led led-on" : "led led-off";

    antButton.textContent = data.ant === 2 ? "ANT2" : "ANT1";
    antButton.className = data.ant === 2 ? "ant-btn ant2" : "ant-btn";
}

setInterval(() =>
{
    if (!currentTime || currentTime === "--:--:--")
        return;

    const parts = currentTime.split(":");

    let h = parseInt(parts[0], 10);
    let m = parseInt(parts[1], 10);
    let s = parseInt(parts[2], 10);

    s++;

    if (s >= 60)
    {
        s = 0;
        m++;
    }

    if (m >= 60)
    {
        m = 0;
        h++;
    }

    if (h >= 24)
    {
        h = 0;
    }

    currentTime =
        `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}:${String(s).padStart(2, '0')}`;

    document.getElementById("clock").textContent = currentTime;

}, 1000);

fetch("/api/status")
    .then(r => r.json())
    .then(updateStatus);

const ws = new WebSocket(`ws://${location.hostname}:81`);

ws.onopen = () =>
{
    ws.send("status");
};

ws.onmessage = event =>
{
    try
    {
        const data = JSON.parse(event.data);
        updateStatus(data);
    }
    catch(ex)
    {
        console.error(ex);
    }
};
