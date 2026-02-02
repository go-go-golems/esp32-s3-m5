You can build a pretty nice Go toolchain on top of your **nRF 802.15.4 sniffer** without reinventing Zigbee—*and* you can choose how “pure Go” you want it to be.

Also, small correction that matters for design: Zigbee generally **doesn’t do a public-key “key pair exchange”** during joining; it’s mostly **symmetric AES-128** keys (a link key used during join + a network key used for normal encrypted traffic). (This affects what you can/should try to “sniff”.)

Below are **three practical architectures**, from fastest-to-build to most “native Go”.

---

## Architecture 1: Go = capture + orchestrate Wireshark dissectors (tshark)

**Fastest path to something powerful**: your Go program captures frames to **pcapng** and/or streams them, then runs `tshark` to decode into JSON that you can post-process into whatever CLI UX you want.

Why this is great:

* You get Wireshark’s mature Zigbee dissection “for free”.
* You can stay CLI-only by parsing `tshark -T json` output.
* You can still add your own “semantic” layer (join timeline, key-transport packets, ZCL attribute reads/writes, etc.).

Wireshark already has rich filter fields for 802.15.4 TAP metadata (`wpan-tap.*`) and WPAN decoding. ([Wireshark][1])

**Implementation sketch**

1. Capture to `*.pcapng` using Go (see next section).
2. Decode:

   ```bash
   tshark -r capture.pcapng -T json > frames.json
   ```
3. Your Go code reads JSON and builds:

   * “join session timeline”
   * per-device traffic summaries
   * ZCL cluster/attribute changes
   * suspicious patterns (rapid relay retries, rejoin storms, etc.)

---

## Architecture 2: Go writes pcapng (802.15.4 TAP) directly from the nRF sniffer

Your sniffer is already meant to feed Wireshark via an **extcap** script; you can port that path to Go.

Key facts:

* The Nordic sniffer ecosystem commonly uses a **Wireshark extcap script** (`nrf802154_sniffer.py`) and lets you select channel + metadata format. ([GitHub][2])
* Wireshark notes that the **nRF 802.15.4 sniffer provides channel/RSSI/LQI metadata using the IEEE 802.15.4 TAP link type**. ([Wireshark Wiki][3])
* The pcap linktype for `LINKTYPE_IEEE802_15_4_TAP` is **283**. ([IETF][4])
* In Go, you can write **pcapng** with `pcapgo.NgWriter` (no libpcap required). ([Go Packages][5])

### What you build in Go

A `zbcap` CLI like:

* `zbcap capture --port /dev/serial/by-id/usb-Nordic_... --channel 15 --out join.pcapng`
* `zbcap live --port ... --channel 15 --stdout-pcapng | wireshark -k -i -`
* `zbcap decode --in join.pcapng --tshark-json` (architecture 1 combined)

### Where you get the “serial protocol”

Don’t guess it—treat the extcap script as the spec:

* Read `nrf802154_sniffer.py` in the repo and port the framing/commands 1:1. ([GitHub][2])
  This is the quickest way to make your Go capture reliable.

### pcapng writing detail

Write an Interface Description Block with linktype **283**, then write Enhanced Packet Blocks containing:

* the 802.15.4 TAP pseudoheader (TLVs with channel/RSSI/LQI)
* followed by the raw 802.15.4 MAC frame bytes

(You can start with *just* the MAC frame bytes if you want MVP, but TAP metadata makes Wireshark/tshark way nicer.)

---

## Architecture 3: “All Go” Zigbee parsing + (optional) decryption

This is the heaviest path but also fun if you want full control.

### Parsing layers (incrementally)

1. **802.15.4 MAC** (addresses, PAN IDs, frame control)
2. **Zigbee NWK**
3. **APS**
4. **ZCL / ZDO**

There isn’t a universally dominant Go Zigbee parser ecosystem, but there *is* useful Go code to reuse:

* **ShimmeringBee Z-Stack / Zigbee**: Go libs oriented around TI Z-Stack/ZNP integration and some Zigbee/ZCL handling. ([GitHub][6])
  This is relevant because your **Sonoff Zigbee 3.0 USB Dongle Plus** is typically used with TI Z-Stack coordinator firmware in Zigbee2MQTT-style setups.

### Decryption (only for your own network/devices)

