# Known issues

This document tracks known limitations and planned work.

## Conditional leading commas are not supported in all lists

Current behavior: Branch-owned leading separator commas are limited to the list positions specified in [preprocessor.md](preprocessor.md).

Planned work: Support branch-owned leading separator commas whenever a conditional branch follows an existing item in any supported comma-separated list.

## `clang-format` control comments are ignored

Current behavior: `// clang-format off` and `// clang-format on` comments remain in the source but do not affect formatting.

Planned work: Decide whether to honor these comments. If strictfmt does not honor them, remove them from formatted source so they do not misleadingly imply that formatting is disabled.

## Comma normalization remains an open issue

Current behavior: Trailing commas in brace lists are normalized from the selected layout, which causes many changes in real code.

Planned work: Decide whether to further tune the rule or stop normalization and preserve trailing commas as written in the source.
