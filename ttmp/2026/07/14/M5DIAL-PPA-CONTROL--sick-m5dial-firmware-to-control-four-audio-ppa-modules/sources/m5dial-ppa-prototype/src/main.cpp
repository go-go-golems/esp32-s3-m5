/*
 * PPA Dial - Szenen-Umschalter fuer Four Audio PPA Module
 * =======================================================
 * Hardware: M5Stack Dial v1.1 (ESP32-S3, rundes 1.28" Touch-Display, Encoder)
 *
 * Bedienung:
 *   - Drehen  : Szene auswaehlen
 *   - Druecken (Encoder-Taste oder Touch): Szene schalten
 *
 * Einrichtung:
 *   1. Beim ersten Start oeffnet das Dial einen WLAN-Hotspot "PPA-Dial"
 *      (Passwort: ppadial123). Damit verbinden und http://192.168.4.1 oeffnen.
 *   2. WLAN-Name + Passwort eintragen und den kompletten Inhalt der Datei
 *      presets.json der Mac-App einfuegen (enthaelt die Szenen).
 *   3. Speichern -> Dial verbindet sich und ist einsatzbereit.
 *      Spaeter erreichbar unter http://ppadial.local
 *
 * Protokoll: identisch zur Mac-App "PPA Group Control" (UDP Port 5001,
 * per Mitschnitt der Four Audio Software rekonstruiert).
 */

#include <M5Dial.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// ---------------------------------------------------------------- Konstanten
static const uint16_t PPA_PORT = 5001;
static const char*    AP_SSID  = "PPA-Dial";
static const char*    AP_PASS  = "ppadial123";
static const uint32_t DISCOVERY_INTERVAL_MS = 20000;

// Farben (dunkles Hoffmann-Design)
static const uint32_t COL_BG     = 0x111111;
static const uint32_t COL_FG     = 0xFFFFFF;
static const uint32_t COL_DIM    = 0x888888;
static const uint32_t COL_GREEN  = 0x14A038;
static const uint32_t COL_RED    = 0xC03020;

// ---------------------------------------------------------------- Datentypen
struct Action {            // ein Modul innerhalb einer Szene
    uint32_t uid;          // DeviceUniqueId (0 = per fester IP)
    IPAddress fixedIp;     // nur wenn uid == 0
    uint8_t  presetId;
    uint8_t  presetSub;
    String   presetName;
};

struct Scene {
    String name;
    std::vector<Action> actions;
};

static std::vector<Scene> scenes;
static int   selected     = 0;     // per Encoder gewaehlte Szene
static int   activeScene  = -1;    // zuletzt erfolgreich geschaltete Szene
static bool  wifiOk       = false;

// uid -> IP (per Ping-Broadcast gelernt)
struct Found { uint32_t uid; IPAddress ip; uint32_t lastSeen; };
static std::vector<Found> found;

static WiFiUDP    udp;
static WebServer  server(80);
static uint16_t   seqCounter = 0x0100;

// ---------------------------------------------------------------- Protokoll
static uint16_t nextSeq() { return ++seqCounter; }

static void buildHeader(uint8_t* b, uint8_t type, uint16_t status,
                        uint16_t seq, uint8_t comp, uint8_t res) {
    b[0] = type; b[1] = 0x01;
    b[2] = status & 0xFF; b[3] = status >> 8;
    b[4] = b[5] = b[6] = b[7] = 0;          // uid
    b[8] = seq & 0xFF; b[9] = seq >> 8;
    b[10] = comp; b[11] = res;
}

static void sendPing(IPAddress to, uint16_t seq) {
    uint8_t p[16] = {0};
    buildHeader(p, 0x00, 0x0006, seq, 0xFE, 0x00);
    udp.beginPacket(to, PPA_PORT); udp.write(p, 16); udp.endPacket();
}

static void sendRecall(IPAddress to, uint16_t seq, uint8_t id, uint8_t sub) {
    uint8_t p[18] = {0};
    buildHeader(p, 0x04, 0x0102, seq, 0xFF, 0x01);
    p[12] = 0x00; p[13] = id; p[14] = sub;
    udp.beginPacket(to, PPA_PORT); udp.write(p, 18); udp.endPacket();
}

