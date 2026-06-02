// Generated from tests/format/.cpp-format and tests/format/.cpp-format-userver by
// tools/regenerate_tree_sitter_grammar.py.
module.exports = {
  macro_categories: {
    calling_convention: [
      // tests/format/.cpp-format
      "CALLBACK",
      "WINAPI",
      // tests/format/.cpp-format-userver
    ],
    raw_macro_function_definition: [
      // tests/format/.cpp-format
      // tests/format/.cpp-format-userver
      "IMPL_UTEST_ASSERT_NO_THROW",
      "IMPL_UTEST_ASSERT_THROW",
      "IMPL_UTEST_ANY_BEGIN",
      "IMPL_UTEST_ANY_END",
      "IMPL_UTEST_HIDE_USER_FIXTURE_BY_TEST_LAUNCHER",
      "IMPL_UTEST_HIDE_USER_FIXTURE_BY_TEST_LAUNCHER_TYPED",
      "IMPL_UTEST_MAKE_USER_FIXTURE_ALIAS_TYPED",
      "IMPL_UTEST_TEST",
      "IMPL_UTEST_TYPED_ANY_BEGIN",
      "IMPL_UTEST_TYPED_ANY_END",
      "TYPED_UTEST*",
      "INSTANTIATE_UTEST*",
      "UTEST*",
      "UPROTO*",
      "LOG*",
      "RET*",
      "UASSERT*",
      "UEXPECT*",
      "UINVARIANT*",
      "USERVER*",
      "UTILS*",
    ],
    function_prefix: [
      // tests/format/.cpp-format
      // tests/format/.cpp-format-userver
      "USERVER_IMPL_NODEBUG",
      "USERVER_IMPL_NODEBUG_INLINE_FUNC",
      "USERVER_IMPL_FORCE_INLINE",
      "USERVER_IMPL_DISABLE_ASAN",
      "USERVER_IMPL_ALWAYS_INLINE_SIMD",
      "USERVER_IMPL_PROTECT_DWCAS_ATTR",
      "ATTRIBUTE*",
      "FORMAT_USERVER*",
    ],
    macro_function_definition: [
      // tests/format/.cpp-format
      "TEST",
      "TEST_F",
      "TEST_P",
      "TYPED_TEST",
      "TYPED_TEST_P",
      "MATCHER",
      "MATCHER_P*",
      // tests/format/.cpp-format-userver
      "UTEST_F_DEATH",
      "TYPED_UTEST",
      "TYPED_UTEST_P_MT",
      "TYPED_UTEST_MT",
      "TYPED_UTEST_P",
      "UTEST_F_MT",
      "UTEST_P_MT",
      "UTEST_MT",
      "UTEST_DEATH",
      "UTEST_F",
      "UTEST_P",
      "UTEST",
    ],
    macro_function_definition_with_trailing_parameters: [
      // tests/format/.cpp-format
      "BENCHMARK_DEFINE_F",
      "BENCHMARK_DEFINE_TEMPLATE_F",
      // tests/format/.cpp-format-userver
      "BENCHMARK_F",
    ],
    call_expression_with_type_arguments_macro: [
      // tests/format/.cpp-format
      "BENCHMARK_TEMPLATE",
      // tests/format/.cpp-format-userver
    ],
    top_level_call_statement: [
      // tests/format/.cpp-format
      "BENCHMARK*",
      "ENUM_STRING_DECLARE",
      // tests/format/.cpp-format-userver
      "INSTANTIATE*",
      "REGISTER_TYPED*",
      "TYPED*",
      "USERVER_DEFINE_STRUCT_SUBSET*",
    ],
    method_declaration_macro: [
      // tests/format/.cpp-format
      "MOCK_METHOD",
      // tests/format/.cpp-format-userver
    ],
    statement_exception_call_macro: [
      // tests/format/.cpp-format
      "EXPECT_THROW",
      "EXPECT_THROW_MSG",
      "ASSERT_THROW",
      // tests/format/.cpp-format-userver
      "UEXPECT_THROW",
      "UEXPECT_THROW_MSG",
      "UASSERT_THROW",
      "UASSERT_THROW_MSG",
    ],
    statement_argument_call_macro: [
      // tests/format/.cpp-format
      "EXPECT_NO_THROW",
      // tests/format/.cpp-format-userver
      "UEXPECT_NO_THROW",
      "UASSERT_NO_THROW",
    ],
    name_macro_call: [
      // tests/format/.cpp-format
      // tests/format/.cpp-format-userver
      "RET_NAME",
      "TYPED_TEST_SUITE_P",
      "TYPED_UTEST_SUITE_P",
    ],
    qualified_identifier_prefix_macro: [
      // tests/format/.cpp-format
      // tests/format/.cpp-format-userver
      "CURL_FORMAT_USERVER_NAMESPACE",
      "CURL_8_13_NAMESPACE",
      "CURL_SSLVERSION_NAMESPACE",
    ],
    top_level_item_macro: [
      // tests/format/.cpp-format
      // tests/format/.cpp-format-userver
      "USERVER_NAMESPACE_BEGIN",
      "USERVER_NAMESPACE_END",
    ],
    type_specifier_macro_call: [
      // tests/format/.cpp-format
      "STACK_OF",
      // tests/format/.cpp-format-userver
    ],
  },
};
