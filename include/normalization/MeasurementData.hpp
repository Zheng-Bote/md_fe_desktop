#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace normalization {

struct DeviceInfo {
    std::string manufacturer;
    std::optional<std::string> model;
    std::optional<std::string> serial;
};

void to_json(nlohmann::json& j, const DeviceInfo& d);
void from_json(const nlohmann::json& j, DeviceInfo& d);

struct MeasurementInfo {
    std::string type;
    std::map<std::string, double> values;
    std::optional<std::vector<nlohmann::json>> series;
    std::string timestamp; // ISO8601
    std::optional<std::map<std::string, std::string>> meta;
};

void to_json(nlohmann::json& j, const MeasurementInfo& m);
void from_json(const nlohmann::json& j, MeasurementInfo& m);

struct PatientInfo {
    std::string id;
    std::string name;
    std::string dob; // ISO8601
    std::optional<std::map<std::string, std::string>> meta;
};

void to_json(nlohmann::json& j, const PatientInfo& p);
void from_json(const nlohmann::json& j, PatientInfo& p);

struct MeasurementData {
    std::string schema_version = "1.0";
    DeviceInfo device;
    MeasurementInfo measurement;
    std::optional<PatientInfo> patient;
    std::optional<std::map<std::string, std::string>> tags;
    std::optional<std::string> raw; // Base64 encoded

    nlohmann::json toJson() const;
    static MeasurementData fromJson(const nlohmann::json& j);
};

void to_json(nlohmann::json& j, const MeasurementData& m);
void from_json(const nlohmann::json& j, MeasurementData& m);

} // namespace normalization
