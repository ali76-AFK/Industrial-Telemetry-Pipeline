cmake_minimum_required(VERSION 3.16)
project(industrial_telemetry_pipeline_gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(telemetry_app
    main.cpp
    mainwindow.cpp
    db.cpp
    ingestion.cpp
    util.cpp
)

target_include_directories(telemetry_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(telemetry_app PRIVATE
    Qt6::Core
    Qt6::Widgets
    odbc
)
