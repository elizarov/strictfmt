# Agent Notes

Keep docs up to date with code changes in the same change. Do not duplicate information and logic in docs and in source code. Each piece of key logic must be specified exactly once in a document that owns the corresponding area and implemented in exactly one place in the source code. 

References to all the docs their ownership areas, read them only when needed for the task at hand:

- `README.md`: project overview, goals, and doc links.
- `docs/architecture.md`: general architecture and ownership of different code modules.
- `docs/build.md`: build scripts and embedding/test options.
- `docs/command_line.md`: strictfmt executable command-line parameters and behavior.
- `docs/config.md`: formatter configuration.
- `docs/format.md`: specifies source layout produced by formatter.
- `docs/glossary.md`: shared terminology used across docs.
- `docs/macro.md`: macro categories and macro formatting. 
- `docs/preprocessor.md`: preprocessor directives and conditional compilation. 
- `docs/syntax_ambiguities.md`: syntax ambiguities.

## Validation

- Use `scripts\build.sh|cmd` to build.
- Use `scripts\test.sh|cmd` to test.
