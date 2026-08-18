# Changelog

All notable changes to the `md_fe_desktop` (Medical Devices Desktop Frontend) project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.3.0] - 2026-08-18

### Added
- **Search Functionality:** Added a search bar to the "All Devices" section for filtering devices by name and manufacturer.
- **Improved Rendering:** Display device descriptions under their respective categories in the "All Devices" section.

### Changed
- **Startup Flow:** The application no longer requires a login at startup. It boots directly into the main window.
- **Background Sync:** The bidirectional device synchronization now runs in the background without requiring user authentication.
- **UI Layout:** The sidebar now explicitly separates "My Devices" (configured locally) and "All Devices" (available for setup).
- **Logout Behavior:** Logging out now securely clears the session and updates the UI without forcing the application to quit.

### Fixed
- **Rendering Bug:** Fixed an issue where orphaned UI elements from previous sync runs could overlap newly rendered device cards.

## [0.2.0] - 2026-08-17

### Added
- **Medical Devices Database & Sync:** 
  - Local SQLite database integration (`DatabaseManager`) for offline data persistence.
  - Asynchronous background synchronization (`SyncManager`) fetching devices and device types securely from the Go backend using Google Flatbuffers.
- **Dynamic Dashboard UI:** 
  - Responsive grid layout displaying synchronized medical devices as interactive cards.
  - Setup UI workflow allowing users to assign local data directories to specific devices.
  - "Empty State" placeholder support when no devices are synchronized.
- **Session Management:** Secure logout functionality to clear authentication tokens from `QKeychain`.

## [0.1.0] - 2026-08-16

### Added
- **Initial Release:** Qt6 / C++23 Desktop Application skeleton.
- **Authentication:** `AuthService` and `LoginWindow` for user authentication.
- **OS Vault Integration:** Secure token storage via `qtkeychain` (integrates with Linux Secret Service / Windows Credential Manager).
- **Configuration Management:** Global configuration (`md_desktop_config.json`) for backend and defaults.
- **Proxy Support:** 
  - Network-Proxy settings UI in the "Settings" menu.
  - AES-256 encrypted storage of proxy credentials in `data/settings/<username>.enc` using OpenSSL.
  - Application-wide HTTP proxy application via `QNetworkProxyFactory`.
  - WServer and localhost bypass via custom proxy factory.
- **Logging:** Integrated `spdlog` for file logging (`data/logs/YYYY-MM.log`). Supports log rotation by file size and max files limits based on configuration.
- **GitHub Updates:** Asynchronous update checking against the `Zheng-Bote/md_fe_desktop` repository.
- **Status Bar:** Displays application version, OS username, and system hostname. Highlights version in red if an update is available.
- **About Dialog:** Basic application info dialog displaying version, license, copyright, and a direct link to new releases.
