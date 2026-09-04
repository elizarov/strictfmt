cmake_minimum_required(VERSION 3.26)

project(strictfmt LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(STRICTFMT_VERSION "" CACHE STRING
    "Version embedded in the strictfmt executable. Defaults to the current Git description."
)
set(strictfmt_resolved_version "${STRICTFMT_VERSION}")
if(strictfmt_resolved_version STREQUAL "")
    set(strictfmt_git_describe_result 1)
    set(strictfmt_git_description "")
    find_package(Git QUIET)
    if(Git_FOUND)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" describe --tags --match "v[0-9]*" --always --dirty
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE strictfmt_git_describe_result
            OUTPUT_VARIABLE strictfmt_git_description
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    endif()
    if(strictfmt_git_describe_result EQUAL 0 AND NOT strictfmt_git_description STREQUAL "")
        if(strictfmt_git_description MATCHES "^v(.+)$")
            set(strictfmt_resolved_version "${CMAKE_MATCH_1}")
        else()
            string(REPLACE "-" "." strictfmt_git_description "${strictfmt_git_description}")
            set(strictfmt_resolved_version "0.0.0-dev+g${strictfmt_git_description}")
        endif()
    else()
        set(strictfmt_resolved_version "0.0.0-dev")
    endif()
endif()
if(NOT strictfmt_resolved_version MATCHES "^[0-9A-Za-z][0-9A-Za-z.+-]*$")
    message(FATAL_ERROR "STRICTFMT_VERSION contains unsupported characters: ${strictfmt_resolved_version}")
endif()
message(STATUS "strictfmt version: ${strictfmt_resolved_version}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/strictfmt_version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/strictfmt_version.h"
    @ONLY
)

find_package(Python3 COMPONENTS Interpreter)
enable_testing()

option(STRICTFMT_BUILD_STANDALONE "Build the standalone strictfmt executable." ON)
set(STRICTFMT_FORMAT_TEST_EXE "" CACHE STRING
    "Executable used by strictfmt format tests. Defaults to the standalone strictfmt target."
)
set(STRICTFMT_FORMAT_TEST_EXE_ARGS "" CACHE STRING
    "Fixed arguments passed after STRICTFMT_FORMAT_TEST_EXE and before strictfmt test arguments."
)
set(STRICTFMT_TEST_TEMP_ROOT "${CMAKE_BINARY_DIR}/strictfmt-tests" CACHE PATH
    "Directory used by strictfmt format tests for temporary files."
)

add_library(strictfmt_tree_sitter_runtime STATIC
    vendor/tree-sitter/tree-sitter/lib/src/lib.c
)
target_include_directories(strictfmt_tree_sitter_runtime PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/tree-sitter/tree-sitter/lib/include"
)

function(strictfmt_apply_common_options target)
    target_compile_definitions(${target} PUBLIC
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _USE_STD_VECTOR_ALGORITHMS=0
    )
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /EHsc
            /W4
            /permissive-
            /GR-
            /w14505
            /we4505
            /Zi
            $<$<CONFIG:Release>:/O2>
        )
    endif()
endfunction()

add_library(strictfmt_util STATIC
    src/tools/tools_common.cpp
    src/tools/tools_parallel.cpp
    src/tools/tools_progress.cpp
    src/util/file_path.cpp
    src/util/strings.cpp
    src/util/utf8.cpp
)
target_include_directories(strictfmt_util PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/unicode"
)
strictfmt_apply_common_options(strictfmt_util)

add_library(strictfmt_tree_sitter_cpp_grammar STATIC
    vendor/tree-sitter/tree-sitter-cpp/src/parser.c
    vendor/tree-sitter/tree-sitter-cpp/src/scanner.c
)
target_include_directories(strictfmt_tree_sitter_cpp_grammar
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/tree-sitter/tree-sitter-cpp/src"
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/tree-sitter/tree-sitter-cpp/include"
)
target_link_libraries(strictfmt_tree_sitter_cpp_grammar PUBLIC
    strictfmt_tree_sitter_runtime
)
if(MSVC)
    target_compile_options(strictfmt_tree_sitter_cpp_grammar PRIVATE /w)
endif()

