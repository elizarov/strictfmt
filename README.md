# strictfmt

A strict, rule-based source formatter. No heuristics. No bikeshedding.

## Design goals

- Fast, so that you can run it on every commit. 
- Owns whitespace, line breaks, indentation, wrapping, trailing-comma normalization, and control-brace normalization.
- Preserves source tokens, comments, and file line-ending style.

## Main Tenets

- Never use vertical alignment.
- Keep formatter-owned chains and lists compact or split item-by-item.
- Use no heuristics or weights; use only the general line-break optimization rule.
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

The formatter behavior and configuration reference live in [docs/format.md](docs/format.md).

## License

`strictfmt` is distributed under the MIT License. Vendored tree-sitter grammars
retain their upstream MIT notices; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
