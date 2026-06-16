# Testing

This document owns the general testing strategy and test file placement for `strictfmt`.

## Strategy

Formatter tests are driven by `tests/format/format_test.py` through the `strictfmt_tests` CMake target and the `scripts/test.sh|cmd` wrappers described in [build.md](build.md). 

These are end-to-end tests: cases run the built formatter with real command-line arguments, formatter configuration, and temporary files when file-system behavior matters. Golden fixture pairs cover broad formatter behavior, while small inline snippets cover narrow command and configuration edges.

Tests write transient files under `STRICTFMT_TEST_TEMP_ROOT`, defaulting to the
build tree. Golden fixtures are copied before mutation, so tests do not edit
tracked fixtures in place.

## File Placement

- `tests/format/format_test.py` owns the Python test harness and individual test
  cases.
- `tests/format/.cpp-format` owns the default test formatter configuration.
- `tests/format/.cpp-format-userver` owns the userver-oriented test formatter
  configuration.
- `tests/format/src/` owns golden input, formatted output, and diagnostic output
  fixtures.

Place new reusable golden fixtures under `tests/format/src/`. Place
single-purpose inline snippets directly in `format_test.py` when they are small
and exist only to exercise one command or configuration edge.

## Golden Fixtures

- `tests/format/src/format_test_input.cpp` ->
  `tests/format/src/format_test_output.cpp`: broad default-configuration
  formatting coverage for ordinary C++ layout core as documented in [format.md].
- `tests/format/src/format_userver_input.cpp` ->
  `tests/format/src/format_userver_output.cpp`: userver-oriented formatting
  coverage using `.cpp-format-userver`, including configured macro categories as documented in [macro.md] and include grouping.
- `tests/format/src/format_ifdef_input.cpp` ->
  `tests/format/src/format_ifdef_output.cpp`: supported conditional compilation
  and preprocessor formatting coverage using `.cpp-format-userver` as documented in [preprocessor.md].
- `tests/format/src/format_unsupported_input.cpp` ->
  `tests/format/src/format_unsupported_output.cpp`: current formatting for
  parser-recovered unsupported syntactic shapes. This pair records observed
  output only; stable indentation and spacing are not guaranteed.
- `tests/format/src/format_error_input.cpp` ->
  `tests/format/src/format_error_output.txt`: golden diagnostics for recovered
  parse errors.
