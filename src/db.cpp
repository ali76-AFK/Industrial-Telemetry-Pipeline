#include "db.h"

#include <sql.h>
#include <sqlext.h>

#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "util.h"

namespace {
bool is_success(SQLRETURN ret) {
    return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
}

void throw_odbc_error(const std::string& where, SQLSMALLINT handleType, SQLHANDLE handle) {
    std::ostringstream oss;
    oss << where;

    const std::string diag = collect_odbc_diagnostics(handleType, handle);
    if (!diag.empty()) {
        oss << "\n" << diag;
    }

    throw std::runtime_error(oss.str());
}

std::string safe_column_text(SQLHSTMT stmt, SQLUSMALLINT col) {
    char buffer[256] = {0};
    SQLLEN indicator = 0;

    SQLRETURN ret = SQLGetData(stmt, col, SQL_C_CHAR, buffer, sizeof(buffer), &indicator);
    if (!is_success(ret)) {
        return "";
    }

    if (indicator == SQL_NULL_DATA) {
        return "";
    }

    return trim_copy(buffer);
}

SQL_TIMESTAMP_STRUCT parse_iso8601_to_timestamp_struct(const std::string& text) {
    SQL_TIMESTAMP_STRUCT ts{};
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    const int matched = std::sscanf(
        text.c_str(),
        "%d-%d-%dT%d:%d:%d",
        &year, &month, &day, &hour, &minute, &second
    );

    if (matched != 6) {
        throw std::runtime_error("Invalid ISO-8601 timestamp format: " + text);
    }

    ts.year = static_cast<SQLSMALLINT>(year);
    ts.month = static_cast<SQLUSMALLINT>(month);
    ts.day = static_cast<SQLUSMALLINT>(day);
    ts.hour = static_cast<SQLUSMALLINT>(hour);
    ts.minute = static_cast<SQLUSMALLINT>(minute);
    ts.second = static_cast<SQLUSMALLINT>(second);
    ts.fraction = 0;

    return ts;
}
}

std::string collect_odbc_diagnostics(SQLSMALLINT handleType, SQLHANDLE handle) {
    std::ostringstream oss;

    SQLCHAR sqlState[7] = {0};
    SQLINTEGER nativeError = 0;
    SQLCHAR messageText[512] = {0};
    SQLSMALLINT textLength = 0;
    SQLSMALLINT recordNumber = 1;

    while (true) {
        SQLRETURN ret = SQLGetDiagRec(
            handleType,
            handle,
            recordNumber,
            sqlState,
            &nativeError,
            messageText,
            sizeof(messageText),
            &textLength
        );

        if (ret == SQL_NO_DATA) {
            break;
        }

        if (!is_success(ret)) {
            break;
        }

        if (recordNumber > 1) {
            oss << "\n";
        }

        oss << "[" << reinterpret_cast<const char*>(sqlState) << "] "
            << reinterpret_cast<const char*>(messageText)
            << " (NativeError=" << nativeError << ")";

        ++recordNumber;
    }

    return oss.str();
}

void connect_database(OdbcContext& db, const AppConfig& config) {
    cleanup(db);

    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &db.hEnv);
    if (!is_success(ret)) {
        throw std::runtime_error("SQLAllocHandle(SQL_HANDLE_ENV) failed.");
    }

    ret = SQLSetEnvAttr(db.hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (!is_success(ret)) {
        throw_odbc_error("SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION) failed.", SQL_HANDLE_ENV, db.hEnv);
    }

    ret = SQLAllocHandle(SQL_HANDLE_DBC, db.hEnv, &db.hDbc);
    if (!is_success(ret)) {
        throw_odbc_error("SQLAllocHandle(SQL_HANDLE_DBC) failed.", SQL_HANDLE_ENV, db.hEnv);
    }

    std::string connStr =
        "DSN=" + config.dsn +
        ";UID=" + config.username +
        ";PWD=" + config.password + ";";

    SQLCHAR outConnStr[1024];
    SQLSMALLINT outConnStrLen = 0;

    ret = SQLDriverConnect(
        db.hDbc,
        nullptr,
        (SQLCHAR*)connStr.c_str(),
        SQL_NTS,
        outConnStr,
        sizeof(outConnStr),
        &outConnStrLen,
        SQL_DRIVER_NOPROMPT
    );

    if (!is_success(ret)) {
        throw_odbc_error("SQLDriverConnect failed.", SQL_HANDLE_DBC, db.hDbc);
    }
}

