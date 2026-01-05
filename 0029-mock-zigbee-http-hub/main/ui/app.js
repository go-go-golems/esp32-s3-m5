/* Minimal browser-side protobuf decoder for hub.v1.HubEvent (no external deps). */

const WS_PATH = "/v1/events/ws";
const MAX_EVENTS = 250;

const EventId = {
  0: "EVENT_ID_UNSPECIFIED",
  1: "HUB_CMD_DEVICE_ADD",
  2: "HUB_CMD_DEVICE_REMOVE",
  3: "HUB_CMD_DEVICE_INTERVIEW",
  4: "HUB_CMD_DEVICE_SET",
  5: "HUB_CMD_SCENE_TRIGGER",
  6: "HUB_EVT_DEVICE_ADDED",
  7: "HUB_EVT_DEVICE_REMOVED",
  8: "HUB_EVT_DEVICE_INTERVIEWED",
  9: "HUB_EVT_DEVICE_STATE",
  10: "HUB_EVT_DEVICE_REPORT",
};

const DeviceType = {
  0: "DEVICE_TYPE_UNSPECIFIED",
  1: "HUB_DEVICE_PLUG",
  2: "HUB_DEVICE_BULB",
  3: "HUB_DEVICE_TEMP_SENSOR",
};

function nowIso() {
  return new Date().toISOString().replace("T", " ").replace("Z", "");
}

class PbReader {
  constructor(u8) {
    this.u8 = u8;
    this.pos = 0;
    this.view = new DataView(u8.buffer, u8.byteOffset, u8.byteLength);
    this.td = new TextDecoder();
  }

  eof() {
    return this.pos >= this.u8.length;
  }

  varint() {
    let x = 0n;
    let shift = 0n;
    for (let i = 0; i < 10; i++) {
      if (this.pos >= this.u8.length) throw new Error("varint overflow");
      const b = BigInt(this.u8[this.pos++]);
      x |= (b & 0x7fn) << shift;
      if ((b & 0x80n) === 0n) return x;
      shift += 7n;
    }
    throw new Error("varint too long");
  }

  bytes(n) {
    if (this.pos + n > this.u8.length) throw new Error("short read");
    const out = this.u8.subarray(this.pos, this.pos + n);
    this.pos += n;
    return out;
  }

  string(n) {
    return this.td.decode(this.bytes(n));
  }

  float32() {
    if (this.pos + 4 > this.u8.length) throw new Error("short float32");
    const v = this.view.getFloat32(this.pos, true);
    this.pos += 4;
    return v;
  }

  skip(wireType) {
    switch (wireType) {
      case 0: {
        this.varint();
        return;
      }
      case 1: {
        this.pos += 8;
        return;
      }
      case 2: {
        const n = Number(this.varint());
        this.pos += n;
        return;
      }
      case 5: {
        this.pos += 4;
        return;
      }
      default:
        throw new Error(`unsupported wire type ${wireType}`);
    }
  }
}

function decodeDevice(r) {
  const out = {};
  while (!r.eof()) {
    const key = Number(r.varint());
    const field = key >>> 3;
    const wt = key & 7;
    switch (field) {
      case 1:
        out.id = Number(r.varint());
        break;
      case 2:
        out.type = DeviceType[Number(r.varint())] ?? "UNKNOWN";
        break;
      case 3:
        out.caps = Number(r.varint());
        break;
      case 4: {
        const n = Number(r.varint());
        out.name = r.string(n);
        break;
      }
      case 10:
        out.on = Number(r.varint()) !== 0;
        break;
      case 11:
        out.level = Number(r.varint());
        break;
      case 12:
        out.powerW = r.float32();
        break;
      case 13:
        out.temperatureC = r.float32();
        break;
      default:
        r.skip(wt);
    }
  }
  return out;
}

function decodeDeviceState(r) {
  const out = {};
  while (!r.eof()) {
    const key = Number(r.varint());
    const field = key >>> 3;
    const wt = key & 7;
    switch (field) {
      case 1:
        out.tsUs = r.varint();
        break;
      case 2:
        out.deviceId = Number(r.varint());
        break;
      case 3:
        out.on = Number(r.varint()) !== 0;
        break;
      case 4:
        out.level = Number(r.varint());
        break;
      default:
        r.skip(wt);
    }
  }
  return out;
}