Zigbee uses **AES-CCM*** (a small variation of CCM). Kudelski’s overview and various technical writeups summarize the nonce composition and MIC behavior. ([Kudelski Security Research][7])
The **nonce** used for AES-CCM* is **13 bytes**:
**source IEEE address (8) + frame counter (4) + security control (1)**. ([Lucidar][8])

Go doesn’t ship CCM/CCM* in the stdlib, so you typically:

* use a CCM implementation as a starting point (e.g. `github.com/pschlump/AesCCM`) ([Go Packages][9])
* then adapt it to Zigbee’s CCM* requirements (security control / MIC sizes / authenticated data construction).

If you want a more “maintained crypto surface area” for CCM primitives, some COSE libraries include AES-CCM implementations (though you still need Zigbee-specific framing/nonce/AAD rules). ([GitHub][10])

**Important practical point:** you don’t “recover keys” from thin air. For legitimate decryption you typically:

* use your **known network key** (from your coordinator/Zigbee2MQTT backup/config), and
* sometimes the well-known joining link key `ZigbeeAlliance09` for the join phase (the Nordic sniffer README even shows it as a preconfigured key in Wireshark). ([GitHub][2])

---

## A really productive Go plan (I’d do this)

If your goal is security research + CLI ergonomics, without months of protocol reimplementation:

### Phase 1 (1–2 evenings)

* Go tool that captures from the nRF sniffer and writes **pcapng (linktype 283)** using `pcapgo.NgWriter`. ([Go Packages][5])
* Optional: also stream ZEP/pcapng to stdout for live Wireshark.

### Phase 2

* Add a `decode` command that shells out to `tshark` and consumes JSON.
* Implement useful derived views:

  * join timeline
  * device → clusters touched
  * “what messages happened right before device joined”
  * “power plug reporting cadence changes / bursts”

### Phase 3 (only if you really need it)

* Implement native Go parsing for the specific slices you care about (e.g., just enough ZCL to decode metering clusters), and use decryption with your own keys.

---

## One question that changes the best design

Are you sniffing **Zigbee** (your smart plug mesh) or **Thread**? The Nordic sniffer README talks about Thread config too, but your use-case sounds Zigbee. ([GitHub][2])

If you confirm **Zigbee** and tell me:

* which Zigbee channel your coordinator is on (11/15/20/25), and
* whether you want **pcapng output** or **tshark JSON events** as the main “API”,

…I’ll sketch the exact Go module layout + the CLI commands + the minimal pcapng writing code structure (without getting fancy).

[1]: https://www.wireshark.org/docs/dfref/w/wpan.html?utm_source=chatgpt.com "Wireshark • Go Deep | Display Filter Reference: IEEE 802.15.4 Low-Rate Wireless PAN"
[2]: https://github.com/stig-bjorlykke/nRF-802.15.4-sniffer "GitHub - stig-bjorlykke/nRF-802.15.4-sniffer: nRF-based 802.15.4 sniffer (firmware and software)"
[3]: https://wiki.wireshark.org/IEEE_802.15.4?utm_source=chatgpt.com "IEEE_802.15.4 - Wireshark Wiki"
[4]: https://www.ietf.org/archive/id/draft-ietf-opsawg-pcaplinktype-06.html?utm_source=chatgpt.com "Link-Layer Types for PCAP and PCAPNG Capture File Formats"
[5]: https://pkg.go.dev/github.com/hatching/gopacket/pcapgo?utm_source=chatgpt.com "pcapgo package - github.com/hatching/gopacket/pcapgo - Go Packages"
[6]: https://github.com/shimmeringbee/zstack?utm_source=chatgpt.com "GitHub - shimmeringbee/zstack: Implementation of a ZNP and support code designed to interface with Texas Instruments Z-Stack, written in Go."
[7]: https://research.kudelskisecurity.com/2017/11/08/zigbee-security-basics-part-2/?utm_source=chatgpt.com "ZigBee Security: Basics (Part 2) – Kudelski Security Research"
[8]: https://lucidar.me/en/zigbee/zigbee-frame-encryption-with-aes-128-ccm/?utm_source=chatgpt.com "ZigBee frame encryption with AES-128-CCM* | Lulu's blog"
[9]: https://pkg.go.dev/github.com/pschlump/AesCCM?utm_source=chatgpt.com "aesccm package - github.com/pschlump/AesCCM - Go Packages"
[10]: https://github.com/ldclabs/cose?utm_source=chatgpt.com "GitHub - ldclabs/cose: 📧 Implemented Keys, Algorithms (RFC9053), COSE (RFC9052) and CWT (RFC8392) in Go."

