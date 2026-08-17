/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file TokenManager.hpp
 * @brief Manages storing and retrieving auth tokens via qtkeychain
 * @version 1.0.0
 * @date 2026-08-16
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#ifndef TOKENMANAGER_HPP
#define TOKENMANAGER_HPP

#include <QObject>
#include <QString>

class TokenManager : public QObject {
    Q_OBJECT
public:
    explicit TokenManager(QObject* parent = nullptr);

    void saveTokens(const QString& access, const QString& refresh);
    void loadTokens();
    void deleteTokens();

signals:
    void tokensLoaded(const QString& access, const QString& refresh);
    void tokensSaved();
    void errorOccurred(const QString& errorMsg);

private:
    void writeKey(const QString& key, const QString& value, bool isLast);
};

#endif // TOKENMANAGER_HPP
