#include "core/DeviceManager.hpp"
#include "plugins/MaicoGdtPlugin.hpp"
#include "CryptoHelper.hpp"
#include <spdlog/spdlog.h>

using namespace md::crypto;

namespace core {

DeviceManager::DeviceManager(std::shared_ptr<DatabaseManager> dbManager, const std::string& encryptionKey)
    : m_dbManager(dbManager), m_encryptionKey(encryptionKey) {}

DeviceManager::~DeviceManager() {
    stopAll();
}

bool DeviceManager::startDevice(const Device& device) {
    if (m_activePlugins.count(device.id)) {
        spdlog::warn("DeviceManager: Device {} is already running.", device.id);
        return false;
    }

    if (device.local_config_path.empty()) {
        spdlog::warn("DeviceManager: Device {} has no local_config_path. Cannot start.", device.id);
        return false;
    }

    // For Phase 3, we hardcode the MaicoGdtPlugin if interface is GDT.
    // In the future, we would use QPluginLoader to load the downloaded .dll dynamically.
    std::shared_ptr<plugins::IDevicePlugin> plugin;
    
    if (device.interface_name == "GDT" || device.device_name.find("Maico") != std::string::npos) {
        plugin = std::make_shared<plugins::MaicoGdtPlugin>();
    } else {
        // Fallback or generic not implemented yet
        spdlog::error("DeviceManager: No plugin found for device type/interface.");
        return false;
    }

    plugins::DeviceConfig cfg;
    cfg.deviceId = device.id;
    cfg.pathOrPort = device.local_config_path;
    
    auto callback = [this, device](const std::string& measurement_type, 
                                   const std::map<std::string, double>& raw_values, 
                                   const std::string& timestamp_iso, 
                                   const std::string& raw_base64) {
        this->handleMeasurement(device, measurement_type, raw_values, timestamp_iso, raw_base64);
    };

    if (!plugin->init(cfg, callback)) {
        spdlog::error("DeviceManager: Failed to initialize plugin for device {}", device.id);
        return false;
    }

    m_activePlugins[device.id] = plugin;
    spdlog::info("DeviceManager: Successfully started device {}", device.id);
    return true;
}

void DeviceManager::stopDevice(const std::string& deviceId) {
    auto it = m_activePlugins.find(deviceId);
    if (it != m_activePlugins.end()) {
        it->second->shutdown();
        m_activePlugins.erase(it);
        spdlog::info("DeviceManager: Stopped device {}", deviceId);
    }
}

void DeviceManager::stopAll() {
    for (auto& pair : m_activePlugins) {
        pair.second->shutdown();
    }
    m_activePlugins.clear();
}

void DeviceManager::handleMeasurement(const Device& device,
                                      const std::string& measurement_type, 
                                      const std::map<std::string, double>& raw_values, 
                                      const std::string& timestamp_iso, 
                                      const std::string& raw_base64) 
{
    spdlog::info("DeviceManager: Handling measurement from device {}", device.id);
    
    normalization::DeviceInfo devInfo;
    devInfo.manufacturer = device.manufacturer;
    devInfo.model = device.device_name;
    // devInfo.serial could be retrieved if the plugin parsed it
    
    auto result = m_normEngine.normalize(devInfo, measurement_type, raw_values, timestamp_iso, raw_base64);
    
    if (!result.success) {
        spdlog::error("DeviceManager: Normalization failed: {}", result.error_message);
        return;
    }
    
    std::string jsonStr = result.data->toJson().dump();
    
    // Encrypt payload
    std::vector<unsigned char> encrypted = CryptoHelper::encryptAES256(jsonStr, m_encryptionKey);
    
    if (m_dbManager->saveMeasurement(device.id, encrypted)) {
        spdlog::info("DeviceManager: Successfully encrypted and saved measurement for device {}", device.id);
    } else {
        spdlog::error("DeviceManager: Failed to save measurement to DB for device {}", device.id);
    }
}

} // namespace core
