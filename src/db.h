#pragma once

#include <sqlext.h>
#include <string>
#include <vector>

#include "config.h"
#include "model.h"

struct OdbcContext {
    SQLHENV hEnv = SQL_NULL_HENV;
    SQLHDBC hDbc = SQL_NULL_HDBC;
    SQLHSTMT hStmt = SQL_NULL_HSTMT;
};

void connect_database(OdbcContext& db, const AppConfig& config);
void cleanup(OdbcContext& db);

void reset_test_table(OdbcContext& db);
void prepare_insert_statement(OdbcContext& db);
void insert_telemetry_row(
    OdbcContext& db,
    const std::string& device_id,
    const std::string& metric_name,
    double metric_value,
    const std::string& event_time_iso8601,
    const std::string& source_type
);

std::vector<DatabaseRow> fetch_inserted_rows(OdbcContext& db);

std::string collect_odbc_diagnostics(SQLSMALLINT handleType, SQLHANDLE handle);
