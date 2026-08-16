/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file ConfigLoader.h
 * @brief Parses the JSON configuration file for the desktop client
 * @version 1.0.0
 * @date 2026-08-16
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#ifndef CONFIGLOADER_HPP
#define CONFIGLOADER_HPP

#include <QString>
#include <stdexcept>

struct WServerConfig {
    QString host;
    int port;
    bool useHttps;
};

struct ProxyConfig {
    QString proxyHost;
    int proxyPort = 0;
    QString proxyUsername;
    QString proxyPassword;
    bool proxyActive = false;
};

struct DesktopConfig {
    QString name;
    QString logLevel;
    WServerConfig wserver;
    ProxyConfig proxy;
};

class ConfigLoader {
public:
    static DesktopConfig load(const QString& filePath);
    static void save(const DesktopConfig& config, const QString& filePath);
    
    static void loadUserConfig(DesktopConfig& config);
    static void saveUserConfig(const DesktopConfig& config);
};

#endif // CONFIGLOADER_HPP
