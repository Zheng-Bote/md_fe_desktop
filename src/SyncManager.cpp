#include "SyncManager.hpp"
#include <httplib.h>
#include <spdlog/spdlog.h>
#include <QThread>
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
                        if (d->interface()) dev.interface = d->interface()->str();
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
            
            emit syncFinished(true);
        } catch (const std::exception& e) {
            spdlog::error("SyncManager: Sync exception: {}", e.what());
            emit syncFinished(false);
        }
    });
    
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}
