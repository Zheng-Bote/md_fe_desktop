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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "ProxyFactory.hpp"
#include "MainWindow.hpp"
#include "AuthService.hpp"
#include "LoginWindow.hpp"
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
#include <map>
#include <unordered_map>
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
    connect(m_syncManager.get(), &SyncManager::syncAuthError, this, [this]() {
        m_accessToken.clear();
        if (m_statusLabel) {
            m_statusLabel->setText("Token: Invalid / Expired (Please login again)");
        }
        QMessageBox::warning(this, "Session Expired", "Your session has expired or is invalid. Please log in again.");
    });
    
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
    m_sidebar->addItem("All Devices");
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
    
    // All Devices View
    QWidget* allDevicesView = new QWidget();
    QVBoxLayout* allDevLayout = new QVBoxLayout(allDevicesView);
    allDevLayout->addWidget(new QLabel("<h2>All Devices</h2>"));
    
    m_allDevicesSearchInput = new QLineEdit();
    m_allDevicesSearchInput->setPlaceholderText("Search by device name or manufacturer...");
    m_allDevicesSearchInput->setClearButtonEnabled(true);
    allDevLayout->addWidget(m_allDevicesSearchInput);
    connect(m_allDevicesSearchInput, &QLineEdit::textChanged, this, [this]() {
        populateAllDevices();
    });
    
    QScrollArea* allScrollArea = new QScrollArea();
    allScrollArea->setWidgetResizable(true);
    allScrollArea->setFrameShape(QFrame::NoFrame);
    
    m_allDevicesContainer = new QWidget();
    allScrollArea->setWidget(m_allDevicesContainer);
    allDevLayout->addWidget(allScrollArea);
    m_stackedWidget->addWidget(allDevicesView);
    
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
    m_accessToken = token;
    m_statusLabel->setText("Token: " + token);
    startBackgroundSync();
}

void MainWindow::startBackgroundSync() {
    if (m_syncManager) {
        spdlog::info("Starting background sync for devices...");
        m_syncManager->performSync(m_accessToken.toStdString());
    }
}

