cmake_minimum_required(VERSION 3.14)
project(industrial-telemetry-pipeline LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(telemetry_app
    src/main.cpp
    src/ingestion.cpp
    src/db.cpp
    src/util.cpp
)

target_include_directories(telemetry_app PRIVATE src)
target_link_libraries(telemetry_app PRIVATE odbc)
