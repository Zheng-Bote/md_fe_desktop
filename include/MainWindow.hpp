/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file MainWindow.h
 * @brief Main application window
 * @version 1.0.0
 * @date 2026-08-16
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QLabel>
#include <QStackedWidget>
#include <QListWidget>
#include <QLineEdit>
#include <memory>
#include "ConfigLoader.hpp"
#include "DatabaseManager.hpp"
#include "TokenManager.hpp"
#include "SyncManager.hpp"
#include <functional>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(DesktopConfig& config, std::shared_ptr<DatabaseManager> dbMgr, TokenManager* tm, QWidget* parent = nullptr);

    void setAccessToken(const QString& token);
    void startBackgroundSync();
    
public slots:
    void logout();
    void showAboutDialog();
    void showProxyDialog();
    void onSyncFinished(bool success);
    void changeDevicePath(const std::string& deviceId);
    void populateDevices();

private:
    void requireLoginAndExecute(std::function<void()> onSuccess);
    void populateMyDevices();
    void populateAllDevices();

    DesktopConfig& m_config;
    QLabel* m_statusLabel;
    QLabel* m_appInfoLabel;
    std::shared_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<SyncManager> m_syncManager;
    TokenManager* m_tokenManager;
    QString m_accessToken;
    
    QStackedWidget* m_stackedWidget;
    QListWidget* m_sidebar;
    QWidget* m_devicesContainer;
    QWidget* m_allDevicesContainer;
    QLineEdit* m_allDevicesSearchInput;
};

#endif // MAINWINDOW_HPP
