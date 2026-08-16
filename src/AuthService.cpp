/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file AuthService.cpp
 * @brief Handles backend API authentication requests
 * @version 1.0.0
 * @date 2026-08-16
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "AuthService.hpp"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

AuthService::AuthService(const DesktopConfig& config, QObject* parent)
    : QObject(parent), m_config(config), m_manager(new QNetworkAccessManager(this)) {
}

QString AuthService::getBaseUrl() const {
    QString scheme = m_config.wserver.useHttps ? "https://" : "http://";
    QString host = m_config.wserver.host;
    if (host == "[IP_ADDRESS]" || host.isEmpty()) {
        host = "127.0.0.1";
    }
    return scheme + host + ":" + QString::number(m_config.wserver.port) + "/api/v1";
}

void AuthService::login(const QString& username, const QString& password, const QString& totpCode) {
    QUrl url(getBaseUrl() + "/auth/login");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["username"] = username;
    json["password"] = password;
    if (!totpCode.isEmpty()) {
        json["totp_code"] = totpCode;
    }

    QJsonDocument doc(json);
    QNetworkReply* reply = m_manager->post(request, doc.toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray responseData = reply->readAll();

        if (statusCode == 200) {
            QJsonDocument respDoc = QJsonDocument::fromJson(responseData);
            QJsonObject respObj = respDoc.object();
            
            QString accessToken = respObj["access_token"].toString();
            QString refreshToken = respObj["refresh_token"].toString();
            bool mustChangePwd = respObj["must_change_pwd"].toBool();
            
            emit loginSuccess(accessToken, refreshToken, mustChangePwd);
        } else if (statusCode == 428) {
            emit totpRequired();
        } else {
            QString errorMsg = "Login failed";
            if (!responseData.isEmpty()) {
                // Try parsing JSON error
                QJsonDocument errDoc = QJsonDocument::fromJson(responseData);
                if (errDoc.isObject() && errDoc.object().contains("error")) {
                    errorMsg = errDoc.object()["error"].toString();
                } else {
                    errorMsg = QString::fromUtf8(responseData).trimmed();
                }
            }
            emit loginFailed(QString("Error %1: %2").arg(statusCode).arg(errorMsg));
        }
    });
}
