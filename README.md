# strictfmt

A strict, rule-based source formatter. No layout heuristics. No bikeshedding.

## Design goals

- Fast, so that you can run it on every commit. 
- Owns whitespace, line breaks, indentation, wrapping, trailing-comma normalization, and control-brace normalization.
- Preserves source tokens, comments, file line-ending style, and supported conditional compilation structure.

## Main Tenets

- Never use vertical alignment.
- Keep formatter-owned chains and lists compact or split item-by-item.
- Use no layout heuristics or weights; use only the break optimizer.
- Use indentation changes as visual group borders; one indentation size for every indentation change.

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

- [docs/format.md](docs/format.md) specifies the source layout produced by formatter.
- [docs/config.md](docs/config.md) specifies formatter configuration and ignore files.
- [docs/command_line.md](docs/command_line.md) specifies the `strictfmt` executable command line.
- [docs/preprocessor.md](docs/preprocessor.md) describes handling of preprocessor directives and conditional compilation.
- [docs/syntax_ambiguities.md](docs/syntax_ambiguities.md) explains treatment of C++ syntax ambiguities. 
- [docs/known_issues.md](docs/known_issues.md) tracks known limitations and planned work.

Development docs:

- [docs/glossary.md](docs/glossary.md) defines shared terminology used across docs.
- [docs/architecture.md](docs/architecture.md) describes code module ownership.
- [docs/break_solver.md](docs/break_solver.md) describes break solver implementation details and allowed speedups.
- [docs/grammar_research.md](docs/grammar_research.md) records parser grammar experiments and their validation outcomes.
- [docs/scanner.md](docs/scanner.md) explains the custom tree-sitter scanner architecture.
- [docs/tests.md](docs/tests.md) explains test strategy, test file placement, and golden fixtures.

## License

`strictfmt` is distributed under the MIT License. Vendored tree-sitter components
retain their upstream MIT notices; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