// eingehende Pakete verarbeiten; liefert Status-Kind fuer wartende seq
static int pumpUdp(uint16_t waitSeq, IPAddress waitIp) {
    int result = -1;
    int len;
    while ((len = udp.parsePacket()) > 0) {
        uint8_t b[64];
        int n = udp.read(b, sizeof(b));
        if (n < 12) continue;
        uint8_t  type = b[0];
        uint16_t status = b[2] | (b[3] << 8);
        uint32_t uid = b[4] | (b[5] << 8) | (b[6] << 16) | ((uint32_t)b[7] << 24);
        uint16_t seq = b[8] | (b[9] << 8);
        uint8_t  kind = status & 0xFF;
        // Ping-Antwort -> Geraet gefunden/aktualisiert
        if (type == 0x00 && (kind == 0x01 || kind == 0x09)) {
            bool known = false;
            for (auto& f : found) {
                if (f.uid == uid) { f.ip = udp.remoteIP(); f.lastSeen = millis(); known = true; }
            }
            if (!known) found.push_back({uid, udp.remoteIP(), millis()});
        }
        if (waitSeq != 0 && seq == waitSeq && udp.remoteIP() == waitIp) {
            if (kind == 0x01) result = 1;          // OK
            else if (kind == 0x09) result = (n > 12 && b[12] == 0x03) ? 3 : 9; // busy/err
            // 0x41 (Wait) -> weiter warten
        }
    }
    return result;
}

static void broadcastDiscovery() {
    IPAddress bc = WiFi.localIP();
    bc[3] = 255;                                   // /24 directed broadcast
    sendPing(bc, nextSeq());
    sendPing(IPAddress(255, 255, 255, 255), nextSeq());
}

static IPAddress ipForAction(const Action& a) {
    if (a.uid == 0) return a.fixedIp;
    for (auto& f : found)
        if (f.uid == a.uid) return f.ip;
    return IPAddress(0, 0, 0, 0);
}

// Recall mit Ack-Warten + Busy-Retry. true = bestaetigt.
static bool recallAction(const Action& a) {
    IPAddress ip = ipForAction(a);
    if (ip == IPAddress(0, 0, 0, 0)) return false;
    for (int attempt = 0; attempt < 5; attempt++) {
        uint16_t seq = nextSeq();
        sendRecall(ip, seq, a.presetId, a.presetSub);
        uint32_t deadline = millis() + 2500;
        while ((int32_t)(deadline - millis()) > 0) {
            int r = pumpUdp(seq, ip);
            if (r == 1) return true;
            if (r == 3) { delay(500); break; }     // busy -> neuer Versuch
            if (r == 9) return false;
            delay(5);
        }
    }
    return false;
}

// ---------------------------------------------------------------- Konfig
static String cfgSsid, cfgPass, cfgJson;

static void loadConfig() {
    if (!LittleFS.begin(true)) return;
    File f = LittleFS.open("/config.json", "r");
    if (!f) return;
    JsonDocument doc;
    if (deserializeJson(doc, f) == DeserializationError::Ok) {
        cfgSsid = doc["ssid"].as<String>();
        cfgPass = doc["pass"].as<String>();
        cfgJson = doc["presets"].as<String>();
    }
    f.close();
}

static void saveConfig() {
    JsonDocument doc;
    doc["ssid"] = cfgSsid;
    doc["pass"] = cfgPass;
    doc["presets"] = cfgJson;
    File f = LittleFS.open("/config.json", "w");
    serializeJson(doc, f);
    f.close();
}

// presets.json der Mac-App -> Szenenliste
static void parseScenes() {
    scenes.clear();
    if (cfgJson.isEmpty()) return;
    JsonDocument doc;
    if (deserializeJson(doc, cfgJson) != DeserializationError::Ok) return;
    JsonObject sc = doc["scenes"];
    for (JsonPair kv : sc) {
        Scene s;
        s.name = kv.key().c_str();
        for (JsonPair m : kv.value().as<JsonObject>()) {
            Action a{};
            String key = m.key().c_str();
            if (key.startsWith("uid_")) {
                a.uid = strtoul(key.substring(4).c_str(), nullptr, 16);
            } else if (key.startsWith("ip_")) {
                a.uid = 0;
                a.fixedIp.fromString(key.substring(3));
            }
            a.presetId  = m.value()["id"] | 0;
            a.presetSub = m.value()["sub"] | 0;
            a.presetName = m.value()["name"].as<String>();
            s.actions.push_back(a);
        }
        if (!s.actions.empty()) scenes.push_back(s);
    }
}

