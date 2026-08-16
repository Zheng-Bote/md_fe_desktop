# MitM Medical Devices Desktop Frontend

A modern, high-performance desktop client serving as the frontend for the MitM Medical Devices application. Built with **C++23** and **Qt6**, this application provides a robust user interface and secure integration with system services.

## Features

- **Secure Authentication:** Integrates with the host OS vault (Linux Secret Service / Windows Credential Manager) via `qtkeychain` to securely store and retrieve session tokens.
- **Encrypted User Settings:** User-specific settings, such as network proxy credentials, are encrypted using **AES-256** (via OpenSSL) and stored locally.
- **Smart Network Proxying:** Application-wide proxy support with built-in bypass rules for backend API connections.
- **Robust Logging:** Integrated `spdlog` for highly configurable, rotating daily and size-based file logging.
- **Auto-Update Checks:** Asynchronous background checks against GitHub releases to notify users of available updates.

## Tech Stack

- **C++ Standard:** C++23
- **UI Framework:** Qt 6 (Widgets, Gui, Core, Network)
- **Dependency Management:** Conan 2 & CMake
- **Libraries:**
  - `nlohmann_json` (JSON parsing)
  - `spdlog` & `fmt` (Logging)
  - `cpp-httplib` (HTTP requests)
  - `OpenSSL` (Cryptography & AES-256)
  - `qtkeychain` (Secure OS vault integration)

## Prerequisites

To build the project from source, ensure you have the following installed:
- CMake (>= 3.25)
- A C++23 compatible compiler (GCC 13+, Clang 16+, MSVC 2022)
- Conan 2 package manager
- Qt 6.5+ development libraries
- OpenSSL development headers

## Building the Project

1. **Install Dependencies via Conan:**
   ```bash
   conan install . --build=missing -s build_type=Release
   ```

2. **Configure with CMake:**
   ```bash
   cmake --preset conan-release
   ```
   *(Or manually configure using the generated toolchain file from Conan)*

3. **Build:**
   ```bash
   cmake --build build/Release
   ```

## Configuration

The application reads its global configuration from `data/md_desktop_config.json` upon startup. You can find an example configuration in `md_desktop_config__example.json`.

**Sensitive Data:** 
Do not store credentials or proxy passwords in the global JSON file! Once a user configures their proxy via the UI, their sensitive settings are AES-256 encrypted and stored locally under `data/settings/<username>.enc`.

## Logging

Logs are written to `data/logs/YYYY-MM.log`. The rotation rules (max file size and max number of files) as well as the log level (`trace`, `debug`, `info`, `warn`, `error`) can be configured within the global JSON configuration.

## License

This project is licensed under the **Apache License 2.0**.
See the `NOTICE` file for information on third-party libraries used within this project.
