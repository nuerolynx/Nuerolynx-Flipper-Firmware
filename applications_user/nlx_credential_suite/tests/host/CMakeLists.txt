cmake_minimum_required(VERSION 3.16)
project(nlx_credential_suite_host_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(
    nlx_credential_suite_tests
    test_nlx_core.c
    ../../core/nlx_credential_result.c
    ../../core/nlx_format_decoder.c
    ../../core/nlx_report.c
    ../../core/nlx_risk_assessment.c
    ../../core/nlx_scan_schedule.c
    ../../handoff/nlx_tool_registry.c)

target_include_directories(nlx_credential_suite_tests PRIVATE ../../core ../../handoff)

if(MSVC)
    target_compile_options(nlx_credential_suite_tests PRIVATE /W4 /WX)
else()
    target_compile_options(nlx_credential_suite_tests PRIVATE -Wall -Wextra -Werror -pedantic)
endif()

enable_testing()
add_test(NAME nlx_credential_suite_tests COMMAND nlx_credential_suite_tests)
