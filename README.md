# strictfmt

A strict, rule-based source formatter. No layout heuristics. No bikeshedding.

## Design goals

- Fast enough to run on every commit.
- Syntax-driven, without parsing included files or expanding macros.
- Fully determines whitespace, indentation, and wrapping.
- Preserves source tokens and comments.

## Main tenets

- Never use vertical alignment for code.
- Keep operator chains and lists compact or split them item by item.
- Use no layout heuristics or weights; use a cost-based optimizer.
- Use indentation changes as visual group boundaries, with a consistent step size.

## Example

```cpp
void SayHello(std::string_view requestedGreetingName) {
    if (
        requestedGreetingName.empty() ||
        requestedGreetingName == "hello-greeting-recipient" ||
        requestedGreetingName == "world-greeting-recipient"
    ) {
        std::cout << "Hello, world!\n";
    }
}
```

## Detailed documentation

Detailed user-level docs:

- [docs/format.md](docs/format.md) specifies the source layout produced by the formatter.
- [docs/config.md](docs/config.md) specifies formatter configuration and ignore files.
- [docs/command_line.md](docs/command_line.md) specifies the `strictfmt` executable command line.
- [docs/preprocessor.md](docs/preprocessor.md) describes handling of preprocessor directives and conditional compilation.
- [docs/syntax_ambiguities.md](docs/syntax_ambiguities.md) explains the treatment of C++ syntax ambiguities.
- [docs/known_issues.md](docs/known_issues.md) tracks known limitations and planned work.

Development docs:

- [docs/build.md](docs/build.md) describes local builds, CI validation, and releases.
- [docs/glossary.md](docs/glossary.md) defines shared terminology used across docs.
- [docs/architecture.md](docs/architecture.md) describes code module ownership.
- [docs/break_solver.md](docs/break_solver.md) describes break solver implementation details and allowed speedups.
- [docs/scanner.md](docs/scanner.md) explains the custom tree-sitter scanner architecture.
- [docs/tests.md](docs/tests.md) explains test strategy, test file placement, and golden fixtures.

## License

`strictfmt` is distributed under the MIT License. Vendored tree-sitter components
retain their upstream MIT notices; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
