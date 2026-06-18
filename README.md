# Industrial Telemetry Pipeline

<p align="center">
  <img src="./telem.png" alt="Industrial Telemetry Pipeline GUI processing" width="900">
</p>

C++/Qt desktop application for robust telemetry ingestion into Microsoft SQL Server with validation, logging, and reject handling.

## Overview

This project implements a production-style telemetry ingestion workflow for industrial CSV data.  
It allows an operator to connect to a SQL Server database, load telemetry records from CSV files, refresh persisted data, reset and reseed the target table, and capture malformed rows in a reject log instead of failing the entire import.

The project combines desktop software engineering, database integration, and resilient data processing in a single practical application.

## Key Features

- Qt Widgets desktop GUI.
- ODBC-based connection to Microsoft SQL Server.
- Connect and disconnect workflow.
- Refresh button to reload persisted database state.
- Reset and ingest workflow to clear and reseed the telemetry table.
- Ingest-only workflow to append rows.
- Input validation for CSV and reject-log paths.
- Sortable table view after load.
- Operational log panel for status feedback.
- Reject-log support for malformed CSV rows.

## Validation and Error Handling

The application handles common failure cases gracefully:
- missing CSV files,
- invalid reject-log paths,
- database connection failures,
- malformed numeric fields,
- invalid rows written to a reject log instead of stopping the whole ingest.

Valid rows are still inserted even when some records are rejected.

## Tech Stack

- **Language:** C++
- **GUI:** Qt Widgets
- **Database connectivity:** ODBC
- **Database:** Microsoft SQL Server
- **Build system:** CMake
- **Environment:** Linux / Ubuntu

## Workflow

1. Launch the application.
2. Enter DSN, username, password, CSV path, and reject-log path.
3. Connect to load persisted rows from SQL Server.
4. Refresh the table view when needed.
5. Use Reset and Ingest to load a CSV from scratch.
6. Use Ingest Only to append additional rows.
7. Review the reject log if malformed rows are detected.

## Project Structure

```text
industrial-telemetry-pipeline/
├── src/                # Application source files
├── data/               # Sample CSV files and local runtime data
├── build/              # Local build output
├── README.md
└── .gitignore
```

## Build

### Requirements
- C++ compiler with C++17 support
- CMake
- Qt development libraries
- ODBC development libraries
- Configured SQL Server DSN


### Build commands
```bash
mkdir -p build
cd build
cmake ../src
make -j$(nproc)
./telemetry_app
```

## Test Scenarios

### Happy path
- Connect to SQL Server.
- Load existing telemetry rows.
- Run Reset and Ingest with a valid CSV.
- Verify that rows are inserted and displayed.
- Run Ingest Only and verify that rows are appended.

### Bad-row validation
- Add a malformed CSV row such as an invalid numeric `metric_value`.
- Run Reset and Ingest.
- Verify that valid rows are inserted, invalid rows are skipped, and the reject log captures the failure reason.

## Skills Demonstrated

- C++ desktop application development.
- Qt GUI design and event-driven programming.
- SQL Server integration from native applications.
- ODBC connection management.
- CSV parsing and validation.
- Robust error handling and operator feedback.
- Data-ingestion workflow design.
- Demo-ready application testing.

## Future Improvements

- Background worker thread for long-running ingestion jobs.
- Progress bar for large CSV files.
- Export of filtered table views.
- Packaged deployment.
- Unit and integration tests.

## Notes

This repository excludes local build artifacts, runtime logs, and sensitive environment-specific configuration.  
Create your own local DSN and environment settings before running the project.
