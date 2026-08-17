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
#include <spdlog/spdlog.h>
#include <QSplitter>
#include <QScrollArea>
#include <QGridLayout>
#include <QFrame>
#include <QFileDialog>
#include <QApplication>

MainWindow::MainWindow(DesktopConfig& config, std::shared_ptr<DatabaseManager> dbMgr, TokenManager* tm, QWidget* parent) 
    : QMainWindow(parent), m_config(config), m_dbManager(dbMgr), m_tokenManager(tm) {
    
    m_syncManager = std::make_unique<SyncManager>(m_dbManager, m_config, this);
    connect(m_syncManager.get(), &SyncManager::syncFinished, this, &MainWindow::onSyncFinished);
    
    menuBar()->setNativeMenuBar(false);
    
    auto settingsMenu = menuBar()->addMenu("&Settings");
    auto proxyAction = new QAction("Network-Proxy", this);
    settingsMenu->addAction(proxyAction);
    connect(proxyAction, &QAction::triggered, this, &MainWindow::showProxyDialog);
    
    settingsMenu->addSeparator();
    auto logoutAction = new QAction("Logout", this);
    settingsMenu->addAction(logoutAction);
    connect(logoutAction, &QAction::triggered, this, &MainWindow::logout);

    QMenu* infoMenu = menuBar()->addMenu("&Info");
    QAction* aboutAction = new QAction("&About", this);
    infoMenu->addAction(aboutAction);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setOpaqueResize(true);
    splitter->setChildrenCollapsible(false); // Prevents disappearing panes
    
    // Sidebar
    m_sidebar = new QListWidget();
    m_sidebar->setMinimumWidth(150);
    m_sidebar->setMaximumWidth(250);
    m_sidebar->addItem("Dashboard");
    m_sidebar->addItem("My Devices");
    m_sidebar->addItem("Upload Queue");
    m_sidebar->addItem("Settings");
    m_sidebar->setCurrentRow(1); // Default to My Devices
    
    // Main Content
    m_stackedWidget = new QStackedWidget();
    m_stackedWidget->setMinimumWidth(400);
    
    // Dashboard View
    QWidget* dashboardView = new QWidget();
    QVBoxLayout* dLayout = new QVBoxLayout(dashboardView);
    dLayout->addWidget(new QLabel("<h2>MedTracker Dashboard</h2>"));
    m_statusLabel = new QLabel("Token: None");
    m_statusLabel->setWordWrap(true);
    dLayout->addWidget(m_statusLabel);
    dLayout->addStretch();
    m_stackedWidget->addWidget(dashboardView);
    
    // Devices View
    QWidget* devicesView = new QWidget();
    QVBoxLayout* devLayout = new QVBoxLayout(devicesView);
    devLayout->addWidget(new QLabel("<h2>My Devices</h2>"));
    
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    m_devicesContainer = new QWidget();
    // Layout will be created in populateDevices()
    scrollArea->setWidget(m_devicesContainer);
    devLayout->addWidget(scrollArea);
    m_stackedWidget->addWidget(devicesView);
    
    // Dummy views for others
    m_stackedWidget->addWidget(new QLabel("<h2>Upload Queue</h2><p>No active uploads.</p>"));
    m_stackedWidget->addWidget(new QLabel("<h2>Settings</h2>"));
    
    connect(m_sidebar, &QListWidget::currentRowChanged, m_stackedWidget, &QStackedWidget::setCurrentIndex);
    m_stackedWidget->setCurrentIndex(1);
    
    splitter->addWidget(m_sidebar);
    splitter->addWidget(m_stackedWidget);
    
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({200, 700}); // Force initial sizes
    
    setCentralWidget(splitter);
    resize(900, 600);
    
    // Initial population of devices
    populateDevices();

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
    
    // Start DB sync in background
    if (m_syncManager) {
        spdlog::info("Starting background sync for devices...");
        m_syncManager->performSync(token.toStdString());
    }
}

