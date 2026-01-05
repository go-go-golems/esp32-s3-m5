#!/usr/bin/env node
/**
 * Minimal HTTP+protobuf client for 0029 (no deps).
 *
 * Examples:
 *   node http_pb_hub.js --host 192.168.0.18 list
 *   node http_pb_hub.js --host 192.168.0.18 add --type plug --name desk --caps onoff,power
 *   node http_pb_hub.js --host 192.168.0.18 set --id 1 --on true
 *   node http_pb_hub.js --host 192.168.0.18 set --id 1 --level 42
 *
 * The protobuf schema lives at:
 *   0029-mock-zigbee-http-hub/components/hub_proto/defs/hub_events.proto
 */

/* global fetch */

function usage(exitCode = 1) {
  console.error(`
http_pb_hub.js

Global flags:
  --host <ip>   Default: 192.168.0.18

Commands:
  list
  add --type <plug|bulb|temp_sensor> --name <name> [--caps onoff,level,power,temperature]
  set --id <id> [--on true|false] [--level 0..100]
  interview --id <id>
  scene-trigger --id <scene_id>
`.trim());
  process.exit(exitCode);
}

function getArg(args, flag) {
  const i = args.indexOf(flag);
  if (i === -1) return null;
  const v = args[i + 1];
  if (!v || v.startsWith("--")) return null;
  return v;
}

function hasFlag(args, flag) {
  return args.includes(flag);
}

function varintEncode(n) {
  let x = BigInt(n);
  if (x < 0) throw new Error("varintEncode: negative not supported");
  const out = [];
  while (x >= 0x80n) {
    out.push(Number((x & 0x7fn) | 0x80n));
    x >>= 7n;
  }
  out.push(Number(x));
  return Uint8Array.from(out);
}

function key(fieldNumber, wireType) {
  return varintEncode((fieldNumber << 3) | wireType);
}

function concat(chunks) {
  const total = chunks.reduce((a, c) => a + c.length, 0);
  const out = new Uint8Array(total);
  let off = 0;
  for (const c of chunks) {
    out.set(c, off);
    off += c.length;
  }
  return out;
}

function encodeString(fieldNumber, s) {
  const b = new TextEncoder().encode(s);
  return concat([key(fieldNumber, 2), varintEncode(b.length), b]);
}

function encodeVarint(fieldNumber, n) {
  return concat([key(fieldNumber, 0), varintEncode(n)]);
}

function encodeBool(fieldNumber, b) {
  return encodeVarint(fieldNumber, b ? 1 : 0);
}

function encodeCmdDeviceAdd({ type, caps, name }) {
  // CmdDeviceAdd:
  //   uint64 req_id = 1;  (ignored by firmware for HTTP)
  //   DeviceType type = 2;
  //   uint32 caps = 3;
  //   string name = 4;
  const chunks = [];
  chunks.push(encodeVarint(1, 0)); // req_id
  chunks.push(encodeVarint(2, type));
  chunks.push(encodeVarint(3, caps));
  chunks.push(encodeString(4, name));
  return concat(chunks);
}

