/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file ProxyFactory.hpp
 * @brief Header for ProxyFactory.hpp
 * @version 1.0.0
 * @date 2026-08-25
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#pragma once

#include <QNetworkProxyFactory>
#include <QNetworkProxyQuery>
#include <QString>
#include <QList>

class CustomProxyFactory : public QNetworkProxyFactory {
public:
    CustomProxyFactory(const QNetworkProxy& proxy, const QString& bypassHost)
        : m_proxy(proxy), m_bypassHost(bypassHost) {}

    QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery& query) override {
        QString host = query.peerHostName();
        if (host == m_bypassHost || host == "127.0.0.1" || host == "localhost" || host == "[IP_ADDRESS]") {
            return { QNetworkProxy::NoProxy };
        }
        return { m_proxy };
    }

private:
    QNetworkProxy m_proxy;
    QString m_bypassHost;
};