void MainWindow::onSyncFinished(bool success) {
    if (success) {
        spdlog::info("Database sync finished successfully.");
        populateDevices();
    } else {
        spdlog::warn("Database sync failed.");
    }
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

void MainWindow::populateDevices() {
    // Clear existing layout
    if (m_devicesContainer->layout()) {
        QLayoutItem* item;
        while ((item = m_devicesContainer->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete m_devicesContainer->layout();
    }
    
    QGridLayout* grid = new QGridLayout(m_devicesContainer);
    grid->setSpacing(15);
    
    std::vector<Device> devices;
    if (m_dbManager) {
        devices = m_dbManager->getDevices();
    }
    
    int row = 0;
    int col = 0;
    
    for (const auto& dev : devices) {
        QFrame* card = new QFrame();
        card->setFrameShape(QFrame::StyledPanel);
        card->setStyleSheet("QFrame { background-color: white; border-radius: 8px; border: 1px solid #ddd; }");
        card->setMinimumSize(350, 150);
        
        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        
        QLabel* nameLabel = new QLabel(QString("<b>%1</b>").arg(QString::fromStdString(dev.device_name)));
        nameLabel->setStyleSheet("font-size: 16px; border: none;");
        cardLayout->addWidget(nameLabel);
        
        QLabel* mfgLabel = new QLabel(QString::fromStdString(dev.manufacturer));
        mfgLabel->setStyleSheet("color: #555; border: none;");
        cardLayout->addWidget(mfgLabel);
        
        QString pathText = dev.local_config_path.empty() ? "Not configured" : QString::fromStdString(dev.local_config_path);
        QLabel* pathLabel = new QLabel("Path: " + pathText);
        pathLabel->setStyleSheet("color: #777; font-size: 11px; border: none;");
        pathLabel->setWordWrap(true);
        cardLayout->addWidget(pathLabel);
        
        QLabel* statusBadge = new QLabel(dev.local_config_path.empty() ? "Missing Config" : "Monitoring Path");
        statusBadge->setStyleSheet(dev.local_config_path.empty() 
            ? "background-color: #ffcc00; color: #333; padding: 4px; border-radius: 4px; font-weight: bold; border: none;"
            : "background-color: #4CAF50; color: white; padding: 4px; border-radius: 4px; font-weight: bold; border: none;");
        statusBadge->setAlignment(Qt::AlignCenter);
        
        QHBoxLayout* bottomLayout = new QHBoxLayout();
        bottomLayout->addWidget(statusBadge);
        bottomLayout->addStretch();
        
        QPushButton* changeBtn = new QPushButton("Change Path");
        changeBtn->setStyleSheet("background-color: #005A9C; color: white; border-radius: 4px; padding: 5px 10px; border: none;");
        
        // Use a smart pointer or std::string by value for the lambda
        std::string dId = dev.id;
        connect(changeBtn, &QPushButton::clicked, this, [this, dId]() {
            this->changeDevicePath(dId);
        });
        
        bottomLayout->addWidget(changeBtn);
        cardLayout->addLayout(bottomLayout);
        
        grid->addWidget(card, row, col);
        
        col++;
        if (col > 1) {
            col = 0;
            row++;
        }
    }
    
    if (devices.empty()) {
        QLabel* emptyLabel = new QLabel("No medical devices synchronized or found.");
        emptyLabel->setStyleSheet("color: #777; font-size: 14px; font-style: italic;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        grid->addWidget(emptyLabel, 0, 0, 1, 2, Qt::AlignCenter);
        grid->setRowStretch(1, 1);
    } else {
        grid->setRowStretch(row + 1, 1);
    }
}

void MainWindow::changeDevicePath(const std::string& deviceId) {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Local Folder for Device Data",
                                                    QString(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (!dir.isEmpty() && m_dbManager) {
        if (m_dbManager->updateDevicePath(deviceId, dir.toStdString())) {
            populateDevices();
        } else {
            QMessageBox::warning(this, "Error", "Failed to save device path configuration.");
        }
    }
}

void MainWindow::logout() {
    auto reply = QMessageBox::question(this, "Logout", "Are you sure you want to logout? This will require you to enter your credentials again.",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        if (m_tokenManager) {
            m_tokenManager->deleteTokens();
            spdlog::info("User logged out, tokens deleted.");
        }
        
        QMessageBox::information(this, "Logged Out", "You have been successfully logged out. The application will now close.");
        QApplication::quit();
    }
}
