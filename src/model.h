#pragma once

#include <string>

// Telemetry row from CSV after parsing and validation.
struct TelemetryRow {
    std::string device_id;
    std::string metric_name;
    double metric_value;
    std::string event_time;
    std::string source_type;
};

// Row fetched back from the database for preview/verification.
struct DatabaseRow {
    int sample_id = 0;
    std::string device_id;
    std::string metric_name;
    double metric_value = 0.0;
    std::string event_time_utc;
    std::string ingest_time_utc;
    std::string source_type;
};

// Ingestion result counters.
struct IngestionSummary {
    int inserted = 0;
    int skipped = 0;
};
