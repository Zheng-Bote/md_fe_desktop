# Changelog

All notable changes to the `md_fe_desktop` (Medical Devices Desktop Frontend) project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