void cleanup(OdbcContext& db) {
    if (db.hStmt != SQL_NULL_HSTMT) {
        SQLFreeHandle(SQL_HANDLE_STMT, db.hStmt);
        db.hStmt = SQL_NULL_HSTMT;
    }

    if (db.hDbc != SQL_NULL_HDBC) {
        SQLDisconnect(db.hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, db.hDbc);
        db.hDbc = SQL_NULL_HDBC;
    }

    if (db.hEnv != SQL_NULL_HENV) {
        SQLFreeHandle(SQL_HANDLE_ENV, db.hEnv);
        db.hEnv = SQL_NULL_HENV;
    }
}

void reset_test_table(OdbcContext& db) {
    SQLHSTMT stmt = SQL_NULL_HSTMT;

    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, db.hDbc, &stmt);
    if (!is_success(ret)) {
        throw_odbc_error("SQLAllocHandle(SQL_HANDLE_STMT) failed in reset_test_table.", SQL_HANDLE_DBC, db.hDbc);
    }

    const char* sql =
        "DELETE FROM dbo.telemetry_samples;"
        "DBCC CHECKIDENT ('dbo.telemetry_samples', RESEED, 0);";

    ret = SQLExecDirect(stmt, (SQLCHAR*)sql, SQL_NTS);
    if (!is_success(ret)) {
        std::string diag = collect_odbc_diagnostics(SQL_HANDLE_STMT, stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        throw std::runtime_error("reset_test_table failed.\n" + diag);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
}

void prepare_insert_statement(OdbcContext& db) {
    if (db.hStmt != SQL_NULL_HSTMT) {
        SQLFreeHandle(SQL_HANDLE_STMT, db.hStmt);
        db.hStmt = SQL_NULL_HSTMT;
    }

    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, db.hDbc, &db.hStmt);
    if (!is_success(ret)) {
        throw_odbc_error("SQLAllocHandle(SQL_HANDLE_STMT) failed in prepare_insert_statement.", SQL_HANDLE_DBC, db.hDbc);
    }

    const char* insertSql =
        "INSERT INTO dbo.telemetry_samples "
        "(device_id, metric_name, metric_value, event_time, source_type) "
        "VALUES (?, ?, ?, ?, ?);";

    ret = SQLPrepare(db.hStmt, (SQLCHAR*)insertSql, SQL_NTS);
    if (!is_success(ret)) {
        throw_odbc_error("SQLPrepare failed for telemetry_samples insert.", SQL_HANDLE_STMT, db.hStmt);
    }
}

