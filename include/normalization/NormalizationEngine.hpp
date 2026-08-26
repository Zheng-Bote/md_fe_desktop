#pragma once

#include "MeasurementData.hpp"
#include <string>
#include <chrono>

namespace normalization {

enum class NormalizationError {
    NONE,
    ERR_MISSING_FIELD,
    ERR_UNIT_INVALID,
    ERR_TIMEOUT,
    ERR_FRAME_INVALID,
    ERR_CHECKSUM_FAILED
};

struct NormalizationResult {
    bool success;
    NormalizationError error_code;
    std::string error_message;
    std::optional<MeasurementData> data;
};

class NormalizationEngine {
public:
    NormalizationEngine();

    /**
     * Validates and normalizes raw data from a device plugin into standard MeasurementData.
     * Checks for required fields and corrects device clock drift.
     */
    NormalizationResult normalize(
        const DeviceInfo& device,
        const std::string& measurement_type,
        const std::map<std::string, double>& raw_values,
        const std::string& device_timestamp_iso,
        const std::string& raw_base64 = ""
    );

private:
    std::string getSystemTimestampIso() const;
    int calculateClockDriftSeconds(const std::string& device_time_iso, const std::string& system_time_iso) const;
    
    // As per architecture concept: max 300 seconds (5 mins) drift allowed before overriding
    const int MAX_CLOCK_DRIFT_SECONDS = 300;
};

} // namespace normalization
