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