void insert_telemetry_row(
    OdbcContext& db,
    const std::string& device_id,
    const std::string& metric_name,
    double metric_value,
    const std::string& event_time_iso8601,
    const std::string& source_type
) {
    if (db.hStmt == SQL_NULL_HSTMT) {
        throw std::runtime_error("insert_telemetry_row called before prepare_insert_statement.");
    }

    SQL_TIMESTAMP_STRUCT event_ts = parse_iso8601_to_timestamp_struct(event_time_iso8601);

    SQLLEN deviceIdLen = static_cast<SQLLEN>(device_id.size());
    SQLLEN metricNameLen = static_cast<SQLLEN>(metric_name.size());
    SQLLEN metricValueInd = 0;
    SQLLEN eventTimeInd = sizeof(SQL_TIMESTAMP_STRUCT);
    SQLLEN sourceTypeLen = static_cast<SQLLEN>(source_type.size());

    SQLRETURN ret = SQLBindParameter(
        db.hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
        static_cast<SQLULEN>(device_id.size()), 0,
        (SQLPOINTER)device_id.c_str(), 0, &deviceIdLen
    );
    if (!is_success(ret)) {
        throw_odbc_error("SQLBindParameter failed for device_id.", SQL_HANDLE_STMT, db.hStmt);
    }

    ret = SQLBindParameter(
        db.hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
        static_cast<SQLULEN>(metric_name.size()), 0,
        (SQLPOINTER)metric_name.c_str(), 0, &metricNameLen
    );
    if (!is_success(ret)) {
        throw_odbc_error("SQLBindParameter failed for metric_name.", SQL_HANDLE_STMT, db.hStmt);
    }

    ret = SQLBindParameter(
        db.hStmt, 3, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE,
        0, 0, (SQLPOINTER)&metric_value, 0, &metricValueInd
    );
    if (!is_success(ret)) {
        throw_odbc_error("SQLBindParameter failed for metric_value.", SQL_HANDLE_STMT, db.hStmt);
    }

    ret = SQLBindParameter(
        db.hStmt, 4, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP,
        19, 0, (SQLPOINTER)&event_ts, sizeof(SQL_TIMESTAMP_STRUCT), &eventTimeInd
    );
    if (!is_success(ret)) {
        throw_odbc_error("SQLBindParameter failed for event_time.", SQL_HANDLE_STMT, db.hStmt);
    }

    ret = SQLBindParameter(
        db.hStmt, 5, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
        static_cast<SQLULEN>(source_type.size()), 0,
        (SQLPOINTER)source_type.c_str(), 0, &sourceTypeLen
    );
    if (!is_success(ret)) {
        throw_odbc_error("SQLBindParameter failed for source_type.", SQL_HANDLE_STMT, db.hStmt);
    }

    ret = SQLExecute(db.hStmt);
    if (!is_success(ret)) {
        throw_odbc_error("SQLExecute failed for telemetry_samples insert.", SQL_HANDLE_STMT, db.hStmt);
    }

    SQLCloseCursor(db.hStmt);
    SQLFreeStmt(db.hStmt, SQL_RESET_PARAMS);
}

std::vector<DatabaseRow> fetch_inserted_rows(OdbcContext& db) {
    SQLHSTMT stmt = SQL_NULL_HSTMT;

    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, db.hDbc, &stmt);
    if (!is_success(ret)) {
        throw_odbc_error("SQLAllocHandle(SQL_HANDLE_STMT) failed in fetch_inserted_rows.", SQL_HANDLE_DBC, db.hDbc);
    }

    const char* sql =
        "SELECT sample_id, device_id, metric_name, metric_value, "
        "CONVERT(VARCHAR(33), event_time, 126) AS event_time_utc, "
        "CONVERT(VARCHAR(33), ingest_time, 126) AS ingest_time_utc, "
        "source_type "
        "FROM dbo.telemetry_samples "
        "ORDER BY sample_id;";

    ret = SQLExecDirect(stmt, (SQLCHAR*)sql, SQL_NTS);
    if (!is_success(ret)) {
        std::string diag = collect_odbc_diagnostics(SQL_HANDLE_STMT, stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        throw std::runtime_error("fetch_inserted_rows failed.\n" + diag);
    }

    std::vector<DatabaseRow> rows;

    while ((ret = SQLFetch(stmt)) != SQL_NO_DATA) {
        if (!is_success(ret)) {
            std::string diag = collect_odbc_diagnostics(SQL_HANDLE_STMT, stmt);
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            throw std::runtime_error("SQLFetch failed in fetch_inserted_rows.\n" + diag);
        }

        DatabaseRow row{};

        SQLLEN indicator = 0;
        ret = SQLGetData(stmt, 1, SQL_C_SLONG, &row.sample_id, 0, &indicator);
        if (!is_success(ret)) {
            std::string diag = collect_odbc_diagnostics(SQL_HANDLE_STMT, stmt);
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            throw std::runtime_error("SQLGetData failed for sample_id.\n" + diag);
        }

        row.device_id = safe_column_text(stmt, 2);
        row.metric_name = safe_column_text(stmt, 3);

        ret = SQLGetData(stmt, 4, SQL_C_DOUBLE, &row.metric_value, 0, &indicator);
        if (!is_success(ret)) {
            std::string diag = collect_odbc_diagnostics(SQL_HANDLE_STMT, stmt);
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            throw std::runtime_error("SQLGetData failed for metric_value.\n" + diag);
        }

        row.event_time_utc = safe_column_text(stmt, 5);
        row.ingest_time_utc = safe_column_text(stmt, 6);
        row.source_type = safe_column_text(stmt, 7);

        rows.push_back(row);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return rows;
}
