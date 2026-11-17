const antButton = document.getElementById("ant");

const led1 = document.getElementById("led1");

const led2 = document.getElementById("led2");

const queue = document.getElementById("queue");

const settingsPanel = document.getElementById("settingsPanel");

let currentTime = null;
let lastWsMessage = Date.now();
let longPressMs = 500;

function post(url) {
  fetch(url, {
    method: "POST",
  })
    .then(() => {
      return fetch("/api/status");
    })
    .then((r) => r.json())
    .then(updateStatus)
    .catch(console.error);
}

const cupButton = document.getElementById("cup");

cupButton.addEventListener("pointerdown", () => {
  post("/api/cup/start");
});

cupButton.addEventListener("pointerup", () => {
  post("/api/cup/stop");
});

cupButton.addEventListener("pointerleave", () => {
  post("/api/cup/stop");
});

const cdownButton = document.getElementById("cdown");

cdownButton.addEventListener("pointerdown", () => {
  post("/api/cdown/start");
});

cdownButton.addEventListener("pointerup", () => {
  post("/api/cdown/stop");
});

cdownButton.addEventListener("pointerleave", () => {
  post("/api/cdown/stop");
});

const lupButton = document.getElementById("lup");

lupButton.addEventListener("pointerdown", () => {
  post("/api/lup/start");
});

lupButton.addEventListener("pointerup", () => {
  post("/api/lup/stop");
});

lupButton.addEventListener("pointerleave", () => {
  post("/api/lup/stop");
});

const ldownButton = document.getElementById("ldown");

ldownButton.addEventListener("pointerdown", () => {
  post("/api/ldown/start");
});

ldownButton.addEventListener("pointerup", () => {
  post("/api/ldown/stop");
});

ldownButton.addEventListener("pointerleave", () => {
  post("/api/ldown/stop");
});

function setupLongPress(buttonId, shortUrl, startUrl, stopUrl) {
  const button = document.getElementById(buttonId);

  let pressTimer = null;

  let longPressActive = false;

  button.addEventListener("pointerdown", () => {
    longPressActive = false;

    pressTimer = setTimeout(() => {
      longPressActive = true;

      post(startUrl);
    }, longPressMs);
  });

  button.addEventListener("pointerup", () => {
    clearTimeout(pressTimer);

    if (longPressActive) {
      post(stopUrl);
    } else {
      post(shortUrl);
    }
  });

  button.addEventListener("pointerleave", () => {
    clearTimeout(pressTimer);

    if (longPressActive) {
      post(stopUrl);
    }
  });
}

setupLongPress("cup", "/api/cup", "/api/cup/start", "/api/cup/stop");

setupLongPress("cdown", "/api/cdown", "/api/cdown/start", "/api/cdown/stop");

setupLongPress("lup", "/api/lup", "/api/lup/start", "/api/lup/stop");

setupLongPress("ldown", "/api/ldown", "/api/ldown/start", "/api/ldown/stop");

document.getElementById("tune").onclick = () => post("/api/tune");

document.getElementById("ant").onclick = () => post("/api/ant");

document.getElementById("settingsBtn").onclick = () => {
  settingsPanel.classList.toggle("show");
};

document.getElementById("wifiPortalBtn").onclick = () =>
  post("/api/wifiportal");

document.getElementById("rebootBtn").onclick = () => {
  fetch("/api/reboot", {
    method: "POST",
  });

  document.body.innerHTML = `
        <div style="
            color:white;
            background:black;
            height:100vh;
            display:flex;
            flex-direction:column;
            justify-content:center;
            align-items:center;
            font-family:Arial;
        ">
            <h2>ESP is rebooting...</h2>
            <p>Waiting for reconnect</p>
        </div>
    `;

  const waitForEsp = setInterval(async () => {
    try {
      const response = await fetch("/api/status");

      if (response.ok) {
        clearInterval(waitForEsp);

        location.reload();
      }
    } catch (e) {}
  }, 2000);
};

document.getElementById("fwUpdateBtn").onclick = () => {
  window.location = "/fwupdate";
};

document.getElementById("webUpdateBtn").onclick = () => {
  window.location = "/webupdate";
};

function updateStatus(data) {
  if (data.longPressMs !== undefined) {
    longPressMs = data.longPressMs;
  }

  document.getElementById("ssid").textContent = data.ssid;

  document.getElementById("ip").textContent = data.ip;

  const rssiElement = document.getElementById("rssi");

  rssiElement.textContent = data.rssi + " dBm";

  if (data.rssi >= -60) {
    rssiElement.style.color = "#00ff00";
  } else if (data.rssi >= -75) {
    rssiElement.style.color = "#ffcc00";
  } else {
    rssiElement.style.color = "#ff4444";
  }

  currentTime = data.time;

  document.getElementById("clock").textContent = currentTime;

  if (data.uptime !== undefined) {
    const uptimeSeconds = data.uptime;

    const days = Math.floor(uptimeSeconds / 86400);

    const hours = Math.floor((uptimeSeconds % 86400) / 3600);

    const minutes = Math.floor((uptimeSeconds % 3600) / 60);

    document.getElementById("uptime").textContent =
      `UP: ${days}d ${String(hours).padStart(2, "0")}h ${String(minutes).padStart(2, "0")}m`;
  }

  queue.textContent = data.queue;

  document.getElementById("gitversion").textContent = data.version;

  document.getElementById("builddate").textContent = data.build;

  led1.className = data.led1 ? "led led-tuning-on" : "led led-off";

  led2.className = data.led2 ? "led led-swr-on" : "led led-off";

  antButton.textContent = data.ant === 2 ? "ANT2" : "ANT1";

  antButton.className = data.ant === 2 ? "ant-btn ant2" : "ant-btn";
}

setInterval(() => {
  if (!currentTime || currentTime === "--:--:--") {
    return;
  }

  const parts = currentTime.split(":");

  let h = parseInt(parts[0], 10);
  let m = parseInt(parts[1], 10);
  let s = parseInt(parts[2], 10);

  s++;

  if (s >= 60) {
    s = 0;
    m++;
  }

  if (m >= 60) {
    m = 0;
    h++;
  }

  if (h >= 24) {
    h = 0;
  }

  currentTime =
    `${String(h).padStart(2, "0")}:` +
    `${String(m).padStart(2, "0")}:` +
    `${String(s).padStart(2, "0")}`;

  document.getElementById("clock").textContent = currentTime;
}, 1000);

fetch("/api/status")
  .then((r) => r.json())
  .then(updateStatus);

let ws = null;

function connectWebSocket() {
  ws = new WebSocket(`ws://${location.hostname}:81`);

  ws.onopen = () => {
    console.log("WebSocket connected");

    ws.send("status");
  };

  ws.onmessage = (event) => {
    lastWsMessage = Date.now();

    try {
      const data = JSON.parse(event.data);

      updateStatus(data);
    } catch (ex) {
      console.error(ex);
    }
  };

  ws.onclose = () => {
    console.log("WebSocket disconnected");

    setTimeout(connectWebSocket, 2000);
  };

  ws.onerror = () => {
    ws.close();
  };
}

connectWebSocket();

document.addEventListener("visibilitychange", () => {
  if (!document.hidden) {
    fetch("/api/status")
      .then((r) => r.json())
      .then(updateStatus);
  }
});

setInterval(() => {
  const age = Date.now() - lastWsMessage;

  if (age > 5000) {
    console.log("WebSocket timeout");

    location.reload();
  }
}, 1000);
