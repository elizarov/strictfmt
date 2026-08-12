# GoogleTest compatibility

This document owns the compatibility notes for the pinned GoogleTest external project. The formatter configuration is in [`external/googletest/.cpp-format`](../external/googletest/.cpp-format), and the authoritative exclusion list is [`external/googletest/.cpp-format-ignore`](../external/googletest/.cpp-format-ignore).

Each excluded file was parsed separately with the GoogleTest configuration. Candidate macro-category changes were retained only when they removed errors without introducing errors elsewhere in the GoogleTest corpus. Four former exclusions are now supported:

- `googletest/include/gtest/gtest-param-test.h`: `TEST_P`, `INSTANTIATE_TEST_SUITE_P`, and `INSTANTIATE_TEST_CASE_P` definitions intentionally contain generated identifier/declaration fragments, so their definitions are raw.
- `googletest/test/googletest-printers-test.cc`: `EXPECT_PRINT_TO_STRING_` stringizes `value`, so its definition is raw.
- `googlemock/test/gmock-matchers-containers_test.cc`: nested ordinary calls and diagnostic expressions now compose through the normal recursive expression grammar inside statement-argument macro lists.
- `googletest/test/gtest_unittest.cc`: configured statement-argument calls now classify after same-line whitespace, including as unbraced control-flow bodies. `VERIFY_CODE_LOCATION` remains raw because its replacement mixes declarations and calls in a sequence that the structured replacement grammar does not compose.

The audit also classified the genuinely non-structural `MATCHER*`, `MY_MOCK_METHODS*`, and `LEGACY_MY_MOCK_METHODS*` definitions as raw. `GTEST_LOG_` was removed from `BareIdentifierMacros`: its uses are ordinary call expressions with stream tails, and bare classification broke its use inside a statement-argument macro.

## Remaining exclusions

### `googlemock/test/gmock-pp-string_test.cc`

This test deliberately passes preprocessing-token sequences that are not C++ expressions, types, or statements. Use-side macro categories select supported syntactic roles; they do not make an arbitrary argument token stream raw.

```cpp
EXPECT_EXPANSION("+=", GMOCK_PP_CAT(+, =));
EXPECT_EXPANSION("1", GMOCK_PP_HAS_COMMA(, ));
```

### `googlemock/test/gmock-pp_test.cc`

`GMOCK_PP_FOR_EACH` is used inside a template argument list to generate comma-separated types. Its data argument is the standalone preprocessing token `~`, and the tuple supplies type tokens for expansion rather than a C++ expression at the invocation site.

```cpp
static_assert(Test<GMOCK_PP_FOR_EACH(GMOCK_PP_INTERNAL_TYPE_TEST, ~,
                                     (int, float, double, char))>::kArgs == 4,
              "");
```

The file also passes the name of an empty object-like macro as a token to `GMOCK_PP_IS_EMPTY`; classifying that name as a bare macro does not resolve the generated-identifier ambiguity.

### `googlemock/src/gmock_main.cc`

Conditional compilation selects one of two function headers, while both branches share the body after `#endif`. The Windows branch also owns an include before its header. Selected common-body function starts are not a supported conditional placement.

```cpp
#ifdef GTEST_OS_WINDOWS_MOBILE
#include <tchar.h>
GTEST_API_ int _tmain(int argc, TCHAR** argv) {
#else
GTEST_API_ int main(int argc, char** argv) {
#endif
  std::cout << "Running main() from gmock_main.cc\n";
```

### `googlemock/include/gmock/gmock-matchers.h`

The existing `TypeSpecifierMacros` entry for `GTEST_REMOVE_REFERENCE_AND_CONST_` is sufficient in supported simple type positions, but the configured token still loses to ordinary identifier recovery in several `typedef`, `using`, pointer, and nested trait contexts in this header.

```cpp
typedef GTEST_REMOVE_REFERENCE_AND_CONST_(T) RawT;
using Address = const GTEST_REMOVE_REFERENCE_AND_CONST_(Type) *;
```

Adding another use-side category does not change those parser states. The `MATCHER*` definition failures formerly reported later in this file are fixed by the raw-definition configuration.

### `googlemock/test/gmock-function-mocker_test.cc`

The mock-method specifier tuple contains `STDMETHODCALLTYPE` as a calling-convention token inside `Calltype(...)`. The parser currently expects type identifiers at the empty parameter/specifier boundaries and recovers before it can apply a configured macro role.

```cpp
MOCK_METHOD(int, CTNullary, (), (Calltype(STDMETHODCALLTYPE)));
```

`BareIdentifierMacros` is the semantically closest category, but adding `STDMETHODCALLTYPE` there does not fix this invocation and makes its legacy declaration-macro uses recover as errors. The separate `MY_MOCK_METHODS*` and `LEGACY_MY_MOCK_METHODS*` definition failures are fixed by raw-definition configuration.

### `googlemock/test/gmock-matchers-arithmetic_test.cc`

`MATCHER_P*` is already a call-syntax macro-function family. In this header, the argument parser closes the parenthesized conditional expression before its following `+` and then treats the plus as the start of a separate unary expression.

```cpp
MATCHER_P(DoubleLe, rhs,
          (negation ? "is > " : "is <= ") + PrintToString(rhs)) {
  return arg.value() <= rhs.value();
}
```

No existing macro category changes the argument grammar for this call-syntax macro-function header.

### `googletest/include/gtest/internal/gtest-internal.h`

The parser does not accept a function-like type-producing macro after `typename` in this typedef. Adding `GTEST_BIND_` to `TypeSpecifierMacros`, with or without its existing call-syntax classification, does not change the recovery.

```cpp
typedef typename GTEST_BIND_(TestSel, Type) TestClass;
```

### `googletest/test/googletest-death-test-test.cc`

After correcting the `GTEST_LOG_` classification, the remaining failures are semicolonless diagnostic-pop calls used as the final item of a block. They reduce as ordinary call expressions and acquire a missing semicolon; adding the name to `CallSyntaxMacros` does not give it line-item ownership in this position.

```cpp
GTEST_DISABLE_MSC_WARNINGS_POP_()
}
```

The same shape occurs in the conditional-death-macro switch test.