void MainWindow::requireLoginAndExecute(std::function<void()> onSuccess) {
    if (!m_accessToken.isEmpty()) {
        onSuccess();
    } else {
        AuthService* authService = new AuthService(m_config);
        LoginWindow* loginWin = new LoginWindow(authService);
        
        QObject::connect(loginWin, &LoginWindow::loginSuccessful, this, [this, loginWin, onSuccess](const QString& access, const QString& refresh) {
            if (m_tokenManager) {
                m_tokenManager->saveTokens(access, refresh);
            }
            setAccessToken(access);
            loginWin->hide();
            loginWin->deleteLater();
            onSuccess();
        });

        loginWin->show();
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

static void clearLayout(QLayout* layout) {
    if (!layout) return;
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        if (QLayout* childLayout = item->layout()) {
            clearLayout(childLayout);
        }
        delete item;
    }
}

void MainWindow::populateDevices() {
    populateMyDevices();
    populateAllDevices();
}

void MainWindow::populateMyDevices() {
    if (m_devicesContainer->layout()) {
        clearLayout(m_devicesContainer->layout());
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
    bool hasAny = false;
    
    for (const auto& dev : devices) {
        if (dev.local_config_path.empty()) continue;
        hasAny = true;
        
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
        
        QLabel* pathLabel = new QLabel("Path: " + QString::fromStdString(dev.local_config_path));
        pathLabel->setStyleSheet("color: #777; font-size: 11px; border: none;");
        pathLabel->setWordWrap(true);
        cardLayout->addWidget(pathLabel);
        
        QLabel* statusBadge = new QLabel("Monitoring Path");
        statusBadge->setStyleSheet("background-color: #4CAF50; color: white; padding: 4px; border-radius: 4px; font-weight: bold; border: none;");
        statusBadge->setAlignment(Qt::AlignCenter);
        
        QHBoxLayout* bottomLayout = new QHBoxLayout();
        bottomLayout->addWidget(statusBadge);
        bottomLayout->addStretch();
        
        QPushButton* removeBtn = new QPushButton("Remove");
        removeBtn->setStyleSheet("background-color: #D32F2F; color: white; border-radius: 4px; padding: 5px 10px; border: none;");
        
        std::string dId = dev.id;
        connect(removeBtn, &QPushButton::clicked, this, [this, dId]() {
            this->requireLoginAndExecute([this, dId]() {
                auto reply = QMessageBox::question(this, "Remove Device", "Remove device from 'My Devices'?", QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    if (this->m_dbManager && this->m_dbManager->updateDevicePath(dId, "")) {
                        this->populateDevices();
                    } else {
                        QMessageBox::warning(this, "Error", "Failed to remove device.");
                    }
                }
            });
        });

        QPushButton* changeBtn = new QPushButton("Change Path");
        changeBtn->setStyleSheet("background-color: #005A9C; color: white; border-radius: 4px; padding: 5px 10px; border: none;");
        
        connect(changeBtn, &QPushButton::clicked, this, [this, dId]() {
            this->changeDevicePath(dId);
        });
        
        bottomLayout->addWidget(removeBtn);
        bottomLayout->addWidget(changeBtn);
        cardLayout->addLayout(bottomLayout);
        
        grid->addWidget(card, row, col);
        
        col++;
        if (col > 1) {
            col = 0;
            row++;
        }
    }
    
    if (!hasAny) {
        QLabel* emptyLabel = new QLabel("No devices configured on this system.");
        emptyLabel->setStyleSheet("color: #777; font-size: 14px; font-style: italic;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        grid->addWidget(emptyLabel, 0, 0, 1, 2, Qt::AlignCenter);
        grid->setRowStretch(1, 1);
    } else {
        grid->setRowStretch(row + 1, 1);
    }
}

void MainWindow::populateAllDevices() {
    if (m_allDevicesContainer->layout()) {
        clearLayout(m_allDevicesContainer->layout());
        delete m_allDevicesContainer->layout();
    }
    
    QVBoxLayout* mainLayout = new QVBoxLayout(m_allDevicesContainer);
    mainLayout->setSpacing(20);
    
    std::vector<Device> devices;
    std::vector<DeviceType> types;
    if (m_dbManager) {
        devices = m_dbManager->getDevices();
        types = m_dbManager->getDeviceTypes();
    }
    
    std::unordered_map<std::string, DeviceType> typeMap;
    for (const auto& t : types) {
        typeMap[t.id] = t;
    }
    
    QString searchText = m_allDevicesSearchInput ? m_allDevicesSearchInput->text().toLower() : "";
    
    std::map<std::string, std::vector<Device>> devicesByTypeId;
    bool hasAny = false;
    for (const auto& dev : devices) {
        if (!dev.local_config_path.empty() || !dev.active) continue;
        
        if (!searchText.isEmpty()) {
            QString name = QString::fromStdString(dev.device_name).toLower();
            QString mfg = QString::fromStdString(dev.manufacturer).toLower();
            if (!name.contains(searchText) && !mfg.contains(searchText)) {
                continue;
            }
        }
        
        devicesByTypeId[dev.type_id].push_back(dev);
        hasAny = true;
    }
    
    if (!hasAny) {
        QLabel* emptyLabel = new QLabel("No available devices found.");
        emptyLabel->setStyleSheet("color: #777; font-size: 14px; font-style: italic;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(emptyLabel);
        mainLayout->addStretch();
        return;
    }
    
    for (const auto& [typeId, devs] : devicesByTypeId) {
        std::string typeName = "Unknown Type";
        std::string typeDesc = "";
        if (typeMap.count(typeId)) {
            typeName = typeMap[typeId].name;
            typeDesc = typeMap[typeId].description;
        }
        
        QLabel* typeLabel = new QLabel(QString("<h3>%1</h3>").arg(QString::fromStdString(typeName)));
        mainLayout->addWidget(typeLabel);
        
        if (!typeDesc.empty()) {
            QLabel* descLabel = new QLabel(QString::fromStdString(typeDesc));
            descLabel->setStyleSheet("color: #666; font-size: 12px; margin-bottom: 5px;");
            descLabel->setWordWrap(true);
            mainLayout->addWidget(descLabel);
        }
        
        QGridLayout* grid = new QGridLayout();
        grid->setSpacing(15);
        int row = 0;
        int col = 0;
        
        for (const auto& dev : devs) {
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
            
            QHBoxLayout* bottomLayout = new QHBoxLayout();
            bottomLayout->addStretch();
            
            QPushButton* confBtn = new QPushButton("Configure");
            confBtn->setStyleSheet("background-color: #005A9C; color: white; border-radius: 4px; padding: 5px 10px; border: none;");
            
            std::string dId = dev.id;
            connect(confBtn, &QPushButton::clicked, this, [this, dId]() {
                this->changeDevicePath(dId);
            });
            
            bottomLayout->addWidget(confBtn);
            cardLayout->addLayout(bottomLayout);
            
            grid->addWidget(card, row, col);
            
            col++;
            if (col > 1) {
                col = 0;
                row++;
            }
        }
        mainLayout->addLayout(grid);
    }
    
    mainLayout->addStretch();
}

void MainWindow::changeDevicePath(const std::string& deviceId) {
    requireLoginAndExecute([this, deviceId]() {
        downloadPlugin(deviceId, [this, deviceId](bool success) {
            if (!success) {
                QMessageBox::warning(this, "Plugin Download Failed", "Failed to download plugin for device. Configuration aborted.");
                return;
            }
            
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
        });
    });
}

void MainWindow::logout() {
    auto reply = QMessageBox::question(this, "Logout", "Are you sure you want to logout?",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        if (m_tokenManager) {
            m_tokenManager->deleteTokens();
            spdlog::info("User logged out, tokens deleted.");
        }
        
        m_accessToken = "";
        m_statusLabel->setText("Token: None");
        
        QMessageBox::information(this, "Logged Out", "You have been successfully logged out.");
    }
}

void MainWindow::downloadPlugin(const std::string& deviceId, std::function<void(bool)> onComplete) {
    QString pluginDir = QCoreApplication::applicationDirPath() + "/devices/plugins";
    
    QDir dir;
    if (!dir.exists(pluginDir)) {
        dir.mkpath(pluginDir);
    }
    
    QString scheme = m_config.wserver.useHttps ? "https://" : "http://";
    QString host = m_config.wserver.host;
    if (host == "[IP_ADDRESS]" || host.isEmpty()) {
        host = "127.0.0.1";
    }
    QString urlStr = scheme + host + ":" + QString::number(m_config.wserver.port) + "/api/v1/devices/plugin?id=" + QString::fromStdString(deviceId);
    
    QNetworkRequest request((QUrl(urlStr)));
    request.setRawHeader("Authorization", ("Bearer " + m_accessToken).toUtf8());
    
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkReply* reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, manager, pluginDir, deviceId, onComplete]() {
        reply->deleteLater();
        manager->deleteLater();
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QString filename = "plugin_" + QString::fromStdString(deviceId) + ".dll";
            
            QString disp = reply->rawHeader("Content-Disposition");
            if (disp.contains("filename=")) {
                int idx = disp.indexOf("filename=");
                filename = disp.mid(idx + 9).remove("\"");
            }
            
            QString filePath = pluginDir + "/" + filename;
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
                file.close();
                spdlog::info("Plugin downloaded to: {}", filePath.toStdString());
                onComplete(true);
            } else {
                spdlog::error("Failed to write plugin file: {}", filePath.toStdString());
                onComplete(false);
            }
        } else {
            spdlog::error("Failed to download plugin: {}", reply->errorString().toStdString());
            onComplete(false);
        }
    });
}
