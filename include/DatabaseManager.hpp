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

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    bool initialize();
    
    // Sync functions
    bool upsertDeviceType(const DeviceType& dt);
    bool upsertDevice(const Device& d);
    bool updateDevicePath(const std::string& deviceId, const std::string& path);
    
    std::vector<Device> getDevices() const;
    std::vector<DeviceType> getDeviceTypes() const;

private:
    sqlite3* db = nullptr;
    bool executeQuery(const std::string& query);
    bool createTables();
};
