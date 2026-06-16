# strictfmt

A strict, rule-based source formatter. No heuristics. No bikeshedding.

## Design goals

- Fast, so that you can run it on every commit. 
- Owns whitespace, line breaks, indentation, wrapping, trailing-comma normalization, and control-brace normalization.
- Preserves source tokens, comments, file line-ending style, and supported conditional compilation structure.

## Main Tenets

- Never use vertical alignment.
- Keep formatter-owned chains and lists compact or split item-by-item.
- Use no heuristics or weights; use only the break optimizer.
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

- [docs/architecture.md](docs/architecture.md) describes code module ownership.
- [docs/command_line.md](docs/command_line.md) specifies the `strictfmt` executable command line.
- [docs/glossary.md](docs/glossary.md) defines shared terminology used across docs.
- [docs/format.md](docs/format.md) specifies the source layout produced by formatter.
- [docs/config.md](docs/config.md) specifies formatter configuration and ignore files.
- [docs/preprocessor.md](docs/preprocessor.md) describes handling of preprocessor directives and conditional compilation.
- [docs/syntax_ambiguities.md](docs/syntax_ambiguities.md) explains treatment of C++ syntax ambiguities. 

## License

`strictfmt` is distributed under the MIT License. Vendored tree-sitter grammars
retain their upstream MIT notices; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
