# ToDo & Next Steps (Medical Devices Desktop & Backend)

## 1. Frontend (`md_fe_desktop`)

### 1.1 Automatischer Token-Refresh (Priorität: Hoch)
- **Problem:** Das JWT Access-Token des Go-Backends ist aktuell nur 15 Minuten gültig. Nach Ablauf würden Hintergrund-Syncs und -Uploads unbemerkt mit einem `401 Unauthorized` abbrechen.
- **Lösung:** Implementierung einer automatischen Refresh-Logik (z.B. per `QTimer` im `TokenManager` oder `AuthService`), die vor Ablauf der 15 Minuten das Token über den Backend-Endpunkt `/api/v1/auth/refresh` erneuert und sicher im OS-Keychain (`QKeychain`) hinterlegt.

### 1.2 Hintergrund-Dateiüberwachung (`QFileSystemWatcher`) (Priorität: Hoch)
- **Logik:** Die App soll die lokalen Ordner der medizinischen Geräte (die der Nutzer in der UI via "Change Path" konfiguriert hat) permanent überwachen.
- **Implementierung:** Integration von `QFileSystemWatcher`. Erkennt dieser eine neue Datei (z.B. GDT-Dateien aus einem EKG oder DICOM-Daten), wird die Datei sofort für die Verarbeitung markiert.

### 1.3 Lokale Upload-Queue in SQLite (Priorität: Hoch)
- **Datenbank:** Erweiterung der lokalen `medical_devices.db` um eine Tabelle für Upload-Jobs (z.B. `upload_queue`).
- **Verarbeitung:** Neu gefundene Dateien werden in diese Queue eingereiht. Ein Hintergrund-Worker (`QThread`) arbeitet die Queue sequenziell ab und sendet die Dateien sicher an das Backend. Der Status (Pending, Uploading, Done, Error) wird im Frontend unter "Upload Queue" angezeigt.
- **Sicherheit:** (Falls konzeptionell vorgesehen) Verschlüsselung der ausgelesenen Daten (AES-256) vor dem Speichern in der Queue oder direkt beim Transport.

### 1.4 System-Tray Integration (Priorität: Mittel)
- **Problem:** Wenn medizinisches Personal das Fenster des Frontends versehentlich schließt, würde aktuell auch die Dateiüberwachung beendet werden.
- **Lösung:** Verwendung von `QSystemTrayIcon`. Beim Klick auf "Schließen" (X) minimiert sich das Programm in den System-Tray und läuft unsichtbar weiter. Nur über "Beenden" im Tray-Menü wird die App wirklich geschlossen.

---

## 2. Backend (`md_be_wserver`)

### 2.1 Upload-Endpoint für Medizindaten (Priorität: Hoch)
- **Endpoint:** Bereitstellen eines sicheren API-Endpunkts (z. B. `/api/v1/data/upload`), der die GDT/DICOM-Dateien vom Frontend entgegennimmt.
- **Validierung:** Sicherstellen, dass die empfangenen Dateien dem richtigen Gerät (Device ID) und dem authentifizierten Nutzer (bzw. der Praxis) zugeordnet werden.

### 2.2 Plugin-System für Parser (Priorität: Mittel)
- **Konzept:** GDT, DICOM und andere medizinische Formate sollen im Backend serverseitig in ein standardisiertes JSON-Format überführt werden.
- **Implementierung:** Basis-Architektur für ein Plugin-System, sodass für verschiedene Geräte und Hersteller (`Custo Cardio 400`, `Maico MA33` etc.) dynamisch der passende Parser geladen und ausgeführt werden kann.
