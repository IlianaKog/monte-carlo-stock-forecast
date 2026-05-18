add_executable(test_runner
    test_gbm.cpp
    test_statistics.cpp
    ../src/gbm_engine.cpp
    ../src/statistics.cpp
    ../src/stock_data.cpp
    ../src/json_exporter.cpp
)

target_include_directories(test_runner PRIVATE ../include ../third_party)
target_link_libraries(test_runner PRIVATE OpenMP::OpenMP_CXX)

if(WIN32)
    target_link_libraries(test_runner PRIVATE ws2_32)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_runner PRIVATE
        $<$<CONFIG:Release>:-O3 -march=native -ffast-math>)
elseif(MSVC)
    target_compile_options(test_runner PRIVATE
        $<$<CONFIG:Release>:/O2 /arch:AVX2 /fp:fast>)
endif()

add_test(NAME UnitTests COMMAND test_runner)
