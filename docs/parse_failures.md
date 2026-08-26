# Parse-failure research log

This log records iterative investigations of real-world C++ parsing failures.
The current target is
`/Users/romanelizarov/arcadia/taxi/uservices/services/market-delivery-actualizer`.

## 2026-08-26: market-delivery-actualizer

### Baseline

Command:

```sh
strictfmt -n -r /Users/romanelizarov/arcadia/taxi/uservices/services/market-delivery-actualizer
```

The initial scan checked 288 files (43,209 lines) and reported 245 parse
diagnostics in 69 files. The target-local `.cpp-format` was empty.

The first recurring classes are configuration-dependent macro syntax:

- GoogleTest and userver test definitions (`TEST`, `TEST_P`, `UTEST`, and
  `UTEST_P`) are macro-generated function definitions.
- Test-suite instantiations (`INSTANTIATE_TEST_SUITE_P` and
  `INSTANTIATE_UTEST_SUITE_P`) are namespace-scope macro declarations.
- The project-local `MDA_CREATE_METRIC*` and `MDA_REGISTER_METRIC` calls are
  semicolonless namespace-scope declarations. Their metric-sequence arguments
  contain adjacent parenthesized tuples.
- The corresponding `MDA_*` and `I_MDA_*` definitions deliberately contain
  declaration fragments, token pasting, and alternating preprocessor helper
  tokens, so their replacement lists require raw-definition configuration.

### Macro-configuration iteration

Adding target-local categories for the test definitions, test-suite
instantiations, userver namespace sentinels, semicolonless metric calls, and raw
metric definitions reduced the scan to two diagnostics in one file.

An intermediate trial also put the metric calls in
`PreprocessorArgumentMacros`. That category was unnecessary and incorrect for
this use: the metric tuples already compose through the structured macro
argument grammar, while the preprocessing-token call production does not have
the semicolonless namespace-declaration role. Removing that category allowed
all metric invocations to parse structurally.

The resulting target `.cpp-format` uses:

- `MDA_*` and `I_MDA_*` as raw macro definitions;
- `USERVER_NAMESPACE_BEGIN` and `USERVER_NAMESPACE_END` as bare identifiers;
- the observed GoogleTest and userver definition/instantiation macros as
  call-syntax macros; and
- `MDA_CREATE_METRIC`, `MDA_CREATE_METRIC_STRUCT`, and
  `MDA_REGISTER_METRIC` as semicolonless call macros.

### Generic parser iteration

The remaining file used the valid C++20 designated-initializer form
`.delivery_time_from{...}` twice. The inherited C grammar accepted designated
values only after `=`, so the parser recovered the field designator as an error.

A minimized golden reproducer now covers both `.first{1}` and recursively
nested `.inner{.value{2}}`. The C++ grammar's `initializer_pair` production now
accepts a field designator followed directly by an initializer list. Reusing
the recursive initializer-list nonterminal covers arbitrarily nested braced
values without fixture-specific depth rules.

Formatting the non-empty types in that reproducer also exposed declaration-
group state leaking from a nested field scope into its enclosing file scope.
Declaration-group history is now tracked independently for every declaration
scope, so non-empty type declarations remain separated from sibling types and
following callables just like empty type declarations.

### Result

After rebuilding, the final command checked all 288 files (43,209 lines) with
no parsing failures:

```text
Formatting is required for 177 files. Checked 288 files, 43,209 LOC in 192ms.
```

The nonzero dry-run exit is solely the expected formatting-difference result.
`scripts/build.sh` succeeded, and `scripts/test.sh` passed all 65 tests,
including golden-output reparse and idempotence validation.