add_library(strictfmt_core STATIC
    src/format/format.cpp
    src/format/impl/format_break_model.cpp
    src/format/impl/format_break_model_builder.cpp
    src/format/impl/format_break_model_dump.cpp
    src/format/impl/format_break_solver.cpp
    src/format/impl/format_value_profile.cpp
    src/format/impl/format_config.cpp
    src/format/impl/format_include_sort.cpp
    src/format/impl/format_model.cpp
    src/format/impl/format_model_builder.cpp
    src/format/impl/format_model_parse.cpp
    src/format/impl/format_print_token_builder.cpp
    src/format/impl/format_pretty_printer.cpp
    src/format/impl/format_preprocessor_validation.cpp
    src/format/impl/format_raw_macro.cpp
    src/format/impl/format_spacing.cpp
)
target_include_directories(strictfmt_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(strictfmt_core PUBLIC
    strictfmt_util
    strictfmt_tree_sitter_cpp_grammar
    strictfmt_tree_sitter_runtime
)
strictfmt_apply_common_options(strictfmt_core)

add_library(strictfmt_cli STATIC
    src/format/format_cli.cpp
    src/format/impl/format_diff.cpp
    src/format/impl/format_model_dump.cpp
    src/format/impl/format_args.cpp
    src/format/strictfmt_cli.cpp
)
target_include_directories(strictfmt_cli PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_include_directories(strictfmt_cli PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
target_link_libraries(strictfmt_cli PUBLIC
    strictfmt_core
)
strictfmt_apply_common_options(strictfmt_cli)

if(STRICTFMT_BUILD_STANDALONE)
    add_executable(strictfmt
        src/strictfmt_main.cpp
    )
    target_link_libraries(strictfmt PRIVATE
        strictfmt_cli
    )
    strictfmt_apply_common_options(strictfmt)
    if(MSVC)
        target_link_options(strictfmt PRIVATE
            /STACK:16777216
        )
    endif()
endif()

if(Python3_FOUND)
    set(strictfmt_format_test_exe "${STRICTFMT_FORMAT_TEST_EXE}")
    set(strictfmt_format_test_depends)
    if(strictfmt_format_test_exe STREQUAL "")
        if(NOT TARGET strictfmt)
            message(FATAL_ERROR "STRICTFMT_FORMAT_TEST_EXE must be set when STRICTFMT_BUILD_STANDALONE is OFF.")
        endif()
        set(strictfmt_format_test_exe "$<TARGET_FILE:strictfmt>")
        list(APPEND strictfmt_format_test_depends strictfmt)
    endif()
    set(strictfmt_format_test_env
        "STRICTFMT_EXE=${strictfmt_format_test_exe}"
        "STRICTFMT_EXE_ARGS=${STRICTFMT_FORMAT_TEST_EXE_ARGS}"
        "STRICTFMT_PROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}"
        "STRICTFMT_TEST_TEMP_ROOT=${STRICTFMT_TEST_TEMP_ROOT}"
        "STRICTFMT_EXPECTED_VERSION=${strictfmt_resolved_version}"
    )
    add_test(NAME StrictfmtFormatTests
        COMMAND
            "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/format/format_test.py"
    )
    set_tests_properties(StrictfmtFormatTests PROPERTIES
        ENVIRONMENT "${strictfmt_format_test_env}"
    )
    add_executable(strictfmt_utf8_tests EXCLUDE_FROM_ALL tests/util/utf8_test.cpp)
    target_link_libraries(strictfmt_utf8_tests PRIVATE strictfmt_util)
    strictfmt_apply_common_options(strictfmt_utf8_tests)
    add_test(NAME StrictfmtUtf8Tests
        COMMAND strictfmt_utf8_tests "${CMAKE_CURRENT_SOURCE_DIR}/vendor/unicode/GraphemeBreakTest.txt"
    )
    add_custom_target(strictfmt_tests
        COMMAND strictfmt_utf8_tests "${CMAKE_CURRENT_SOURCE_DIR}/vendor/unicode/GraphemeBreakTest.txt"
        COMMAND
            ${CMAKE_COMMAND} -E env ${strictfmt_format_test_env}
            "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/format/format_test.py"
        VERBATIM
    )
    add_dependencies(strictfmt_tests strictfmt_utf8_tests)
    if(strictfmt_format_test_depends)
        add_dependencies(strictfmt_tests ${strictfmt_format_test_depends})
    endif()
endif()
