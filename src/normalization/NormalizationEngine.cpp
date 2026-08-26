#include "normalization/NormalizationEngine.hpp"
#include <spdlog/spdlog.h>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace normalization {

// --- JSON Serialization implementation for MeasurementData ---

void to_json(nlohmann::json& j, const DeviceInfo& d) {
    j = nlohmann::json{{"manufacturer", d.manufacturer}};
    if (d.model) j["model"] = *d.model;
    if (d.serial) j["serial"] = *d.serial;
}

void from_json(const nlohmann::json& j, DeviceInfo& d) {
    j.at("manufacturer").get_to(d.manufacturer);
    if (j.contains("model")) d.model = j.at("model").get<std::string>();
    if (j.contains("serial")) d.serial = j.at("serial").get<std::string>();
}

void to_json(nlohmann::json& j, const MeasurementInfo& m) {
    j = nlohmann::json{
        {"type", m.type},
        {"values", m.values},
        {"timestamp", m.timestamp}
    };
    if (m.series) j["series"] = *m.series;
    if (m.meta) j["meta"] = *m.meta;
}

void from_json(const nlohmann::json& j, MeasurementInfo& m) {
    j.at("type").get_to(m.type);
    j.at("values").get_to(m.values);
    j.at("timestamp").get_to(m.timestamp);
    if (j.contains("series")) m.series = j.at("series").get<std::vector<nlohmann::json>>();
    if (j.contains("meta")) m.meta = j.at("meta").get<std::map<std::string, std::string>>();
}

void to_json(nlohmann::json& j, const PatientInfo& p) {
    j = nlohmann::json{
        {"id", p.id},
        {"name", p.name},
        {"dob", p.dob}
    };
    if (p.meta) j["meta"] = *p.meta;
}

void from_json(const nlohmann::json& j, PatientInfo& p) {
    j.at("id").get_to(p.id);
    j.at("name").get_to(p.name);
    j.at("dob").get_to(p.dob);
    if (j.contains("meta")) p.meta = j.at("meta").get<std::map<std::string, std::string>>();
}

void to_json(nlohmann::json& j, const MeasurementData& m) {
    j = nlohmann::json{
        {"schema_version", m.schema_version},
        {"device", m.device},
        {"measurement", m.measurement}
    };
    if (m.patient) j["patient"] = *m.patient;
    if (m.tags) j["tags"] = *m.tags;
    if (m.raw) j["raw"] = *m.raw;
}

void from_json(const nlohmann::json& j, MeasurementData& m) {
    j.at("schema_version").get_to(m.schema_version);
    j.at("device").get_to(m.device);
    j.at("measurement").get_to(m.measurement);
    if (j.contains("patient")) m.patient = j.at("patient").get<PatientInfo>();
    if (j.contains("tags")) m.tags = j.at("tags").get<std::map<std::string, std::string>>();
    if (j.contains("raw")) m.raw = j.at("raw").get<std::string>();
}

nlohmann::json MeasurementData::toJson() const {
    nlohmann::json j;
    to_json(j, *this);
    return j;
}

MeasurementData MeasurementData::fromJson(const nlohmann::json& j) {
    MeasurementData m;
    from_json(j, m);
    return m;
}

// --- NormalizationEngine implementation ---

NormalizationEngine::NormalizationEngine() {}

std::string NormalizationEngine::getSystemTimestampIso() const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&now_c), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

int NormalizationEngine::calculateClockDriftSeconds(const std::string& device_time_iso, const std::string& system_time_iso) const {
    auto parse_iso = [](const std::string& iso) -> std::time_t {
        std::tm tm = {};
        std::stringstream ss(iso);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        return std::mktime(&tm); // Note: mktime uses local timezone, but we just need delta, so it's roughly ok.
    };

    std::time_t dev_time = parse_iso(device_time_iso);
    std::time_t sys_time = parse_iso(system_time_iso);
    
    return std::abs(static_cast<int>(std::difftime(sys_time, dev_time)));
}

NormalizationResult NormalizationEngine::normalize(
    const DeviceInfo& device,
    const std::string& measurement_type,
    const std::map<std::string, double>& raw_values,
    const std::string& device_timestamp_iso,
    const std::string& raw_base64
) {
    NormalizationResult result;
    result.success = false;

    // 1. Check required fields
    if (device.manufacturer.empty()) {
        result.error_code = NormalizationError::ERR_MISSING_FIELD;
        result.error_message = "Device manufacturer is missing.";
        spdlog::warn("Normalization failed: {}", result.error_message);
        return result;
    }

    if (raw_values.empty()) {
        result.error_code = NormalizationError::ERR_MISSING_FIELD;
        result.error_message = "Measurement values are missing.";
        spdlog::warn("Normalization failed: {}", result.error_message);
        return result;
    }

    // 2. Validate and adjust timestamp (Clock Drift Logic)
    std::string final_timestamp = device_timestamp_iso;
    std::string sys_timestamp = getSystemTimestampIso();
    
    if (device_timestamp_iso.empty()) {
        // Fallback if device provides no time at all
        final_timestamp = sys_timestamp;
    } else {
        int drift = calculateClockDriftSeconds(device_timestamp_iso, sys_timestamp);
        if (drift > MAX_CLOCK_DRIFT_SECONDS) {
            spdlog::warn("Clock drift detected! Drift ({}s) > Max ({}s). Overriding device time with system time.", drift, MAX_CLOCK_DRIFT_SECONDS);
            final_timestamp = sys_timestamp;
        }
    }

    // 3. Assemble the normalized data
    MeasurementData data;
    data.schema_version = "1.0";
    data.device = device;
    data.measurement.type = measurement_type;
    data.measurement.values = raw_values;
    data.measurement.timestamp = final_timestamp;
    
    if (!raw_base64.empty()) {
        data.raw = raw_base64;
    }

    result.success = true;
    result.error_code = NormalizationError::NONE;
    result.data = data;
    
    spdlog::info("Successfully normalized measurement of type '{}' from '{}'", measurement_type, device.manufacturer);
    return result;
}

} // namespace normalization
