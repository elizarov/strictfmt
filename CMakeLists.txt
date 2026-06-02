cmake_minimum_required(VERSION 3.26)

project(strictfmt LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python3 COMPONENTS Interpreter)
find_package(unofficial-tree-sitter CONFIG REQUIRED)

enable_testing()

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
    src/tools/impl/tools_common.cpp
    src/tools/impl/tools_parallel.cpp
    src/tools/impl/tools_progress.cpp
    src/util/file_path.cpp
    src/util/strings.cpp
    src/util/text_encoding.cpp
)
target_include_directories(strictfmt_util PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
strictfmt_apply_common_options(strictfmt_util)
add_library(strictfmt::util ALIAS strictfmt_util)

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
    unofficial::tree-sitter::tree-sitter
)
if(MSVC)
    target_compile_options(strictfmt_tree_sitter_cpp_grammar PRIVATE /w)
endif()

add_library(strictfmt_core STATIC
    src/tools/format.cpp
    src/tools/impl/format_break_model.cpp
    src/tools/impl/format_break_model_builder.cpp
    src/tools/impl/format_break_solver.cpp
    src/tools/impl/format_config.cpp
    src/tools/impl/format_include_sort.cpp
    src/tools/impl/format_model.cpp
    src/tools/impl/format_model_builder.cpp
    src/tools/impl/format_model_parse.cpp
    src/tools/impl/format_pretty_printer.cpp
    src/tools/impl/format_spacing.cpp
)
target_include_directories(strictfmt_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(strictfmt_core PUBLIC
    strictfmt_util
    strictfmt_tree_sitter_cpp_grammar
)
strictfmt_apply_common_options(strictfmt_core)
add_library(strictfmt::core ALIAS strictfmt_core)

add_library(strictfmt_cli STATIC
    src/tools/format_cli.cpp
    src/tools/format_model_dump.cpp
    src/tools/impl/format_args.cpp
    src/tools/strictfmt_cli.cpp
)
target_include_directories(strictfmt_cli PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(strictfmt_cli PUBLIC
    strictfmt_core
)
strictfmt_apply_common_options(strictfmt_cli)
add_library(strictfmt::cli ALIAS strictfmt_cli)

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

if(Python3_FOUND)
    set(strictfmt_format_test_env
        "STRICTFMT_EXE=$<TARGET_FILE:strictfmt>"
        "STRICTFMT_PROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}"
        "STRICTFMT_TEST_TEMP_ROOT=${CMAKE_BINARY_DIR}/strictfmt-tests"
    )
    add_test(NAME StrictfmtFormatTests
        COMMAND
            "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/format/format_test.py"
    )
    set_tests_properties(StrictfmtFormatTests PROPERTIES
        ENVIRONMENT "${strictfmt_format_test_env}"
    )
    add_custom_target(strictfmt_tests
        COMMAND
            ${CMAKE_COMMAND} -E env ${strictfmt_format_test_env}
            "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/format/format_test.py"
        DEPENDS strictfmt
        VERBATIM
    )
endif()
