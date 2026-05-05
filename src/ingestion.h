#pragma once

#include <string>

#include "db.h"
#include "model.h"

IngestionSummary ingest_csv(
    OdbcContext& db,
    const std::string& csv_path,
    const std::string& reject_log_path
);
