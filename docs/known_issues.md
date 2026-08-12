# Known issues

This document tracks known limitations and planned work.

## Conditional leading commas are not supported in all lists

Current behavior: Branch-owned leading separator commas are limited to the list positions specified in [preprocessor.md](preprocessor.md).

Planned work: Support branch-owned leading separator commas whenever a conditional branch follows an existing item in any supported comma-separated list.

## Some GoogleTest source files are not supported

Current behavior: The GoogleTest files listed in [.cpp-format-ignore](../external/googletest/.cpp-format-ignore) are excluded from recursive formatting and parsing coverage. [googletest.md](googletest.md) records the per-file audit, offending shapes, and configuration fixes already applied.

Planned work: Support the remaining GoogleTest source files and remove their ignore entries.
