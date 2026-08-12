# Agent Notes

Keep docs up to date with code changes in the same change. Do not duplicate information and logic in docs and in source code. Each piece of key logic must be specified exactly once in a document that owns the corresponding area and implemented in exactly one place in the source code. 

References to all docs and their ownership areas; read them only when needed for the task at hand:

- `README.md`: project overview, goals, and doc links.
- `docs/architecture.md`: general architecture and ownership of different code modules.
- `docs/break_solver.md`: break solver implementation details and allowed optimization speedups.
- `docs/build.md`: build scripts and embedding/test options.
- `docs/command_line.md`: strictfmt executable command-line parameters and behavior.
- `docs/config.md`: formatter configuration.
- `docs/format.md`: specifies source layout produced by formatter.
- `docs/glossary.md`: shared terminology used across docs.
- `docs/known_issues.md`: known limitations and planned work.
- `docs/macro.md`: macro categories and macro formatting. 
- `docs/preprocessor.md`: preprocessor directives and conditional compilation. 
- `docs/scanner.md`: custom tree-sitter scanner architecture and external tokens.
- `docs/syntax_ambiguities.md`: syntax ambiguities.
- `docs/tests.md`: test strategy, test file placement, and golden fixtures.
- `vendor/tree-sitter/README.md`: vendored tree-sitter runtime and owned grammar, read it if you are changing `vendor/tree-sitter/tree-sitter-cpp/grammar.js`.

## Validation

On source code changes:

- Use `scripts\build.sh|cmd` to build.
- Use `scripts\test.sh|cmd` to test (includes build).
