/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file MainWindow.cpp
 * @brief Main application window
 * @version 1.0.0
 * @date 2026-08-16
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include <QStatusBar>
#include <QProcessEnvironment>
#include <QSysInfo>
#include <QNetworkProxy>
#include "ProxyFactory.hpp"
#include "MainWindow.hpp"
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QAction>
#include <QDialog>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <thread>
#include "rz_config.hpp"
#include <check_gh-update.hpp>

MainWindow::MainWindow(DesktopConfig& config, QWidget* parent) : QMainWindow(parent), m_config(config) {
    menuBar()->setNativeMenuBar(false);
    
    auto settingsMenu = menuBar()->addMenu("&Settings");
    auto proxyAction = new QAction("Network-Proxy", this);
    settingsMenu->addAction(proxyAction);
    connect(proxyAction, &QAction::triggered, this, &MainWindow::showProxyDialog);

    QMenu* infoMenu = menuBar()->addMenu("&Info");
    QAction* aboutAction = new QAction("&About", this);
    infoMenu->addAction(aboutAction);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

    QWidget* central = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);
    
    QLabel* welcome = new QLabel("<h2>Dashboard</h2>");
    layout->addWidget(welcome);
    
    m_statusLabel = new QLabel("Token: None");
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);
    
    setCentralWidget(central);
    resize(800, 600);

    // Setup Status Bar Info
    QString versionStr = QString::fromUtf8(rz::config::VERSION.data(), rz::config::VERSION.size());
    auto env = QProcessEnvironment::systemEnvironment();
    QString osUser = env.value("USER", env.value("USERNAME", "unknown"));
    QString compName = QSysInfo::machineHostName();
    
    m_appInfoLabel = new QLabel(this);
    m_appInfoLabel->setText(QString("Version: %1 | User: %2 | Computer: %3").arg(versionStr, osUser, compName));
    statusBar()->addWidget(m_appInfoLabel);

    // Check for updates asynchronously
    std::string proxyStr;
    if (m_config.proxy.proxyActive) {
        std::string host = m_config.proxy.proxyHost.toStdString();
        std::string user = m_config.proxy.proxyUsername.toStdString();
        std::string pass = m_config.proxy.proxyPassword.toStdString();
        if (!user.empty() || !pass.empty()) {
            proxyStr = user + ":" + pass + "@" + host + ":" + std::to_string(m_config.proxy.proxyPort);
        } else {
            // Include dummy credentials for regex parsing if necessary, or just host:port
            proxyStr = "user:pass@" + host + ":" + std::to_string(m_config.proxy.proxyPort);
        }
    }
    
    std::thread([this, versionStr, osUser, compName, proxyStr]() {
        try {
            auto future = ghupdate::check_github_update_async(
                std::string(rz::config::PROJECT_HOMEPAGE_URL),
                std::string(rz::config::VERSION), proxyStr);
            auto result = future.get();

            if (result.hasUpdate) {
                QMetaObject::invokeMethod(
                    m_appInfoLabel,
                    [this, osUser, compName]() {
                        QString versionStr = QString::fromUtf8(rz::config::VERSION.data(), rz::config::VERSION.size());
                        m_appInfoLabel->setText(QString("Version: <font color='red'>%1</font> | User: %2 | Computer: %3").arg(versionStr, osUser, compName));
                    },
                    Qt::QueuedConnection);
            }
        } catch (const std::exception &) {
            // Ignore errors here
        }
    }).detach();
}

void MainWindow::setAccessToken(const QString& token) {
    m_statusLabel->setText("Token: " + token);
}

