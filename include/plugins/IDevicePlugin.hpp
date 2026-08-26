#pragma once

#include <string>
#include <map>
#include <functional>
#include <QtPlugin>
#include "normalization/MeasurementData.hpp"

namespace plugins {

struct PluginMetadata {
    std::string name;
    std::string shortDescription;
    std::string version;
};

struct DeviceConfig {
    std::string deviceId;
    std::string pathOrPort; // e.g. COM3 or /path/to/gdt/dir
    int baudRate = 9600;
};

// Callback invoked by the plugin when a new measurement is ready.
// The raw values will then be routed to the NormalizationEngine.
using MeasurementCallback = std::function<void(
    const std::string& measurement_type, 
    const std::map<std::string, double>& raw_values, 
    const std::string& timestamp_iso, 
    const std::string& raw_base64
)>;

class IDevicePlugin {
public:
    virtual ~IDevicePlugin() = default;

    // Initialize the plugin with config and a callback for asynchronous data delivery
    virtual bool init(const DeviceConfig& cfg, MeasurementCallback callback) = 0;
    
    // Request a read (for polling devices). Event-driven devices might ignore this.
    virtual void readMeasurement() = 0;
    
    virtual normalization::DeviceInfo getInfo() = 0;
    virtual PluginMetadata getMetadata() const = 0;
    
    virtual void shutdown() = 0;
};

} // namespace plugins

Q_DECLARE_INTERFACE(plugins::IDevicePlugin, "net.hase-zheng.MitM.IDevicePlugin/1.0")