function encodeCmdDeviceSet({ deviceId, hasOn, on, hasLevel, level }) {
  // CmdDeviceSet:
  //   uint64 req_id = 1;      (ignored by firmware for HTTP)
  //   uint32 device_id = 2;   (ignored; taken from URL)
  //   bool has_on = 3;
  //   bool on = 4;
  //   bool has_level = 5;
  //   uint32 level = 6;
  const chunks = [];
  chunks.push(encodeVarint(1, 0)); // req_id
  chunks.push(encodeVarint(2, deviceId));
  chunks.push(encodeBool(3, hasOn));
  if (hasOn) chunks.push(encodeBool(4, on));
  chunks.push(encodeBool(5, hasLevel));
  if (hasLevel) chunks.push(encodeVarint(6, level));
  return concat(chunks);
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

const DeviceType = {
  plug: 1,
  bulb: 2,
  temp_sensor: 3,
};

const CapBits = {
  onoff: 1 << 0,
  level: 1 << 1,
  power: 1 << 2,
  temperature: 1 << 3,
};

function decodeDevice(buf) {
  const r = new PbReader(buf);
  const out = {};
  while (!r.eof()) {
    const k = Number(r.varint());
    const field = k >>> 3;
    const wt = k & 7;
    switch (field) {
      case 1:
        out.id = Number(r.varint());
        break;
      case 2:
        out.type = Number(r.varint());
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

function decodeDeviceList(buf) {
  const r = new PbReader(buf);
  const out = { devices: [] };
  while (!r.eof()) {
    const k = Number(r.varint());
    const field = k >>> 3;
    const wt = k & 7;
    if (field === 1 && wt === 2) {
      const n = Number(r.varint());
      out.devices.push(decodeDevice(r.bytes(n)));
    } else {
      r.skip(wt);
    }
  }
  return out;
}

function decodeReplyStatus(buf) {
  const r = new PbReader(buf);
  const out = {};
  while (!r.eof()) {
    const k = Number(r.varint());
    const field = k >>> 3;
    const wt = k & 7;
    switch (field) {
      case 1:
        out.ok = Number(r.varint()) !== 0;
        break;
      case 2:
        out.status = Number(r.varint());
        break;
      default:
        r.skip(wt);
    }
  }
  return out;
}

async function http(method, url, body) {
  const r = await fetch(url, {
    method,
    headers: body ? { "Content-Type": "application/x-protobuf" } : undefined,
    body: body ? Buffer.from(body) : undefined,
  });
  const b = new Uint8Array(await r.arrayBuffer());
  return { status: r.status, ok: r.ok, contentType: r.headers.get("content-type") ?? "", body: b };
}

function parseCaps(s) {
  if (!s) return 0;
  let mask = 0;
  for (const part of s.split(",").map((x) => x.trim()).filter(Boolean)) {
    if (!(part in CapBits)) throw new Error(`unknown cap: ${part}`);
    mask |= CapBits[part];
  }
  return mask >>> 0;
}

async function main() {
  const args = process.argv.slice(2);

  let host = "192.168.0.18";
  let i = 0;
  while (i < args.length) {
    const a = args[i];
    if (a === "--host") {
      const v = args[i + 1];
      if (!v) usage();
      host = v;
      i += 2;
      continue;
    }
    if (a.startsWith("--")) {
      // Unknown global flag; stop parsing globals so the command can handle it.
      break;
    }
    break;
  }

  const base = `http://${host}`;

  const cmd = args[i];
  if (!cmd) usage();

  if (cmd === "list") {
    const r = await http("GET", `${base}/v1/devices`);
    if (!r.ok) throw new Error(`HTTP ${r.status}: ${Buffer.from(r.body).toString("utf8")}`);
    console.log(JSON.stringify(decodeDeviceList(r.body), null, 2));
    return;
  }

  if (cmd === "add") {
    const typeStr = getArg(args, "--type");
    const name = getArg(args, "--name") ?? "";
    if (!typeStr || !(typeStr in DeviceType) || !name) usage();
    const caps = parseCaps(getArg(args, "--caps"));
    const msg = encodeCmdDeviceAdd({ type: DeviceType[typeStr], caps, name });
    const r = await http("POST", `${base}/v1/devices`, msg);
    if (!r.ok) throw new Error(`HTTP ${r.status}: ${Buffer.from(r.body).toString("utf8")}`);
    console.log(JSON.stringify(decodeDevice(r.body), null, 2));
    return;
  }

  if (cmd === "set") {
    const id = Number(getArg(args, "--id") ?? "");
    if (!Number.isFinite(id) || id <= 0) usage();
    const onStr = getArg(args, "--on");
    const levelStr = getArg(args, "--level");
    const hasOn = onStr !== null;
    const hasLevel = levelStr !== null;
    if (!hasOn && !hasLevel) usage();

    const on = hasOn ? onStr === "true" || onStr === "1" : false;
    const level = hasLevel ? Number(levelStr) : 0;
    const msg = encodeCmdDeviceSet({ deviceId: id, hasOn, on, hasLevel, level });
    const r = await http("POST", `${base}/v1/devices/${id}/set`, msg);
    if (!r.ok) throw new Error(`HTTP ${r.status}: ${Buffer.from(r.body).toString("utf8")}`);
    console.log(JSON.stringify(decodeReplyStatus(r.body), null, 2));
    return;
  }

  if (cmd === "interview") {
    const id = Number(getArg(args, "--id") ?? "");
    if (!Number.isFinite(id) || id <= 0) usage();
    const r = await http("POST", `${base}/v1/devices/${id}/interview`);
    if (!r.ok) throw new Error(`HTTP ${r.status}: ${Buffer.from(r.body).toString("utf8")}`);
    console.log(JSON.stringify(decodeReplyStatus(r.body), null, 2));
    return;
  }

  if (cmd === "scene-trigger") {
    const id = Number(getArg(args, "--id") ?? "");
    if (!Number.isFinite(id) || id <= 0) usage();
    const r = await http("POST", `${base}/v1/scenes/${id}/trigger`);
    if (!r.ok) throw new Error(`HTTP ${r.status}: ${Buffer.from(r.body).toString("utf8")}`);
    console.log(JSON.stringify(decodeReplyStatus(r.body), null, 2));
    return;
  }

  usage();
}

main().catch((err) => {
  console.error(String(err));
  process.exit(1);
});