void MainWindow::showAboutDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("About " + QString::fromStdString(std::string(rz::config::PROJECT_NAME)));
    dialog.resize(400, 250);

    auto *layout = new QVBoxLayout(&dialog);

    QString infoText =
        QString("<b>%1</b><br/>"
                "%2<br/><br/>"
                "<b>Version:</b> %3<br/>"
                "<b>License:</b> %4<br/>"
                "<b>Copyright:</b> %5 %6<br/><br/>")
            .arg(QString::fromUtf8(rz::config::PROJECT_NAME.data(), rz::config::PROJECT_NAME.size()),
                 QString::fromUtf8(rz::config::PROJECT_DESCRIPTION.data(), rz::config::PROJECT_DESCRIPTION.size()),
                 QString::fromUtf8(rz::config::VERSION.data(), rz::config::VERSION.size()),
                 QString::fromUtf8(rz::config::LICENSE.data(), rz::config::LICENSE.size()),
                 QString::fromUtf8(rz::config::CREATED_YEAR.data(), rz::config::CREATED_YEAR.size()),
                 QString::fromUtf8(rz::config::COPYRIGHT.data(), rz::config::COPYRIGHT.size()));

    auto *hLayout = new QHBoxLayout();
    layout->addLayout(hLayout);

    auto *logoLabel = new QLabel(&dialog);
    QPixmap logoPixmap("img/logo_256x256.png");
    if (!logoPixmap.isNull()) {
        logoLabel->setPixmap(logoPixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    hLayout->addWidget(logoLabel);

    auto *infoLabel = new QLabel(infoText, &dialog);
    infoLabel->setWordWrap(true);
    hLayout->addWidget(infoLabel);

    auto *updateLabel = new QLabel("<i>Checking for updates...</i>", &dialog);
    updateLabel->setWordWrap(true);
    updateLabel->setOpenExternalLinks(true);
    layout->addWidget(updateLabel);

    auto *okButton = new QPushButton("OK", &dialog);
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addStretch();
    layout->addWidget(okButton);

    std::string proxyStr;
    if (m_config.proxy.proxyActive) {
        proxyStr = m_config.proxy.proxyHost.toStdString() + ":" + std::to_string(m_config.proxy.proxyPort);
    }
    
    std::thread([updateLabel, proxyStr]() {
        try {
            auto future = ghupdate::check_github_update_async(
                std::string(rz::config::PROJECT_HOMEPAGE_URL),
                std::string(rz::config::VERSION), proxyStr);
            auto result = future.get();

            QMetaObject::invokeMethod(
                updateLabel,
                [updateLabel, result]() {
                    if (result.hasUpdate) {
                        QString repoUrl = QString::fromUtf8(rz::config::PROJECT_HOMEPAGE_URL.data(), rz::config::PROJECT_HOMEPAGE_URL.size());
                        QString releaseUrl = repoUrl + "/releases/latest";
                        updateLabel->setText(
                            QString("<font color='green'><b>🚀 Update available: %1</b></font><br/>"
                                    "<a href=\"%2\">Download Latest Release</a>")
                                .arg(QString::fromStdString(result.latestVersion))
                                .arg(releaseUrl));
                    } else {
                        updateLabel->setText("You are using the latest version.");
                    }
                },
                Qt::QueuedConnection);
        } catch (const std::exception &) {
            QMetaObject::invokeMethod(
                updateLabel,
                [updateLabel]() {
                    updateLabel->setText(
                        "<font color='red'>Update check failed.</font><br/><i>Ensure your internet connection is active and the Github repository has releases.</i>");
                },
                Qt::QueuedConnection);
        }
    }).detach();

    dialog.exec();
}

void MainWindow::showProxyDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Network Proxy Settings");
    dialog.resize(350, 200);

    auto *layout = new QFormLayout(&dialog);

    auto *hostEdit = new QLineEdit(m_config.proxy.proxyHost, &dialog);
    auto *portEdit = new QLineEdit(QString::number(m_config.proxy.proxyPort), &dialog);
    auto *userEdit = new QLineEdit(m_config.proxy.proxyUsername, &dialog);
    auto *passEdit = new QLineEdit(m_config.proxy.proxyPassword, &dialog);
    passEdit->setEchoMode(QLineEdit::Password);
    auto *activeCheck = new QCheckBox("Enable Proxy", &dialog);
    activeCheck->setChecked(m_config.proxy.proxyActive);

    layout->addRow("Proxy Host:", hostEdit);
    layout->addRow("Proxy Port:", portEdit);
    layout->addRow("Username:", userEdit);
    layout->addRow("Password:", passEdit);
    layout->addRow("", activeCheck);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        m_config.proxy.proxyHost = hostEdit->text();
        bool ok;
        int port = portEdit->text().toInt(&ok);
        m_config.proxy.proxyPort = ok ? port : 8080;
        m_config.proxy.proxyUsername = userEdit->text();
        m_config.proxy.proxyPassword = passEdit->text();
        m_config.proxy.proxyActive = activeCheck->isChecked();

        if (m_config.proxy.proxyActive) {
            QNetworkProxy proxy;
            proxy.setType(QNetworkProxy::HttpProxy);
            proxy.setHostName(m_config.proxy.proxyHost);
            proxy.setPort(m_config.proxy.proxyPort);
            proxy.setUser(m_config.proxy.proxyUsername);
            proxy.setPassword(m_config.proxy.proxyPassword);
            QNetworkProxyFactory::setApplicationProxyFactory(new CustomProxyFactory(proxy, m_config.wserver.host));
        } else {
            QNetworkProxyFactory::setApplicationProxyFactory(new CustomProxyFactory(QNetworkProxy::NoProxy, m_config.wserver.host));
        }

        ConfigLoader::saveUserConfig(m_config);
    }
}
