# Projektstatus: md_fe_desktop (Medical Devices Desktop Frontend)

## 1. Aktueller Stand & Features
Das Qt6-basierte C++-Frontend (`md_fe_desktop`) wurde um mehrere Funktionen erweitert und stabilisiert:

- **Statusleiste:** Zeigt die Programm-Version, den System-Benutzernamen und den Computernamen an. 
- **GitHub Update-Check:** Überprüft asynchron beim Start, ob auf GitHub (`Zheng-Bote/md_fe_desktop`) ein neueres Release vorliegt. Falls ja, wird die Versionsnummer in der Statusleiste **rot** eingefärbt. Im "About"-Dialog gibt es zudem einen anklickbaren Link zum neuesten Release.
- **Proxy-Konfiguration & AES-256 Verschlüsselung:** 
  - Die Proxy-Daten (Host, Port, Username, Passwort) lassen sich über den "Settings -> Network-Proxy"-Dialog konfigurieren.
  - Diese benutzerspezifischen Einstellungen werden lokal AES-256-verschlüsselt in `<Programm-Ordner>/data/settings/<username>.enc` gespeichert (implementiert in `CryptoHelper.cpp` via OpenSSL `EVP`).
  - Ist der Proxy aktiv, wird er via `QNetworkProxyFactory::setApplicationProxyFactory` systemweit angewendet.
- **WServer Proxy-Bypass:** API-Anfragen an den konfigurierten Backend-Server (`wserver.host`), sowie `localhost` und `127.0.0.1` umgehen den Proxy automatisch (realisiert über die `CustomProxyFactory`).
- **Fehlerbehebungen:** Ein ursprünglicher `GLib-CRITICAL` Runtime-Crash wurde durch sauberes System-Linking statt Conan-Packages für libsecret behoben.

## 2. Architektur & Abhängigkeiten
- **C++ Standard:** C++23
- **Frameworks:** Qt6 (Core, Gui, Widgets, Network)
- **Paket-Manager:** Conan (nlohmann_json, spdlog, cpp-httplib, openssl)
- **Zusätzliche Bibliotheken:** 
  - `qtkeychain` (für zukünftige sichere OS-Token-Speicherung)
  - `gh_update_checker` (via FetchContent geladen)

## 3. Wichtige Dateien
- `src/main.cpp` / `src/MainWindow.cpp`: Haupt-Einstiegspunkte; setzen die Statusleiste und Proxy-Factory auf.
- `src/ConfigLoader.cpp` / `include/ConfigLoader.hpp`: Handhabt das Laden der globalen `md_desktop_config.json` sowie der AES-256 verschlüsselten benutzerspezifischen Konfiguration.
- `src/CryptoHelper.cpp` / `include/CryptoHelper.hpp`: Kapselt OpenSSL für die AES-256 Ver- und Entschlüsselung.
- `include/ProxyFactory.hpp`: Beinhaltet die Bypass-Logik für den WServer.

## 4. Offene Punkte / Nächste Schritte
- Das GitHub-Repository (`Zheng-Bote/md_fe_desktop`) benötigt ein erstes Release, damit der Update-Check keinen 404-Fehler wirft. (Aktuelle Meldung fängt dies ab und bittet um Netzwerk/Release-Überprüfung).
- Weitere Funktionalitäten der Benutzeroberfläche (Dashboards, Medical Devices Integration) müssen noch an den Backend-API-Server (wserver) angebunden werden.
