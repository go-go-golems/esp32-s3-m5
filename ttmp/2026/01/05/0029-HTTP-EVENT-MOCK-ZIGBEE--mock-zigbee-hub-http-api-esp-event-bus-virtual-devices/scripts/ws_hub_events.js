#!/usr/bin/env node
/**
 * WebSocket client for 0029 protobuf event stream.
 *
 * Usage:
 *   node ws_hub_events.js --host 192.168.0.18
 *   node ws_hub_events.js --url ws://192.168.0.18/v1/events/ws --duration 12000
 *   node ws_hub_events.js --host 192.168.0.18 --seed
 *   node ws_hub_events.js --host 192.168.0.18 --head
 *   node ws_hub_events.js --host 192.168.0.18 --decode
 *
 * Notes:
 * - Node 22+ provides a global WebSocket implementation (no deps).
 * - This script intentionally avoids requiring protobuf libraries; the `--decode`
 *   mode uses a minimal proto3 decoder for hub.v1.HubEvent matching:
 *     `0029-mock-zigbee-http-hub/components/hub_proto/defs/hub_events.proto`.
 */
/* global WebSocket, fetch */

function usage(exitCode = 1) {
  const msg = `
ws_hub_events.js

Flags:
  --host <ip>        Default: 192.168.0.18
  --url <ws-url>     Overrides host; default: ws://<host>/v1/events/ws
  --duration <ms>    Default: 12000
  --send <text>      Send one TEXT frame after connect (default: none)
  --seed             POST /v1/debug/seed once after connect
  --head             Print first 12 bytes as hex per binary frame
  --decode           Decode hub.v1.HubEvent (minimal decoder) and print JSON
  --quiet            Don't print per-frame lines, only summary
`;
  console.error(msg.trim());
  process.exit(exitCode);
}

function getArg(flag) {
  const i = process.argv.indexOf(flag);
  if (i === -1) return null;
  const v = process.argv[i + 1];
  if (!v || v.startsWith("--")) return null;
  return v;
}

function hasFlag(flag) {
  return process.argv.includes(flag);
}

function hexHead(buf, n = 12) {
  return Buffer.from(buf.subarray(0, Math.min(n, buf.length))).toString("hex");
}

const host = getArg("--host") ?? "192.168.0.18";
const url = getArg("--url") ?? `ws://${host}/v1/events/ws`;
const durationMs = Number(getArg("--duration") ?? "12000");
if (!Number.isFinite(durationMs) || durationMs <= 0) usage();

const sendText = getArg("--send");
const doSeed = hasFlag("--seed");
const doHead = hasFlag("--head");
const doDecode = hasFlag("--decode");
const quiet = hasFlag("--quiet");

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
      case 0:
        this.varint();
        return;
      case 1:
        this.pos += 8;
        return;
      case 2: {
        const n = Number(this.varint());
        this.pos += n;
        return;
      }
      case 5:
        this.pos += 4;
        return;
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

function decodeHubEvent(buf) {
  const r = new PbReader(new Uint8Array(buf));
  const out = {};
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
        out.tsUs = r.varint().toString();
        break;
      case 10: {
        const n = Number(r.varint());
        out.payloadType = "device";
        out.device = decodeDevice(new PbReader(r.bytes(n)));
        break;
      }
      default:
        r.skip(wt);
    }
  }
  return out;
}

async function maybeSeed(baseUrl) {
  try {
    const r = await fetch(`${baseUrl}/v1/debug/seed`, { method: "POST" });
    const t = await r.text();
    if (!r.ok) throw new Error(`${r.status} ${t}`);
    if (!quiet) console.log("seed ok:", t.trim());
  } catch (err) {
    console.error("seed failed:", String(err));
  }
}

let msgs = 0;
let bytes = 0;

const ws = new WebSocket(url);
ws.binaryType = "arraybuffer";

ws.onopen = async () => {
  console.log("ws open:", url);
  if (sendText) ws.send(sendText);

  if (doSeed) {
    const base = url.replace(/^ws:/, "http:").replace(/^wss:/, "https:").replace(/\/v1\/events\/ws$/, "");
    await maybeSeed(base);
  }

  setTimeout(() => ws.close(), durationMs);
};

ws.onmessage = (ev) => {
  if (typeof ev.data === "string") {
    if (!quiet) console.log("text:", ev.data.length);
    return;
  }
  if (!(ev.data instanceof ArrayBuffer)) return;

  const b = Buffer.from(ev.data);
  msgs++;
  bytes += b.length;

  if (quiet) return;
  if (doDecode) {
    try {
      console.log(JSON.stringify(decodeHubEvent(b), null, 0));
    } catch (err) {
      console.log(JSON.stringify({ decodeError: String(err), len: b.length }, null, 0));
    }
    return;
  }

  if (doHead) {
    console.log(`bin: len=${b.length} head=${hexHead(b)}`);
  } else {
    console.log(`bin: len=${b.length}`);
  }
};

ws.onerror = (ev) => {
  console.error("ws error:", ev?.message ?? String(ev));
};

ws.onclose = (ev) => {
  console.log(`ws close: code=${ev.code} msgs=${msgs} bytes=${bytes}`);
};

