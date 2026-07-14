# PPA Dial – Firmware aufspielen (Schritt für Schritt)

Du brauchst: den M5Stack Dial v1.1, das mitgelieferte USB-C-Kabel und ca. 20 Minuten.
Programmieren musst du nichts – nur klicken.

## 1. Software installieren (einmalig)

1. **Visual Studio Code** herunterladen und installieren: https://code.visualstudio.com
2. VS Code öffnen → links auf das **Bausteine-Symbol** (Extensions) klicken
   → nach **"PlatformIO IDE"** suchen → **Install** klicken.
3. Warten, bis unten rechts "PlatformIO installed" erscheint (dauert ein paar Minuten),
   dann VS Code einmal neu starten.

## 2. Firmware aufspielen

1. Diesen Ordner (`m5dial-ppa`) in VS Code öffnen: **File → Open Folder…**
2. Den M5 Dial per USB-C an den Mac anschließen.
3. Unten in der blauen Leiste auf den **Pfeil nach rechts (→)** klicken ("PlatformIO: Upload").
4. Beim ersten Mal lädt PlatformIO automatisch alles Nötige herunter (5–10 Min.).
   Am Ende steht **SUCCESS** – fertig, das Dial startet neu.

Falls "no serial port found" erscheint: Kabel prüfen (es muss ein Datenkabel sein),
oder beim Einstecken die Taste auf der Dial-Unterseite gedrückt halten.

## 3. Dial einrichten (einmalig)

1. Das Dial zeigt "WLAN nicht verbunden". Es öffnet einen eigenen Hotspot:
   Am Mac/iPhone mit dem WLAN **PPA-Dial** verbinden (Passwort: `ppadial123`).
2. Im Browser **http://192.168.4.1** öffnen.
3. Dein WLAN (Name + Passwort) eintragen.
4. Am Mac die Datei öffnen:
   `~/Library/Application Support/PPA Group Control/presets.json`
   (Finder → Gehe zu → Gehe zum Ordner … → Pfad einfügen)
   Kompletten Inhalt kopieren und in das große Feld einfügen.
5. **Speichern & Neustarten** klicken. Das Dial verbindet sich mit deinem WLAN
   und zeigt deine Szenen an.

Szenen später ändern: neue Szenen zuerst in der Mac-App anlegen, dann im Browser
**http://ppadial.local** öffnen und die presets.json erneut einfügen.

## 4. Bedienung

- **Drehen** = Szene auswählen (Punkte oben zeigen die Position)
- **Drücken** (Taste oder Touch) = Szene schalten
- Grün + "AKTIV" = diese Szene ist gerade geschaltet
- Unten steht, wie viele Module der Szene online sind (z. B. "2/2 online")

## Bei Problemen

Fehlermeldung beim Upload oder auf dem Display? Einfach den Text kopieren
und Claude schicken – dann fixen wir das gemeinsam.
