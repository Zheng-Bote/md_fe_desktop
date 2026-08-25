/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file SyncManager.hpp
 * @brief Header for SyncManager.hpp
 * @version 1.0.0
 * @date 2026-08-25
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#pragma once
#include "DatabaseManager.hpp"
#include "ConfigLoader.hpp"
#include <memory>
#include <QObject>

class SyncManager : public QObject {
    Q_OBJECT
public:
    SyncManager(std::shared_ptr<DatabaseManager> dbMgr, const DesktopConfig& config, QObject* parent = nullptr);
    void performSync(const std::string& jwtToken);

signals:
    void syncFinished(bool success);
    void syncAuthError();

private:
    std::shared_ptr<DatabaseManager> dbManager;
    DesktopConfig m_config;
};