---

Yep — that write-up is mostly pointing at the *right families* of tools, but a couple of details are easy to misread. Here’s how it really maps to **your setup (Nordic Semiconductor nRF sniffer + Sonoff ZBDongle-P coordinator)**.

## 1) “Zigbee2MQTT exposes ZEP over UDP 17754”

**Core idea is valid, but the wording is misleading.**

* **UDP port 17754 is the default ZEP port**, and Wireshark decodes Zigbee traffic from packets arriving there. ([Zigbee2MQTT][1])
* But **Zigbee2MQTT itself isn’t the thing that magically emits ZEP**. What Zigbee2MQTT’s sniffing page shows is: *once you run a sniffer tool that sends ZEP to your machine*, you can capture it on loopback and filter `udp.port==17754`. ([Zigbee2MQTT][1])
* ZEP is just a **UDP encapsulation of 802.15.4 packets** (Wireshark wiki even sketches the packet layout conceptually). ([Wireshark Wiki][2])

**So your `tcpdump -i lo udp port 17754` trick works only if you have some sniffer/bridge program actually sending ZEP to 127.0.0.1:17754.** ([Zigbee2MQTT][1])

### How that helps you

* If you want a “*CLI-first feed*” that still plays perfectly with Wireshark/tshark, **ZEP is a great intermediate format**.
* But with your current hardware, the cleanest source is still your nRF sniffer doing true OTA capture (next section).

---

## 2) True over-the-air sniffing

This is where your nRF sniffer shines.

Wireshark explicitly calls out that the **nRF 802.15.4 sniffer provides an extcap capture interface** and can include **channel/RSSI/LQI metadata** using the **IEEE 802.15.4 TAP link type**. ([Wireshark Wiki][2])
The nRF sniffer repos also document selecting **channel + serial port** via the extcap UI/options. ([GitHub][3])

**Implication for Go:** the most “native” thing you can build is:

* read frames from `/dev/serial/by-id/usb-Nordic_...`
* write **pcapng** with linktype “802.15.4 TAP” (so Wireshark gets metadata)
* optionally also offer a “live” mode (pipe/stream)

(And you can still add a “decode” subcommand that shells out to tshark if you want Wireshark-level decoding without reimplementing Zigbee.)

---

## 3) CC2531 + `whsniff` / CC2531 extcap

Your pasted notes are accurate **for CC2531 workflows**, but you don’t currently have a CC2531.

* `whsniff` is exactly what that snippet says: **reads CC2531 sniffer firmware packets and writes PCAP to stdout**; it’s built for piping into Wireshark or saving to a file. ([GitHub][4])
* `wireshark-cc2531` is an extcap interface for CC2531 with TI sniffer firmware. ([GitHub][5])

**How it compares to your nRF sniffer**

* CC2531 + whsniff is popular because it’s “pipe-friendly CLI” out of the box. ([GitHub][4])
* Your nRF sniffer can be just as CLI-friendly, you just need to either:

  * drive the extcap program headless, or
  * reimplement the serial protocol in Go and output pcapng.

---

## 4) KillerBee (Python toolkit)

This is real tooling, but it matters what hardware you plan to use and what you want to do with it.

* KillerBee is positioned as a security research toolkit and includes a bunch of tools/scripts. ([River Loop Security][6])
* Hardware support is **not “everything”**; the repo lists supported devices (ApiMote, RZUSB stick, TelosB, etc.) and points you to firmware notes. ([GitHub][7])
* CC2531 can be used with KillerBee if you flash a KillerBee-compatible firmware (e.g., Bumblebee). ([GitHub][8])

**For your purposes (your own network analysis):** KillerBee is most useful as

* a capture harness (when you’re using supported sniffers),
* plus some higher-level Zigbee/802.15.4 analysis conveniences.

I’m not going to walk through “attack tool” workflows; but for legitimate auditing of your own devices, it’s fine as another way to produce captures or parse them.

---

## 5) Decryption keys (what’s safe + practical)