function decodeDeviceReport(r) {
  const out = {};
  while (!r.eof()) {
    const key = Number(r.varint());
    const field = key >>> 3;
    const wt = key & 7;
    switch (field) {
      case 1:
        out.tsUs = r.varint();
        break;
      case 2:
        out.deviceId = Number(r.varint());
        break;
      case 3:
        out.hasPower = Number(r.varint()) !== 0;
        break;
      case 4:
        out.powerW = r.float32();
        break;
      case 5:
        out.hasTemperature = Number(r.varint()) !== 0;
        break;
      case 6:
        out.temperatureC = r.float32();
        break;
      default:
        r.skip(wt);
    }
  }
  return out;
}

function decodeCmdDeviceAdd(r) {
  const out = {};
  while (!r.eof()) {
    const key = Number(r.varint());
    const field = key >>> 3;
    const wt = key & 7;
    switch (field) {
      case 1:
        out.reqId = r.varint();
        break;
      case 2:
        out.type = DeviceType[Number(r.varint())] ?? "UNKNOWN";
        break;
      case 3:
        out.caps = Number(r.varint());
        break;
      case 4: {
        const n = Number(r.varint());
        out.name = r.string(n);
        break;
      }
      default:
        r.skip(wt);
    }
  }
  return out;
}

function decodeCmdDeviceSet(r) {
  const out = {};
  while (!r.eof()) {
    const key = Number(r.varint());
    const field = key >>> 3;
    const wt = key & 7;
    switch (field) {
      case 1:
        out.reqId = r.varint();
        break;
      case 2:
        out.deviceId = Number(r.varint());
        break;
      case 3:
        out.hasOn = Number(r.varint()) !== 0;
        break;
      case 4:
        out.on = Number(r.varint()) !== 0;
        break;
      case 5:
        out.hasLevel = Number(r.varint()) !== 0;
        break;
      case 6:
        out.level = Number(r.varint());
        break;
      default:
        r.skip(wt);
    }
  }
  return out;
}

function decodeCmdDeviceInterview(r) {
  const out = {};
  while (!r.eof()) {
    const key = Number(r.varint());
    const field = key >>> 3;
    const wt = key & 7;
    switch (field) {
      case 1:
        out.reqId = r.varint();
        break;
      case 2:
        out.deviceId = Number(r.varint());
        break;
      default:
        r.skip(wt);
    }
  }
  return out;
}

function decodeCmdSceneTrigger(r) {
  const out = {};
  while (!r.eof()) {
    const key = Number(r.varint());
    const field = key >>> 3;
    const wt = key & 7;
    switch (field) {
      case 1:
        out.reqId = r.varint();
        break;
      case 2:
        out.sceneId = Number(r.varint());
        break;
      default:
        r.skip(wt);
    }
  }
  return out;
}

function decodeHubEvent(buf) {
  const r = new PbReader(new Uint8Array(buf));
  const out = { recvIso: nowIso() };

  while (!r.eof()) {
    const key = Number(r.varint());
    const field = key >>> 3;
    const wt = key & 7;
    switch (field) {
      case 1:
        out.schemaVersion = Number(r.varint());
        break;
      case 2: {
        const id = Number(r.varint());
        out.id = id;
        out.eventId = EventId[id] ?? `UNKNOWN_${id}`;
        break;
      }
      case 3:
        out.tsUs = r.varint();
        break;
      case 10: {
        const n = Number(r.varint());
        out.payloadType = "device";
        out.device = decodeDevice(new PbReader(r.bytes(n)));
        break;
      }
      case 11: {
        const n = Number(r.varint());
        out.payloadType = "device_state";
        out.deviceState = decodeDeviceState(new PbReader(r.bytes(n)));
        break;
      }
      case 12: {
        const n = Number(r.varint());
        out.payloadType = "device_report";
        out.deviceReport = decodeDeviceReport(new PbReader(r.bytes(n)));
        break;
      }
      case 20: {
        const n = Number(r.varint());
        out.payloadType = "cmd_device_add";
        out.cmdDeviceAdd = decodeCmdDeviceAdd(new PbReader(r.bytes(n)));
        break;
      }
      case 21: {
        const n = Number(r.varint());
        out.payloadType = "cmd_device_set";
        out.cmdDeviceSet = decodeCmdDeviceSet(new PbReader(r.bytes(n)));
        break;
      }
      case 22: {
        const n = Number(r.varint());
        out.payloadType = "cmd_device_interview";
        out.cmdDeviceInterview = decodeCmdDeviceInterview(new PbReader(r.bytes(n)));
        break;
      }
      case 23: {
        const n = Number(r.varint());
        out.payloadType = "cmd_scene_trigger";
        out.cmdSceneTrigger = decodeCmdSceneTrigger(new PbReader(r.bytes(n)));
        break;
      }
      default:
        r.skip(wt);
    }
  }

  return out;
}

