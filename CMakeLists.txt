cmake_minimum_required(VERSION 3.26)

project(strictfmt LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python3 COMPONENTS Interpreter)
enable_testing()

option(STRICTFMT_BUILD_STANDALONE "Build the standalone strictfmt executable." ON)
option(STRICTFMT_USE_SYSTEM_TREE_SITTER
    "Use an installed tree-sitter runtime instead of the vendored static runtime."
    OFF
)
set(STRICTFMT_FORMAT_TEST_EXE "" CACHE STRING
    "Executable used by strictfmt format tests. Defaults to the standalone strictfmt target."
)
set(STRICTFMT_FORMAT_TEST_EXE_ARGS "" CACHE STRING
    "Fixed arguments passed after STRICTFMT_FORMAT_TEST_EXE and before strictfmt test arguments."
)
set(STRICTFMT_TEST_TEMP_ROOT "${CMAKE_BINARY_DIR}/strictfmt-tests" CACHE PATH
    "Directory used by strictfmt format tests for temporary files."
)

if(STRICTFMT_USE_SYSTEM_TREE_SITTER)
    find_package(unofficial-tree-sitter CONFIG QUIET)
    if(TARGET unofficial::tree-sitter::tree-sitter)
        set(strictfmt_tree_sitter_target unofficial::tree-sitter::tree-sitter)
    endif()
    find_package(PkgConfig QUIET)
    if(NOT strictfmt_tree_sitter_target AND PkgConfig_FOUND)
        pkg_check_modules(TREE_SITTER QUIET IMPORTED_TARGET tree-sitter)
    endif()
    if(TARGET PkgConfig::TREE_SITTER AND NOT strictfmt_tree_sitter_target)
        set(strictfmt_tree_sitter_target PkgConfig::TREE_SITTER)
    endif()
    if(NOT strictfmt_tree_sitter_target)
        message(FATAL_ERROR
            "STRICTFMT_USE_SYSTEM_TREE_SITTER=ON requires unofficial-tree-sitter "
            "or pkg-config package tree-sitter."
        )
    endif()
else()
    add_library(strictfmt_tree_sitter_runtime STATIC
        vendor/tree-sitter/tree-sitter/lib/src/lib.c
    )
    target_include_directories(strictfmt_tree_sitter_runtime PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/tree-sitter/tree-sitter/lib/include"
    )
    set(strictfmt_tree_sitter_target strictfmt_tree_sitter_runtime)
endif()

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
)
target_include_directories(strictfmt_util PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
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
    ${strictfmt_tree_sitter_target}
)
if(MSVC)
    target_compile_options(strictfmt_tree_sitter_cpp_grammar PRIVATE /w)
endif()

add_library(strictfmt_core STATIC
    src/format/format.cpp
    src/format/impl/format_break_model.cpp
    src/format/impl/format_break_model_builder.cpp
    src/format/impl/format_break_solver.cpp
    src/format/impl/format_config.cpp
    src/format/impl/format_include_sort.cpp
    src/format/impl/format_model.cpp
    src/format/impl/format_model_builder.cpp
    src/format/impl/format_model_parse.cpp
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
    ${strictfmt_tree_sitter_target}
)
strictfmt_apply_common_options(strictfmt_core)

add_library(strictfmt_cli STATIC
    src/format/format_cli.cpp
    src/format/format_model_dump.cpp
    src/format/impl/format_args.cpp
    src/format/strictfmt_cli.cpp
)
target_include_directories(strictfmt_cli PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
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
        VERBATIM
    )
    if(strictfmt_format_test_depends)
        add_dependencies(strictfmt_tests ${strictfmt_format_test_depends})
    endif()
endif()
