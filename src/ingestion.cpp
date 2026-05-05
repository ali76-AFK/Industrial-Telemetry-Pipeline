#include "ingestion.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "util.h"

namespace {
std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> cols;
    std::stringstream ss(line);
    std::string item;

    while (std::getline(ss, item, ',')) {
        cols.push_back(trim_copy(item));
    }

    return cols;
}
}

IngestionSummary ingest_csv(
    OdbcContext& db,
    const std::string& csv_path,
    const std::string& reject_log_path
) {
    std::ifstream input(csv_path);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open CSV file: " + csv_path);
    }

    std::ofstream reject_log(reject_log_path, std::ios::out | std::ios::trunc);
    if (!reject_log.is_open()) {
        throw std::runtime_error("Failed to open reject log file: " + reject_log_path);
    }

    IngestionSummary summary;
    std::string line;
    int line_number = 0;

    while (std::getline(input, line)) {
        ++line_number;

        if (trim_copy(line).empty()) {
            continue;
        }

        if (line_number == 1) {
            const auto header_cols = split_csv_line(line);
            if (!header_cols.empty() && to_lower_copy(header_cols[0]) == "device_id") {
                continue;
            }
        }

        const auto cols = split_csv_line(line);
        if (cols.size() != 5) {
            ++summary.skipped;
            reject_log << "Line " << line_number << ": expected 5 columns, got "
                       << cols.size() << " | " << line << "\n";
            continue;
        }

        const std::string& device_id = cols[0];
        const std::string& metric_name = cols[1];
        const std::string& metric_value_text = cols[2];
        const std::string& event_time = cols[3];
        const std::string& source_type = cols[4];

        if (device_id.empty() || metric_name.empty() || metric_value_text.empty() ||
            event_time.empty() || source_type.empty()) {
            ++summary.skipped;
            reject_log << "Line " << line_number << ": one or more required fields are empty | "
                       << line << "\n";
            continue;
        }

        double metric_value = 0.0;
        if (!try_parse_double(metric_value_text, metric_value)) {
            ++summary.skipped;
            reject_log << "Line " << line_number << ": invalid metric_value '" << metric_value_text
                       << "' | " << line << "\n";
            continue;
        }

        if (!is_iso8601_utc_loose(event_time)) {
            ++summary.skipped;
            reject_log << "Line " << line_number << ": invalid event_time '" << event_time
                       << "' | " << line << "\n";
            continue;
        }

        try {
            insert_telemetry_row(db, device_id, metric_name, metric_value, event_time, source_type);
            ++summary.inserted;
        } catch (const std::exception& ex) {
            ++summary.skipped;
            reject_log << "Line " << line_number << ": database insert failed: " << ex.what()
                       << " | " << line << "\n";
        }
    }

    return summary;
}
