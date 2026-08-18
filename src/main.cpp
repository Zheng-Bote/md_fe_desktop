/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file main.cpp
 * @brief Main entry point
 * @version 1.0.0
 * @date 2026-08-16
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include <QApplication>
#include <QDebug>
#include <QNetworkProxy>
#include "ProxyFactory.hpp"
#include <QProcessEnvironment>
#include <QMessageBox>
#include <QIcon>
#include <QDate>
#include <QDir>
#include <QSysInfo>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "rz_config.hpp"

#include <qtkeychain/keychain.h>

#include "ConfigLoader.hpp"
#include "AuthService.hpp"
#include "LoginWindow.hpp"
#include "MainWindow.hpp"
#include "TokenManager.hpp"
#include "DatabaseManager.hpp"
#include <memory>

QString getOSUser() {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
#ifdef Q_OS_WIN
    return env.value("USERNAME");
#else
    return env.value("USER");
#endif
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("MitM Medical Devices Desktop");
    app.setWindowIcon(QIcon("img/logo_256x256.png"));

    QString osUser = getOSUser();
    qDebug() << "Started application as OS User:" << osUser;

    DesktopConfig config;
    try {
        config = ConfigLoader::load("data/md_desktop_config.json");
        ConfigLoader::loadUserConfig(config);

        if (config.proxy.proxyActive) {
            QNetworkProxy proxy;
            proxy.setType(QNetworkProxy::HttpProxy);
            proxy.setHostName(config.proxy.proxyHost);
            proxy.setPort(config.proxy.proxyPort);
            proxy.setUser(config.proxy.proxyUsername);
            proxy.setPassword(config.proxy.proxyPassword);
            QNetworkProxyFactory::setApplicationProxyFactory(new CustomProxyFactory(proxy, config.wserver.host));
        } else {
            QNetworkProxyFactory::setApplicationProxyFactory(new CustomProxyFactory(QNetworkProxy::NoProxy, config.wserver.host));
        }

        QString logDir = QCoreApplication::applicationDirPath() + "/data/logs/";
        QDir().mkpath(logDir);
        QString logFile = logDir + QDate::currentDate().toString("yyyy-MM") + ".log";
        
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logFile.toStdString(),
            config.log.maxFileSizeMb * 1024 * 1024,
            config.log.maxFiles
        );

        spdlog::set_default_logger(std::make_shared<spdlog::logger>("main", file_sink));
        spdlog::flush_on(spdlog::level::info);

        QString levelStr = config.log.logLevel.toLower();
        if (levelStr == "debug") spdlog::set_level(spdlog::level::debug);
        else if (levelStr == "trace") spdlog::set_level(spdlog::level::trace);
        else if (levelStr == "warning" || levelStr == "warn") spdlog::set_level(spdlog::level::warn);
        else if (levelStr == "error" || levelStr == "err") spdlog::set_level(spdlog::level::err);
        else spdlog::set_level(spdlog::level::info);

        QString compName = QSysInfo::machineHostName();
        spdlog::info("=== Application Started ===");
        spdlog::info("Program: {}", std::string(rz::config::PROG_LONGNAME));
        spdlog::info("Version: {}", std::string(rz::config::VERSION));
        spdlog::info("User: {}", osUser.toStdString());
        spdlog::info("Computer: {}", compName.toStdString());

        qDebug() << "Loaded config:" << config.name;
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Config Error", e.what());
        return -1;
    }
    TokenManager* tokenManager = new TokenManager(&app);

    std::shared_ptr<DatabaseManager> dbManager = std::make_shared<DatabaseManager>();
    if (!dbManager->initialize()) {
        spdlog::error("Failed to initialize local SQLite database");
    } else {
        spdlog::info("Local SQLite database initialized.");
    }

    auto startAppFlow = [&config, tokenManager, dbManager]() {
        qDebug() << "Vault access granted. Loading tokens...";
        
        QObject::connect(tokenManager, &TokenManager::tokensLoaded, [&config, tokenManager, dbManager](const QString& access, const QString& refresh) {
            MainWindow* mainWin = new MainWindow(config, dbManager, tokenManager);
            if (!access.isEmpty()) {
                qDebug() << "Tokens found in Vault! Restoring session...";
                mainWin->setAccessToken(access);
            } else {
                qDebug() << "No tokens found. Starting without login...";
                mainWin->startBackgroundSync();
            }
            mainWin->show();
        });

        tokenManager->loadTokens();
    };

#ifdef Q_OS_WIN
    // TODO: Implement actual WinRT Windows::Security::Credentials::UI::UserConsentVerifier here
    qDebug() << "Triggering Windows Hello Biometrics via WinRT...";
    // Mock success
    startAppFlow();
#else
    // Linux: Use qtkeychain to ensure OS vault is unlocked.
    qDebug() << "Authenticating via Linux Secret Service (PAM/Polkit)...";
    auto *job = new QKeychain::ReadPasswordJob(QLatin1String("md_fe_desktop"));
    job->setAutoDelete(true);
    job->setKey("master_auth_check");

    QObject::connect(job, &QKeychain::Job::finished, [startAppFlow, &app](QKeychain::Job *job) {
        if (job->error() && job->error() != QKeychain::Error::EntryNotFound) {
            qWarning() << "Keychain error:" << job->errorString();
            QMessageBox::critical(nullptr, "OS Auth Failed", "Could not unlock OS vault.");
            app.quit();
        } else {
            startAppFlow();
        }
    });

    job->start();
#endif

    int ret = app.exec();
    spdlog::info("=== Application Ended ===");
    spdlog::shutdown();
    return ret;
}
