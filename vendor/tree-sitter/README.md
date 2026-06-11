# Tree-Sitter Grammars

This directory vendors the grammar sources and generated parser files used by `strictfmt`.

- `tree-sitter-cpp` is based on `tree-sitter-cpp` 0.23.4 with strictfmt parser support applied directly in `grammar.js`.
- `tree-sitter-c` is based on `tree-sitter-c` 0.23.4 and is kept as the C grammar dependency used by the C++ grammar.
- Runtime bindings, prebuilt native packages, WASM packages, and `node_modules` are not vendored.
- Each vendored grammar retains its upstream MIT license in its `LICENSE` file.

Normal builds compile `tree-sitter-cpp/src/parser.c` and `tree-sitter-cpp/src/scanner.c` directly. To regenerate generated grammar outputs after changing `grammar.js`, run:

```sh
python tools/regenerate_tree_sitter_grammar.py
```

The regeneration tool downloads the pinned host tree-sitter CLI under `build/` when no `--tree-sitter-cli` path is provided.
