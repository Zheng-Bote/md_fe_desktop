/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file DatabaseManager.hpp
 * @brief Header for DatabaseManager.hpp
 * @version 1.0.0
 * @date 2026-08-25
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#pragma once
#include <string>
#include <sqlite3.h>
#include <vector>

#ifdef interface
#undef interface
#endif

struct DeviceType {
    std::string id;
    std::string name;
    std::string description;
    std::string created_at;
    std::string last_update;
    bool active;
};

struct Device {
    std::string id;
    std::string type_id;
    std::string device_name;
    std::string manufacturer;
    std::string interface_name;
    std::string description;
    std::string created_at;
    std::string last_update;
    bool active;
    std::string local_config_path; // Frontend configured field
};

struct MeasurementRecord {
    std::string id;
    std::string device_id;
    std::vector<unsigned char> payload_encrypted;
    std::string status; // "pending", "synced", "failed"
    std::string created_at;
};

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    bool initialize();
    
    // Sync functions
    bool upsertDeviceType(const DeviceType& dt);
    bool upsertDevice(const Device& d);
    bool updateDevicePath(const std::string& deviceId, const std::string& path);
    
    // Measurement functions
    bool saveMeasurement(const std::string& device_id, const std::vector<unsigned char>& payload_encrypted);
    std::vector<MeasurementRecord> getUnsyncedMeasurements() const;
    bool updateMeasurementStatus(const std::string& measurement_id, const std::string& status);
    
    std::vector<Device> getDevices() const;
    std::vector<DeviceType> getDeviceTypes() const;

private:
    sqlite3* db = nullptr;
    bool executeQuery(const std::string& query);
    bool createTables();
};
