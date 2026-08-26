/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file SyncManager.cpp
 * @brief Implementation of SyncManager.cpp
 * @version 1.0.0
 * @date 2026-08-25
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "SyncManager.hpp"
#include <nlohmann/json.hpp>
#include <httplib.h>
#include <spdlog/spdlog.h>
#include <QThread>

#ifdef interface
#undef interface
#endif

#include "fbs/devices_generated.h"

SyncManager::SyncManager(std::shared_ptr<DatabaseManager> dbMgr, const DesktopConfig& config, QObject* parent)
    : QObject(parent), dbManager(dbMgr), m_config(config) {}

void SyncManager::performSync(const std::string& jwtToken) {
    QThread* thread = QThread::create([this, jwtToken]() {
        try {
            std::string host = m_config.wserver.host.toStdString();
            int port = m_config.wserver.port;
            bool useHttps = m_config.wserver.useHttps;

            std::string url = (useHttps ? "https://" : "http://") + host + ":" + std::to_string(port);
            httplib::Client cli(url);
            
            cli.set_bearer_token_auth(jwtToken);
            cli.set_connection_timeout(5);

            // Fetch device types
            auto resTypes = cli.Get("/api/v1/devices/get_device_types");
            if (resTypes && resTypes->status == 200) {
                auto fbTypes = flatbuffers::GetRoot<fbs::DeviceTypeList>(resTypes->body.data());
                if (fbTypes && fbTypes->types()) {
                    for (const auto* dt : *fbTypes->types()) {
                        DeviceType t;
                        if (dt->id()) t.id = dt->id()->str();
                        if (dt->name()) t.name = dt->name()->str();
                        if (dt->description()) t.description = dt->description()->str();
                        if (dt->created_at()) t.created_at = dt->created_at()->str();
                        if (dt->last_update()) t.last_update = dt->last_update()->str();
                        t.active = dt->active();
                        dbManager->upsertDeviceType(t);
                    }
                    spdlog::info("Successfully synced device types.");
                }
            } else {
                if (resTypes) {
                    spdlog::warn("SyncManager: Failed to fetch device types. Status: {}, Body: {}", resTypes->status, resTypes->body);
                    if (resTypes->status == 401) {
                        emit syncAuthError();
                        return;
                    }
                } else {
                    auto err = resTypes.error();
                    spdlog::warn("SyncManager: Failed to fetch device types. Network error: {}", httplib::to_string(err));
                }
            }

            // Fetch devices
            auto resDevices = cli.Get("/api/v1/devices/get_devices");
            if (resDevices && resDevices->status == 200) {
                auto fbDevices = fbs::GetDeviceList(resDevices->body.data());
                if (fbDevices && fbDevices->devices()) {
                    for (const auto* d : *fbDevices->devices()) {
                        Device dev;
                        if (d->id()) dev.id = d->id()->str();
                        if (d->type_id()) dev.type_id = d->type_id()->str();
                        if (d->device_name()) dev.device_name = d->device_name()->str();
                        if (d->manufacturer()) dev.manufacturer = d->manufacturer()->str();
                        if (d->interface()) dev.interface_name = d->interface()->str();
                        if (d->description()) dev.description = d->description()->str();
                        if (d->created_at()) dev.created_at = d->created_at()->str();
                        if (d->last_update()) dev.last_update = d->last_update()->str();
                        dev.active = d->active();
                        dbManager->upsertDevice(dev);
                    }
                    spdlog::info("Successfully synced devices.");
                }
            } else {
                spdlog::warn("SyncManager: Failed to fetch devices.");
            }
            // -- Measurement Upload (Store and Forward) --
            auto pendingMeasurements = dbManager->getUnsyncedMeasurements();
            if (!pendingMeasurements.empty()) {
                nlohmann::json uploadJson;
                uploadJson["measurements"] = nlohmann::json::array();
                
                for (const auto& m : pendingMeasurements) {
                    nlohmann::json mJson;
                    mJson["measurement_id"] = m.id;
                    mJson["device_id"] = m.device_id;
                    
                    // Convert vector<unsigned char> to base64
                    QByteArray ba(reinterpret_cast<const char*>(m.payload_encrypted.data()), m.payload_encrypted.size());
                    mJson["payload_encrypted"] = ba.toBase64().toStdString();
                    
                    uploadJson["measurements"].push_back(mJson);
                }
                
                std::string body = uploadJson.dump();
                auto resUpload = cli.Post("/api/v1/measurements/upload", body, "application/json");
                
                if (resUpload && resUpload->status == 200) {
                    spdlog::info("Successfully uploaded {} measurements.", pendingMeasurements.size());
                    for (const auto& m : pendingMeasurements) {
                        dbManager->updateMeasurementStatus(m.id, "synced");
                    }
                } else {
                    spdlog::warn("SyncManager: Failed to upload measurements. Status: {}", resUpload ? resUpload->status : -1);
                }
            }
            
            emit syncFinished(true);
        } catch (const std::exception& e) {
            spdlog::error("SyncManager: Sync exception: {}", e.what());
            emit syncFinished(false);
        }
    });
    
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}