// ---------------------------------------------------------------- Web-Konfig
static const char* PAGE =
    "<!doctype html><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>PPA Dial Setup</title>"
    "<style>body{font-family:-apple-system,sans-serif;background:#111;color:#eee;"
    "max-width:560px;margin:24px auto;padding:0 16px}input,textarea{width:100%;"
    "padding:10px;margin:6px 0 14px;border-radius:8px;border:1px solid #444;"
    "background:#1c1c1e;color:#eee;font-size:15px}textarea{height:220px;"
    "font-family:monospace}button{background:#14a038;color:#fff;border:0;"
    "padding:12px 28px;border-radius:8px;font-size:16px}</style>"
    "<h2>PPA Dial – Einrichtung</h2><form method='POST' action='/save'>"
    "<label>WLAN-Name (SSID)</label><input name='ssid' value='%SSID%'>"
    "<label>WLAN-Passwort</label><input name='pass' type='password' value='%PASS%'>"
    "<label>Szenen: kompletten Inhalt der presets.json hier einf&uuml;gen<br>"
    "<small>(Mac: ~/Library/Application Support/PPA Group Control/presets.json)</small>"
    "</label><textarea name='presets'>%JSON%</textarea>"
    "<button>Speichern &amp; Neustarten</button></form>";

static void handleRoot() {
    String page = PAGE;
    page.replace("%SSID%", cfgSsid);
    page.replace("%PASS%", cfgPass);
    page.replace("%JSON%", cfgJson);
    server.send(200, "text/html", page);
}

static void handleSave() {
    cfgSsid = server.arg("ssid");
    cfgPass = server.arg("pass");
    cfgJson = server.arg("presets");
    saveConfig();
    server.send(200, "text/html",
                "<meta charset='utf-8'><body style='background:#111;color:#eee;"
                "font-family:sans-serif'><h2>Gespeichert - Dial startet neu &hellip;</h2>");
    delay(800);
    ESP.restart();
}

// ---------------------------------------------------------------- Anzeige
static void drawCentered(const String& txt, int y, int font, uint32_t col) {
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextColor(col, COL_BG);
    M5Dial.Display.setTextFont(font);
    M5Dial.Display.drawString(txt, M5Dial.Display.width() / 2, y);
}

static void drawMain() {
    auto& d = M5Dial.Display;
    d.fillScreen(COL_BG);
    int cx = d.width() / 2, cy = d.height() / 2;

    if (!wifiOk) {
        drawCentered("WLAN", cy - 20, 4, COL_RED);
        drawCentered("nicht verbunden", cy + 10, 2, COL_DIM);
        drawCentered("Setup: Hotspot PPA-Dial", cy + 40, 2, COL_DIM);
        return;
    }
    if (scenes.empty()) {
        drawCentered("Keine Szenen", cy - 10, 4, COL_FG);
        drawCentered("http://ppadial.local", cy + 24, 2, COL_DIM);
        return;
    }
    const Scene& s = scenes[selected];
    bool isActive = (selected == activeScene);

    // Szenen-Name gross in der Mitte
    d.setTextFont(4);
    d.setTextColor(isActive ? COL_GREEN : COL_FG, COL_BG);
    d.setTextDatum(middle_center);
    if (d.textWidth(s.name) > d.width() - 30) d.setTextFont(2);
    d.drawString(s.name, cx, cy - 8);

    if (isActive) drawCentered("AKTIV", cy + 24, 2, COL_GREEN);
    else          drawCentered("druecken = schalten", cy + 24, 2, COL_DIM);

    // Online-Status der Szenen-Module
    int online = 0;
    for (auto& a : s.actions)
        if (ipForAction(a) != IPAddress(0, 0, 0, 0)) online++;
    String st = String(online) + "/" + String(s.actions.size()) + " online";
    drawCentered(st, cy + 60, 2, online == (int)s.actions.size() ? COL_GREEN : COL_RED);

    // Punkte-Indikator (welche Szene von wie vielen)
    int n = scenes.size();
    int y = 24;
    int totalW = n * 14;
    for (int i = 0; i < n; i++) {
        int x = cx - totalW / 2 + i * 14 + 7;
        if (i == selected) d.fillCircle(x, y, 4, i == activeScene ? COL_GREEN : COL_FG);
        else               d.fillCircle(x, y, 2, i == activeScene ? COL_GREEN : COL_DIM);
    }
}

