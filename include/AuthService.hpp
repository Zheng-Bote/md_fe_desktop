/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file AuthService.h
 * @brief Handles backend API authentication requests
 * @version 1.0.0
 * @date 2026-08-16
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#ifndef AUTHSERVICE_HPP
#define AUTHSERVICE_HPP

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include "ConfigLoader.hpp"

class AuthService : public QObject {
    Q_OBJECT
public:
    explicit AuthService(const DesktopConfig& config, QObject* parent = nullptr);
    void login(const QString& username, const QString& password, const QString& totpCode = "");

signals:
    void loginSuccess(const QString& accessToken, const QString& refreshToken, bool mustChangePwd);
    void totpRequired();
    void loginFailed(const QString& errorMsg);

private:
    DesktopConfig m_config;
    QNetworkAccessManager* m_manager;
    QString getBaseUrl() const;
};

#endif // AUTHSERVICE_H
