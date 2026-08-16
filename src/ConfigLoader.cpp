/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file ConfigLoader.cpp
 * @brief Parses the JSON configuration file for the desktop client
 * @version 1.0.0
 * @date 2026-08-16
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "ConfigLoader.hpp"
#include <QFile>
#include <QDir>
#include <QProcessEnvironment>
#include <QCoreApplication>
#include <nlohmann/json.hpp>
#include <iostream>
#include "CryptoHelper.hpp"

using json = nlohmann::json;

DesktopConfig ConfigLoader::load(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("Could not open config file: " + filePath.toStdString());
    }

    QByteArray data = file.readAll();
    file.close();

    json j = json::parse(data.constData());

    DesktopConfig config;
    if (j.contains("name")) config.name = QString::fromStdString(j["name"]);
    
    if (j.contains("log")) {
        auto l = j["log"];
        if (l.contains("log_level")) config.log.logLevel = QString::fromStdString(l["log_level"]);
        if (l.contains("max_file_size_mb")) config.log.maxFileSizeMb = l["max_file_size_mb"];
        if (l.contains("max_files")) config.log.maxFiles = l["max_files"];
        if (l.contains("rotate_hour")) config.log.rotateHour = l["rotate_hour"];
        if (l.contains("rotate_minute")) config.log.rotateMinute = l["rotate_minute"];
    }
    
    if (j.contains("wserver")) {
        auto w = j["wserver"];
        if (w.contains("host")) config.wserver.host = QString::fromStdString(w["host"]);
        if (w.contains("port")) config.wserver.port = w["port"];
        if (w.contains("use_https")) config.wserver.useHttps = w["use_https"];
    }
    
    if (j.contains("networking") && j["networking"].contains("proxy")) {
        auto p = j["networking"]["proxy"];
        if (p.contains("proxy_host")) config.proxy.proxyHost = QString::fromStdString(p["proxy_host"]);
        if (p.contains("proxy_port")) config.proxy.proxyPort = p["proxy_port"];
        if (p.contains("proxy_username")) config.proxy.proxyUsername = QString::fromStdString(p["proxy_username"]);
        if (p.contains("proxy_password")) config.proxy.proxyPassword = QString::fromStdString(p["proxy_password"]);
        if (p.contains("proxy_active")) config.proxy.proxyActive = p["proxy_active"];
    }

    return config;
}

void ConfigLoader::save(const DesktopConfig& config, const QString& filePath) {
    QFile file(filePath);
    json j;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray data = file.readAll();
        file.close();
        try { j = json::parse(data.constData()); } catch (...) {}
    }

    j["name"] = config.name.toStdString();
    
    j["log"]["log_level"] = config.log.logLevel.toStdString();
    j["log"]["max_file_size_mb"] = config.log.maxFileSizeMb;
    j["log"]["max_files"] = config.log.maxFiles;
    j["log"]["rotate_hour"] = config.log.rotateHour;
    j["log"]["rotate_minute"] = config.log.rotateMinute;
    
    j["wserver"]["host"] = config.wserver.host.toStdString();
    j["wserver"]["port"] = config.wserver.port;
    j["wserver"]["use_https"] = config.wserver.useHttps;

    j["networking"]["proxy"]["proxy_host"] = config.proxy.proxyHost.toStdString();
    j["networking"]["proxy"]["proxy_port"] = config.proxy.proxyPort;

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::string out = j.dump(4);
        file.write(out.c_str(), out.size());
        file.close();
    }
}

void ConfigLoader::loadUserConfig(DesktopConfig& config) {
    auto env = QProcessEnvironment::systemEnvironment();
    QString osUser = env.value("USER", env.value("USERNAME", "unknown"));
    
    QString dir = QCoreApplication::applicationDirPath() + "/data/settings/";
    QString userConfigPath = dir + osUser + ".enc";
    
    QFile file(userConfigPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    QByteArray encryptedData = file.readAll();
    file.close();
    
    std::string pwd = osUser.toStdString() + "_mitm_desktop_secret_2026";
    std::vector<unsigned char> encVec(encryptedData.begin(), encryptedData.end());
    
    try {
        std::string jsonStr = md::crypto::CryptoHelper::decryptAES256(encVec, pwd);
        json j = json::parse(jsonStr);
        if (j.contains("networking") && j["networking"].contains("proxy")) {
            auto p = j["networking"]["proxy"];
            if (p.contains("proxy_host")) config.proxy.proxyHost = QString::fromStdString(p["proxy_host"]);
            if (p.contains("proxy_port")) config.proxy.proxyPort = p["proxy_port"];
            if (p.contains("proxy_username")) config.proxy.proxyUsername = QString::fromStdString(p["proxy_username"]);
            if (p.contains("proxy_password")) config.proxy.proxyPassword = QString::fromStdString(p["proxy_password"]);
            if (p.contains("proxy_active")) config.proxy.proxyActive = p["proxy_active"];
        }
    } catch (...) {
        // decryption or parse failed, ignore
    }
}

void ConfigLoader::saveUserConfig(const DesktopConfig& config) {
    auto env = QProcessEnvironment::systemEnvironment();
    QString osUser = env.value("USER", env.value("USERNAME", "unknown"));
    
    QString dir = QCoreApplication::applicationDirPath() + "/data/settings/";
    QDir().mkpath(dir);
    QString userConfigPath = dir + osUser + ".enc";
    
    json j;
    j["networking"]["proxy"]["proxy_host"] = config.proxy.proxyHost.toStdString();
    j["networking"]["proxy"]["proxy_port"] = config.proxy.proxyPort;
    j["networking"]["proxy"]["proxy_username"] = config.proxy.proxyUsername.toStdString();
    j["networking"]["proxy"]["proxy_password"] = config.proxy.proxyPassword.toStdString();
    j["networking"]["proxy"]["proxy_active"] = config.proxy.proxyActive;
    
    std::string pwd = osUser.toStdString() + "_mitm_desktop_secret_2026";
    std::string jsonStr = j.dump(4);
    
    try {
        auto encryptedData = md::crypto::CryptoHelper::encryptAES256(jsonStr, pwd);
        QFile file(userConfigPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reinterpret_cast<const char*>(encryptedData.data()), encryptedData.size());
            file.close();
        }
    } catch (...) {
        // encryption failed
    }
}
