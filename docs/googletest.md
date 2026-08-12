# GoogleTest compatibility

This document owns the compatibility notes for the pinned GoogleTest external project. The formatter configuration is in [`external/googletest/.cpp-format`](../external/googletest/.cpp-format), and the authoritative exclusion list is [`external/googletest/.cpp-format-ignore`](../external/googletest/.cpp-format-ignore).

Each excluded file was parsed separately with the GoogleTest configuration. Candidate macro-category changes were retained only when they removed errors without introducing errors elsewhere in the GoogleTest corpus. Eight former exclusions are now supported:

- `googletest/include/gtest/gtest-param-test.h`: `TEST_P`, `INSTANTIATE_TEST_SUITE_P`, and `INSTANTIATE_TEST_CASE_P` definitions intentionally contain generated identifier/declaration fragments, so their definitions are raw.
- `googletest/test/googletest-printers-test.cc`: `EXPECT_PRINT_TO_STRING_` stringizes `value`, so its definition is raw.
- `googlemock/test/gmock-matchers-containers_test.cc`: nested ordinary calls and diagnostic expressions now compose through the normal recursive expression grammar inside statement-argument macro lists.
- `googlemock/test/gmock-matchers-arithmetic_test.cc`: parenthesized conditional expressions in ordinary macro arguments now compose recursively with following binary operators and calls.
- `googlemock/test/gmock-function-mocker_test.cc`: method-macro return types accept dependent references such as `const T&`, and configured macro identifiers retain their role when they are the first indented item in a preprocessor branch.
- `googlemock/include/gmock/gmock-matchers.h`: configured type-specifier macros retain their role after declaration keywords and type modifiers, including `typedef GTEST_REMOVE_REFERENCE_AND_CONST_(T) RawT` and `using Address = const GTEST_REMOVE_REFERENCE_AND_CONST_(Type) *`.
- `googlemock/test/gmock-pp-string_test.cc`: `EXPECT_EXPANSION` uses the preprocessing-token-argument category because its second argument deliberately contains non-C++ token sequences such as `GMOCK_PP_CAT(+, =)`, empty comma arguments, and adjacent calls and identifiers. Its local `JOINER` definition remains structured after complete recursive expressions are preferred over a shallow call-only replacement.
- `googletest/test/gtest_unittest.cc`: configured statement-argument calls now classify after same-line whitespace, including as unbraced control-flow bodies. `VERIFY_CODE_LOCATION` remains raw because its replacement mixes declarations and calls in a sequence that the structured replacement grammar does not compose.

The audit also classified the genuinely non-structural `MATCHER*`, `MY_MOCK_METHODS*`, and `LEGACY_MY_MOCK_METHODS*` definitions as raw. `GTEST_LOG_` was removed from `BareIdentifierMacros`: its uses are ordinary call expressions with stream tails, and bare classification broke its use inside a statement-argument macro.

## Remaining exclusions

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
