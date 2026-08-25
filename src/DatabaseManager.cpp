/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file DatabaseManager.cpp
 * @brief Implementation of DatabaseManager.cpp
 * @version 1.0.0
 * @date 2026-08-25
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "DatabaseManager.hpp"
#include <spdlog/spdlog.h>
#include <QCoreApplication>
#include <QDir>

DatabaseManager::DatabaseManager() {}

DatabaseManager::~DatabaseManager() {
    if (db) {
        sqlite3_close(db);
    }
}

bool DatabaseManager::initialize() {
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);
    dir.mkpath("data/db");
    
    QString dbPath = dir.absoluteFilePath("data/db/medical_devices.db");
    
    int rc = sqlite3_open(dbPath.toUtf8().constData(), &db);
    if (rc) {
        spdlog::error("Can't open database: {}", sqlite3_errmsg(db));
        return false;
    }
    
    return createTables();
}

bool DatabaseManager::executeQuery(const std::string& query) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, query.c_str(), nullptr, 0, &errMsg);
    if (rc != SQLITE_OK) {
        spdlog::error("SQL error: {}", errMsg);
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool DatabaseManager::createTables() {
    const char* typesTable = R"(
        CREATE TABLE IF NOT EXISTS device_types (
            id TEXT PRIMARY KEY,
            name TEXT,
            description TEXT,
            created_at TEXT,
            last_update TEXT,
            active INTEGER
        );
    )";
    
    const char* devicesTable = R"(
        CREATE TABLE IF NOT EXISTS devices (
            id TEXT PRIMARY KEY,
            type_id TEXT,
            device_name TEXT,
            manufacturer TEXT,
            interface TEXT,
            description TEXT,
            created_at TEXT,
            last_update TEXT,
            active INTEGER,
            local_config_path TEXT DEFAULT ''
        );
    )";
    
    return executeQuery(typesTable) && executeQuery(devicesTable);
}

bool DatabaseManager::upsertDeviceType(const DeviceType& dt) {
    const char* sql = "INSERT INTO device_types (id, name, description, created_at, last_update, active) "
                      "VALUES (?, ?, ?, ?, ?, ?) "
                      "ON CONFLICT(id) DO UPDATE SET "
                      "name=excluded.name, description=excluded.description, "
                      "created_at=excluded.created_at, last_update=excluded.last_update, "
                      "active=excluded.active;";
                      
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare upsertDeviceType stmt: {}", sqlite3_errmsg(db));
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, dt.id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, dt.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, dt.description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, dt.created_at.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, dt.last_update.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, dt.active ? 1 : 0);
    
    bool result = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return result;
}

bool DatabaseManager::upsertDevice(const Device& d) {
    // We only update if local_config_path is empty (not configured locally)
    const char* sql = "INSERT INTO devices (id, type_id, device_name, manufacturer, interface, description, created_at, last_update, active, local_config_path) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, '') "
                      "ON CONFLICT(id) DO UPDATE SET "
                      "type_id=excluded.type_id, device_name=excluded.device_name, manufacturer=excluded.manufacturer, "
                      "interface=excluded.interface, description=excluded.description, created_at=excluded.created_at, "
                      "last_update=excluded.last_update, active=excluded.active "
                      "WHERE local_config_path = '' OR local_config_path IS NULL;";
                      
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare upsertDevice stmt: {}", sqlite3_errmsg(db));
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, d.id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, d.type_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, d.device_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, d.manufacturer.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, d.interface_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, d.description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, d.created_at.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, d.last_update.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 9, d.active ? 1 : 0);
    
    bool result = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!result) {
        spdlog::error("Failed to execute upsertDevice: {}", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
    return result;
}

bool DatabaseManager::updateDevicePath(const std::string& deviceId, const std::string& path) {
    const char* sql = "UPDATE devices SET local_config_path = ? WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare updateDevicePath stmt: {}", sqlite3_errmsg(db));
        return false;
    }
    sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, deviceId.c_str(), -1, SQLITE_STATIC);
    
    bool result = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Device> DatabaseManager::getDevices() const {
    std::vector<Device> result;
    const char* sql = "SELECT id, type_id, device_name, manufacturer, interface, description, created_at, last_update, active, local_config_path FROM devices;";
    sqlite3_stmt* stmt;
    
    auto safe_str = [](const unsigned char* text) -> std::string {
        return text ? reinterpret_cast<const char*>(text) : "";
    };
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Device d;
            d.id = safe_str(sqlite3_column_text(stmt, 0));
            d.type_id = safe_str(sqlite3_column_text(stmt, 1));
            d.device_name = safe_str(sqlite3_column_text(stmt, 2));
            d.manufacturer = safe_str(sqlite3_column_text(stmt, 3));
            d.interface_name = safe_str(sqlite3_column_text(stmt, 4));
            d.description = safe_str(sqlite3_column_text(stmt, 5));
            d.created_at = safe_str(sqlite3_column_text(stmt, 6));
            d.last_update = safe_str(sqlite3_column_text(stmt, 7));
            d.active = sqlite3_column_int(stmt, 8) != 0;
            d.local_config_path = safe_str(sqlite3_column_text(stmt, 9));
            result.push_back(d);
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

std::vector<DeviceType> DatabaseManager::getDeviceTypes() const {
    std::vector<DeviceType> result;
    const char* sql = "SELECT id, name, description, created_at, last_update, active FROM device_types;";
    sqlite3_stmt* stmt;
    
    auto safe_str = [](const unsigned char* text) -> std::string {
        return text ? reinterpret_cast<const char*>(text) : "";
    };
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            DeviceType dt;
            dt.id = safe_str(sqlite3_column_text(stmt, 0));
            dt.name = safe_str(sqlite3_column_text(stmt, 1));
            dt.description = safe_str(sqlite3_column_text(stmt, 2));
            dt.created_at = safe_str(sqlite3_column_text(stmt, 3));
            dt.last_update = safe_str(sqlite3_column_text(stmt, 4));
            dt.active = sqlite3_column_int(stmt, 5) != 0;
            result.push_back(dt);
        }
        sqlite3_finalize(stmt);
    }
    return result;
}
