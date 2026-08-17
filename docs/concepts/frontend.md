# Architektur-Konzept: md_fe_desktop

Dieses Dokument skizziert das architektonische Konzept für die zukünftige "Offline-First"-Datenverarbeitung, das Plugin-System und die Hintergrund-Ausführung des Qt6-Frontends (`md_fe_desktop`).

## 1. Verzeichnisüberwachung (Directory Monitoring)
* **Technologie:** `QFileSystemWatcher` von Qt.
* **Ablauf:** Sobald ein medizinisches Personal den Pfad für ein Gerät im Dashboard konfiguriert, wird dieser Pfad dem `QFileSystemWatcher` übergeben. 
* **Verarbeitung:** Wird eine Datei in diesem Ordner abgelegt oder verändert, feuert das Signal `directoryChanged` oder `fileChanged`. Ein dedizierter Background-Worker (z.B. ein `DeviceMonitorService`) fängt das Signal ab, stellt die Datei in eine interne Queue und übergibt sie an das entsprechende Plugin zur Verarbeitung.

## 2. Hintergrundbetrieb & Taskleiste (System Tray)
* **Technologie:** `QSystemTrayIcon` und Anpassungen am `QApplication`-Lifecycle.
* **Ablauf:** Die Option `app.setQuitOnLastWindowClosed(false)` wird aktiviert. Wenn der Nutzer das Fenster schließt, wird es nur verborgen (`hide()`), das Programm läuft aber im Hintergrund weiter.
* **Interaktion:** Über ein kleines Icon im System Tray kann der User den Status einsehen (z.B. "3 Dateien verarbeitet", "Sync läuft...") oder das Dashboard per Doppelklick wieder öffnen. Das Programm soll für den OS-User jederzeit zugänglich bleiben.
* **Autostart:** Um das Programm bei PC-Start mitzustarten, wird plattformspezifisch verfahren (z.B. Autostart-Registry-Key bei Windows oder `.desktop`-Datei unter Linux).

## 3. Plugin-System für Geräte
Nach dem FlatBuffers-Sync der `device_types` und `devices` sollen spezifische Plugins heruntergeladen werden.
* **Speicherort:** `<Programm-Ordner>/data/devices/`
* **Technologie (Evaluierung):** 
  1. **Kompilierte C++ Plugins (Shared Libraries):** Geladen über `QPluginLoader` (`.dll` oder `.so`). Höchste Performance, erfordert aber plattformspezifische Kompilierung im Backend.
  2. **Scripting Plugins (z.B. JavaScript):** Geladen über `QJSEngine`. Komplett plattformunabhängig und leicht vom Go-Backend auszuliefern (eine `.js`-Datei für alle OS).
* **Download:** Der `SyncManager` prüft, für welche Geräte die Plugins lokal fehlen oder veraltet sind. Über einen gesicherten API-Endpunkt werden diese heruntergeladen, geprüft (Signatur/Hash) und lokal abgelegt.

## 4. Datenverarbeitung & Verschlüsselung
1. Der `QFileSystemWatcher` meldet eine neue medizinische Datei.
2. Das Frontend ruft die Verarbeitungsroutine des passenden Plugins auf.
3. Das Plugin liest die herstellerspezifische Rohdatei, extrahiert die relevanten Werte und transformiert sie in ein einheitliches JSON-Format (via `nlohmann_json`).
4. Das JSON wird an den bestehenden `CryptoHelper` (AES-256) übergeben, verschlüsselt und sicher abgelegt. Sämtliche dieser Aktivitäten werden detailliert geloggt.

## 5. Lokale Speicherung & Backend-Sync (Store-and-Forward)
* **Technologie:** SQLite (Erweiterung der lokalen DB, z.B. um eine `upload_queue`-Tabelle).
* **Ablauf Offline:** Das verschlüsselte JSON wird in der lokalen DB zwischengespeichert (Status: `pending`). Das System kann so auch ohne direkte Serververbindung einwandfrei arbeiten.
* **Ablauf Online:** Sobald eine Netzwerkverbindung zum Backend (`wserver`) besteht, werden die Datensätze aus der Queue an das Backend übertragen. Wenn der Server einen Erfolg bestätigt, wird der Eintrag in der lokalen SQLite gelöscht oder als `synced` markiert.

## 6. Sicherheitsaspekte
* Heruntergeladene Plugins müssen vor der Ausführung validiert werden (Schutz vor Code-Injection/Manipulation).
* Die End-to-End-Verschlüsselung der medizinischen Nutzdaten erfordert ein durchdachtes Key-Management zwischen Frontend und Backend, das unabhängig vom lokalen Proxy-AES-Schlüssel agiert.