function safeJson(x) {
  return JSON.stringify(
    x,
    (_, v) => {
      if (typeof v === "bigint") return v.toString();
      return v;
    },
    2,
  );
}

let ws = null;
let events = [];
let selected = -1;

const statusDot = document.getElementById("statusDot");
const statusText = document.getElementById("statusText");
const msgCount = document.getElementById("msgCount");
const eventsUl = document.getElementById("events");
const detail = document.getElementById("detail");

const connectBtn = document.getElementById("connectBtn");
const disconnectBtn = document.getElementById("disconnectBtn");
const clearBtn = document.getElementById("clearBtn");
const seedBtn = document.getElementById("seedBtn");

function setStatus(ok, text) {
  statusDot.classList.toggle("ok", ok);
  statusText.textContent = text;
}

function renderList() {
  msgCount.textContent = String(events.length);
  eventsUl.innerHTML = "";
  for (let i = 0; i < events.length; i++) {
    const e = events[i];
    const li = document.createElement("li");
    if (i === selected) li.classList.add("active");

    const top = document.createElement("div");
    top.className = "row";
    top.innerHTML = `<div class="v">${e.eventId ?? "?"}</div><div class="k">${e.recvIso}</div>`;

    const bot = document.createElement("div");
    bot.className = "row";
    bot.innerHTML = `<div class="k">${e.payloadType ?? ""}</div><div class="k">len=${e._rawLen}</div>`;

    li.appendChild(top);
    li.appendChild(bot);

    li.onclick = () => {
      selected = i;
      detail.textContent = safeJson(events[i]);
      renderList();
    };

    eventsUl.appendChild(li);
  }
}

function connect() {
  if (ws) return;
  const proto = location.protocol === "https:" ? "wss:" : "ws:";
  const url = `${proto}//${location.host}${WS_PATH}`;

  ws = new WebSocket(url);
  ws.binaryType = "arraybuffer";

  ws.onopen = () => {
    setStatus(true, `Connected (${WS_PATH})`);
    connectBtn.disabled = true;
    disconnectBtn.disabled = false;
  };

  ws.onmessage = (ev) => {
    if (!(ev.data instanceof ArrayBuffer)) return;
    let decoded = null;
    try {
      decoded = decodeHubEvent(ev.data);
      decoded._rawLen = ev.data.byteLength;
    } catch (err) {
      decoded = { recvIso: nowIso(), decodeError: String(err), _rawLen: ev.data.byteLength };
    }
    events.unshift(decoded);
    if (events.length > MAX_EVENTS) events.pop();
    if (selected === -1) selected = 0;
    renderList();
  };

  ws.onclose = () => {
    ws = null;
    setStatus(false, "Disconnected");
    connectBtn.disabled = false;
    disconnectBtn.disabled = true;
  };

  ws.onerror = () => {
    setStatus(false, "WS error");
  };
}

function disconnect() {
  if (!ws) return;
  ws.close();
}

connectBtn.onclick = connect;
disconnectBtn.onclick = disconnect;
clearBtn.onclick = () => {
  events = [];
  selected = -1;
  detail.textContent = "Click an event to inspect decoded protobuf.";
  renderList();
};

seedBtn.onclick = async () => {
  seedBtn.disabled = true;
  try {
    const r = await fetch("/v1/debug/seed", { method: "POST" });
    if (!r.ok) {
      const t = await r.text();
      alert(`seed failed: ${r.status} ${t}`);
    }
  } catch (err) {
    alert(`seed failed: ${String(err)}`);
  } finally {
    seedBtn.disabled = false;
  }
};

setStatus(false, "Disconnected");

