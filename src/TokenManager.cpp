/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file TokenManager.cpp
 * @brief Manages storing and retrieving auth tokens via qtkeychain
 * @version 1.0.0
 * @date 2026-08-16
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "TokenManager.hpp"
#include <qtkeychain/keychain.h>
#include <QDebug>

static const QString SERVICE_NAME = "md_fe_desktop";

TokenManager::TokenManager(QObject* parent) : QObject(parent) {}

void TokenManager::saveTokens(const QString& access, const QString& refresh) {
    auto *job = new QKeychain::WritePasswordJob(SERVICE_NAME, this);
    job->setAutoDelete(true);
    job->setKey("access_token");
    job->setTextData(access);

    connect(job, &QKeychain::Job::finished, [this, refresh](QKeychain::Job* accessJob) {
        if (accessJob->error()) {
            emit errorOccurred("Failed to save access token: " + accessJob->errorString());
            return;
        }

        auto *refreshJob = new QKeychain::WritePasswordJob(SERVICE_NAME, this);
        refreshJob->setAutoDelete(true);
        refreshJob->setKey("refresh_token");
        refreshJob->setTextData(refresh);

        connect(refreshJob, &QKeychain::Job::finished, [this](QKeychain::Job* refJob) {
            if (refJob->error()) {
                emit errorOccurred("Failed to save refresh token: " + refJob->errorString());
            } else {
                emit tokensSaved();
            }
        });
        refreshJob->start();
    });
    job->start();
}

void TokenManager::loadTokens() {
    auto *job = new QKeychain::ReadPasswordJob(SERVICE_NAME, this);
    job->setAutoDelete(true);
    job->setKey("access_token");

    connect(job, &QKeychain::Job::finished, [this](QKeychain::Job* accessJob) {
        if (accessJob->error()) {
            if (accessJob->error() != QKeychain::Error::EntryNotFound) {
                emit errorOccurred("Error reading access token: " + accessJob->errorString());
            } else {
                emit tokensLoaded("", ""); // Not found
            }
            return;
        }

        QString access = static_cast<QKeychain::ReadPasswordJob*>(accessJob)->textData();

        auto *refreshJob = new QKeychain::ReadPasswordJob(SERVICE_NAME, this);
        refreshJob->setAutoDelete(true);
        refreshJob->setKey("refresh_token");

        connect(refreshJob, &QKeychain::Job::finished, [this, access](QKeychain::Job* refJob) {
            if (refJob->error()) {
                emit tokensLoaded(access, ""); // Only access found
            } else {
                QString refresh = static_cast<QKeychain::ReadPasswordJob*>(refJob)->textData();
                emit tokensLoaded(access, refresh);
            }
        });
        refreshJob->start();
    });
    job->start();
}