static void drawResult(bool ok, int done, int total) {
    auto& d = M5Dial.Display;
    d.fillScreen(COL_BG);
    int cy = d.height() / 2;
    drawCentered(ok ? "OK" : "Fehler", cy - 12, 4, ok ? COL_GREEN : COL_RED);
    drawCentered(String(done) + "/" + String(total) + " Module", cy + 22, 2, COL_DIM);
}

// ---------------------------------------------------------------- Schalten
static void activateSelected() {
    if (scenes.empty()) return;
    const Scene& s = scenes[selected];
    auto& d = M5Dial.Display;
    d.fillScreen(COL_BG);
    drawCentered("Schalte …", d.height() / 2, 4, COL_FG);
    M5Dial.Speaker.tone(3000, 40);

    int okCount = 0;
    for (auto& a : s.actions)
        if (recallAction(a)) okCount++;

    bool allOk = (okCount == (int)s.actions.size());
    if (allOk) activeScene = selected;
    drawResult(allOk, okCount, s.actions.size());
    M5Dial.Speaker.tone(allOk ? 4000 : 800, allOk ? 60 : 250);
    delay(900);
    drawMain();
}

// ---------------------------------------------------------------- Setup/Loop
static uint32_t lastDiscovery = 0;
static long     lastEncoder = 0;
static uint32_t lastStepMs = 0;

void setup() {
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);
    M5Dial.Display.setBrightness(120);
    M5Dial.Display.fillScreen(COL_BG);
    drawCentered("PPA Dial", M5Dial.Display.height() / 2 - 10, 4, COL_FG);
    drawCentered("starte …", M5Dial.Display.height() / 2 + 24, 2, COL_DIM);

    loadConfig();
    parseScenes();

    if (cfgSsid.length()) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(cfgSsid.c_str(), cfgPass.c_str());
        uint32_t t0 = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(200);
        wifiOk = (WiFi.status() == WL_CONNECTED);
    }
    if (!wifiOk) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASS);
    } else {
        MDNS.begin("ppadial");
    }
    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.begin();

    udp.begin(PPA_PORT);
    if (wifiOk) broadcastDiscovery();
    lastEncoder = M5Dial.Encoder.read();
    drawMain();
}

void loop() {
    M5Dial.update();
    server.handleClient();
    pumpUdp(0, IPAddress());

    // Encoder: Szene waehlen (mit Zeit-Entprellung)
    long pos = M5Dial.Encoder.read();
    long delta = pos - lastEncoder;
    if (abs(delta) >= 2 && millis() - lastStepMs > 80 && !scenes.empty()) {
        selected = (selected + (delta > 0 ? 1 : -1) + scenes.size()) % scenes.size();
        lastEncoder = pos;
        lastStepMs = millis();
        M5Dial.Speaker.tone(2400, 20);
        drawMain();
    } else if (abs(delta) >= 2) {
        lastEncoder = pos;
    }

    // Druecken (Encoder-Taste oder Touch) -> Szene schalten
    bool pressed = M5Dial.BtnA.wasPressed();
    auto t = M5Dial.Touch.getDetail();
    if (t.wasPressed()) pressed = true;
    if (pressed && wifiOk) activateSelected();

    // Discovery zyklisch auffrischen
    if (wifiOk && millis() - lastDiscovery > DISCOVERY_INTERVAL_MS) {
        lastDiscovery = millis();
        broadcastDiscovery();
        drawMain();   // Online-Zaehler aktualisieren
    }
    delay(5);
}