Your pasted section on keys aligns with the Zigbee2MQTT guidance:

* Wireshark needs a **Trust Center link key** + the **network key** to decrypt most useful payloads. ([Zigbee2MQTT][1])
* Zigbee2MQTT also notes that older versions used a known default network key and newer versions generate a random one. ([Zigbee2MQTT][9])

For your own network: the sane workflow is **extract your network key from your coordinator/Zigbee2MQTT backup/config** and load it into Wireshark/tshark. ([Zigbee2MQTT][1])

---

# How I’d build “your own tooling” in Go on top of these

Think in terms of **pluggable capture backends** that all normalize into the same internal event stream:

### Capture backends (choose 1–3)

1. **nRF serial backend** (your current OTA sniffer)

   * input: `/dev/serial/by-id/usb-Nordic_...`
   * output: pcapng (802.15.4 TAP)
   * best for: joins + routing weirdness + “what was on the air”

2. **ZEP UDP backend**

   * input: UDP packets on `:17754` (if you run some bridge/sniffer that emits ZEP) ([Zigbee2MQTT][1])
   * output: either:

     * re-wrap into pcapng, or
     * directly parse ZEP → frames in Go

3. **PCAP/PCAPNG file backend**

   * input: `*.pcap` from whsniff or `*.pcapng` from your own capture
   * output: decode/summary/report (repeatable CI-like workflows)

### What you implement once

* a small internal model: `Frame{ ts, channel, rssi, lqi, bytes []byte }`
* “sinks”:

  * write pcapng
  * print summaries
  * (optional) call `tshark -T json` and post-process

This gets you a killer CLI quickly, without immediately reimplementing Zigbee NWK/APS/ZCL.

---

## One concrete correction to your pasted text

> “Capture what your coordinator sees (easy…)”

That’s usually *not* a raw packet feed unless you have special firmware or a bridge that emits frames (like ZEP). Most of the “easy” paths people use are either:

* **OTA sniffer** (nRF/CC2531/etc), or
* **a sniffer tool that sends ZEP** (so you can tcpdump loopback). ([Zigbee2MQTT][1])

---

If you want, I can sketch the minimal Go package layout for:

* `zbcap capture ota --port /dev/serial/by-id/usb-Nordic... --channel 15 --out join.pcapng`
* `zbcap capture zep --listen :17754 --out zep.pcapng`

…and keep it tight (no feature creep).

[1]: https://www.zigbee2mqtt.io/advanced/zigbee/04_sniff_zigbee_traffic.html?utm_source=chatgpt.com "Sniff Zigbee traffic | Zigbee2MQTT"
[2]: https://wiki.wireshark.org/IEEE_802.15.4?utm_source=chatgpt.com "IEEE_802.15.4 - Wireshark Wiki"
[3]: https://github.com/stig-bjorlykke/nRF-802.15.4-sniffer?utm_source=chatgpt.com "GitHub - stig-bjorlykke/nRF-802.15.4-sniffer: nRF-based 802.15.4 sniffer (firmware and software)"
[4]: https://github.com/homewsn/whsniff?utm_source=chatgpt.com "GitHub - homewsn/whsniff: Whsniff is a command line utility that interfaces TI CC2531 USB dongle with Wireshark for capturing and displaying IEEE 802.15.4 traffic at 2.4 GHz."
[5]: https://github.com/andrebdo/wireshark-cc2531?utm_source=chatgpt.com "GitHub - andrebdo/wireshark-cc2531: Wireshark extcap interface for the TI CC2531 USB dongle"
[6]: https://riverloopsecurity.com/projects/killerbee/?utm_source=chatgpt.com "KillerBee 2.0"
[7]: https://github.com/riverloopsec/killerbee?utm_source=chatgpt.com "GitHub - riverloopsec/killerbee: IEEE 802.15.4/ZigBee Security Research Toolkit"
[8]: https://github.com/virtualabs/cc2531-killerbee-fw?utm_source=chatgpt.com "GitHub - virtualabs/cc2531-killerbee-fw: Killerbee compatible ZigBee sniffer/injector firmware for TI CC2531 USB dongles"
[9]: https://www.zigbee2mqtt.io/advanced/zigbee/03_secure_network.html?utm_source=chatgpt.com "Secure your Zigbee network | Zigbee2MQTT"

