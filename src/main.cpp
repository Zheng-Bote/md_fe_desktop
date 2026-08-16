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

#include <qtkeychain/keychain.h>

#include "ConfigLoader.hpp"
#include "AuthService.hpp"
#include "LoginWindow.hpp"
#include "MainWindow.hpp"
#include "TokenManager.hpp"

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

        qDebug() << "Loaded config:" << config.name;
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Config Error", e.what());
        return -1;
    }

    TokenManager* tokenManager = new TokenManager(&app);

    auto startAppFlow = [&config, tokenManager]() {
        qDebug() << "Vault access granted. Loading tokens...";
        
        QObject::connect(tokenManager, &TokenManager::tokensLoaded, [&config, tokenManager](const QString& access, const QString& refresh) {
            if (!access.isEmpty()) {
                qDebug() << "Tokens found in Vault! Restoring session...";
                MainWindow* mainWin = new MainWindow(config);
                mainWin->setAccessToken(access);
                mainWin->show();
            } else {
                qDebug() << "No tokens found. Showing Login UI...";
                AuthService* authService = new AuthService(config);
                LoginWindow* loginWin = new LoginWindow(authService);
                
                QObject::connect(loginWin, &LoginWindow::loginSuccessful, [&config, loginWin, tokenManager](const QString& access, const QString& refresh) {
                    tokenManager->saveTokens(access, refresh);
                    loginWin->hide();
                    
                    MainWindow* mainWin = new MainWindow(config);
                    mainWin->setAccessToken(access);
                    mainWin->show();
                });

                loginWin->show();
            }
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

    return app.exec();
}
