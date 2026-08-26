#pragma once

#include <memory>
#include <map>
#include <string>
#include "DatabaseManager.hpp"
#include "normalization/NormalizationEngine.hpp"
#include "plugins/IDevicePlugin.hpp"

namespace core {

class DeviceManager {
public:
    DeviceManager(std::shared_ptr<DatabaseManager> dbManager, const std::string& encryptionKey);
    ~DeviceManager();

    // Starts monitoring a device (loads plugin, sets up callback)
    bool startDevice(const Device& device);

    // Stops monitoring a device
    void stopDevice(const std::string& deviceId);
    
    // Stop all devices
    void stopAll();

private:
    void handleMeasurement(const Device& device,
                           const std::string& measurement_type, 
                           const std::map<std::string, double>& raw_values, 
                           const std::string& timestamp_iso, 
                           const std::string& raw_base64);

    std::shared_ptr<DatabaseManager> m_dbManager;
    std::string m_encryptionKey;
    normalization::NormalizationEngine m_normEngine;
    
    std::map<std::string, std::shared_ptr<plugins::IDevicePlugin>> m_activePlugins;
};

} // namespace core
