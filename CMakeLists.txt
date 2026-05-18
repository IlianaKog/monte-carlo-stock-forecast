cmake_minimum_required(VERSION 3.16)
project(MonteCarloForecast CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP REQUIRED)

add_executable(forecast
    src/main.cpp
    src/stock_data.cpp
    src/gbm_engine.cpp
    src/statistics.cpp
    src/json_exporter.cpp
    src/server.cpp
)

target_include_directories(forecast PRIVATE include third_party)
target_link_libraries(forecast PRIVATE OpenMP::OpenMP_CXX)

if(WIN32)
    target_link_libraries(forecast PRIVATE ws2_32)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(forecast PRIVATE
        $<$<CONFIG:Release>:-O3 -march=native -ffast-math>)
elseif(MSVC)
    target_compile_options(forecast PRIVATE
        $<$<CONFIG:Release>:/O2 /arch:AVX2 /fp:fast>)
endif()

# ── Tests ────────────────────────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)
