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
#include "ConfigLoader.hpp"

class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(DesktopConfig& config, QWidget* parent = nullptr);
    void setAccessToken(const QString& token);

private slots:
    void showAboutDialog();
    void showProxyDialog();

private:
    QLabel* m_statusLabel;
    QLabel* m_appInfoLabel;
    DesktopConfig& m_config;
};

#endif // MAINWINDOW_HPP
