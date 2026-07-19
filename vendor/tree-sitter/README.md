# Tree-Sitter

This directory vendors the tree-sitter runtime and grammar sources used by
`strictfmt`.

- `tree-sitter` is the pinned tree-sitter C runtime from tree-sitter 0.20.6 used
  by the static build. It contains only the runtime `lib` source, headers,
  README, and license files needed by `strictfmt`.
- `tree-sitter-cpp` is based on `tree-sitter-cpp` 0.23.4 with strictfmt parser support applied directly in `grammar.js`.
- `tree-sitter-c` is based on `tree-sitter-c` 0.23.4 and is kept as the C grammar dependency used by the C++ grammar.
- Runtime bindings, prebuilt native packages, WASM packages, and `node_modules`
  are not vendored.
- Each vendored tree-sitter component retains its upstream MIT license in its
  `LICENSE` file.

Grammar changes must follow the parser genericity rule in
[docs/architecture.md](../../docs/architecture.md#parser-genericity): model C++
constructs at their reusable grammar role rather than adding fixture-specific
productions.

Normal builds compile `tree-sitter/lib/src/lib.c`,
`tree-sitter-cpp/src/parser.c`, and `tree-sitter-cpp/src/scanner.c` directly.
The custom scanner architecture is documented in [docs/scanner.md](../../docs/scanner.md).

Runtime and generated-parser changes must follow the hard
[upstream tree-sitter runtime constraint](../../docs/architecture.md#upstream-tree-sitter-runtime).

To regenerate generated grammar outputs after changing `grammar.js`, run:

```sh
python3 tools/regenerate_tree_sitter_grammar.py
```

The regeneration tool downloads the pinned host tree-sitter CLI under `build/` when no `--tree-sitter-cli` path is provided.
