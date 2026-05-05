#pragma once

#include <string>

struct AppConfig {
    std::string dsn = "telemetry-dev";
    std::string username = "sa";
    std::string password = "";

    std::string default_csv_path = "data/telemetry_samples.csv";
    std::string reject_log_path = "data/rejected_rows.log";
};
