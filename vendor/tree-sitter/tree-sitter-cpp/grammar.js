/**
 * @file C++ grammar for tree-sitter
 * @author Max Brunsfeld <maxbrunsfeld@gmail.com>
 * @author Amaan Qureshi <amaanq12@gmail.com>
 * @author John Drouhard <john@drouhard.dev>
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

const C = require('tree-sitter-c/grammar');

const PREC = Object.assign(C.PREC, {
  LAMBDA: 18,
  NEW: C.PREC.CALL + 1,
  STRUCTURED_BINDING: -1,
  THREE_WAY: C.PREC.RELATIONAL + 1,
});

const FOLD_OPERATORS = [
  '+', '-', '*', '/', '%',
  '^', '&', '|',
  '=', '<', '>',
  '<<', '>>',
  '+=', '-=', '*=', '/=', '%=', '^=', '&=', '|=',
  '>>=', '<<=',
  '==', '!=', '<=', '>=',
  '&&', '||',
  ',',
  '.*', '->*',
  'or', 'and', 'bitor', 'xor', 'bitand', 'not_eq',
];

const ASSIGNMENT_OPERATORS = [
  '=',
  '*=',
  '/=',
  '%=',
  '+=',
  '-=',
  '<<=',
  '>>=',
  '&=',
  '^=',
  '|=',
  'and_eq',
  'or_eq',
  'xor_eq',
];

const PREPROC_IFDEF = 1 << 0;
const PREPROC_ELSE = 1 << 1;
const PREPROC_ELIF = 1 << 2;
const PREPROC_ALL_BRANCH_FORMS = PREPROC_IFDEF | PREPROC_ELSE | PREPROC_ELIF;

const PREPROC_SEMICOLON_RHS = /#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\n(?:[ \t]*(?:(?:\/\/[^\n]*)|(?:[^#\r\n][^\r\n]*))\r?\n)*?[ \t]*[^#\r\n][^\r\n]*;[ \t]*(?:\/\/[^\n]*)?\r?\n(?:#[ \t]*(?:elif|else)[^\n]*\r?\n(?:[ \t]*(?:(?:\/\/[^\n]*)|(?:[^#\r\n][^\r\n]*))\r?\n)*?[ \t]*[^#\r\n][^\r\n]*;[ \t]*(?:\/\/[^\n]*)?\r?\n)*#[ \t]*endif/;
const PREPROC_EXPRESSION_FRAGMENT = /#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\n(?:[ \t]*[^#;{}\r\n][^;{}\n]*\r?\n)+(?:#[ \t]*else[^\n]*\r?\n(?:[ \t]*[^#;{}\r\n][^;{}\n]*\r?\n)+)?#[ \t]*endif(?:\r?\n#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\n(?:[ \t]*[^#;{}\r\n][^;{}\n]*\r?\n)+(?:#[ \t]*else[^\n]*\r?\n(?:[ \t]*[^#;{}\r\n][^;{}\n]*\r?\n)+)?#[ \t]*endif)*/;

function templateDeclarationItem($) {
  return choice(
    $._empty_declaration,
    $.alias_declaration,
    alias($.qualified_type_function_definition, $.function_definition),
    alias($.constructor_or_destructor_declaration, $.declaration),
    alias($.operator_cast_declaration, $.declaration),
    alias($.operator_cast_definition, $.function_definition),
    $.preproc_value_declaration,
    $.declaration,
    $.template_declaration,
    $.function_definition,
    $.concept_definition,
    $.friend_declaration,
  );
}

function constructorOrDestructorBody($) {
  return choice(
    seq(
      optional($.field_initializer_list),
      field('body', $.compound_statement),
    ),
    alias($.constructor_try_statement, $.try_statement),
    $.default_method_clause,
    $.delete_method_clause,
    $.pure_virtual_clause,
  );
}

module.exports = grammar(C, {
  name: 'cpp',

  externals: $ => [
    $.raw_string_delimiter,
    $.raw_string_content,
    $.raw_macro_definition_identifier,
    $.raw_macro_replacement,
    $.bare_macro_identifier,
    $.declaration_prefix_macro_identifier,
    $.call_syntax_macro_identifier,
    $.statement_argument_macro_identifier,
    $.type_specifier_macro_identifier,
    $._preproc_directive_end,
    $._line_break_whitespace,
  ],

  extras: $ => [
    /[ \t\f\v]|\\\r?\n[ \t]*/,
    $._line_break_whitespace,
    $.comment,
  ],

  conflicts: $ => [
    // C
    [$.type_specifier, $._declarator],
    [$.type_specifier, $._type_declarator],
    [$.type_specifier, $._declarator, $._type_declarator],
    [$.type_specifier, $.expression],
    [$.sized_type_specifier, $.expression],
    [$.expression, $.class_specifier],
    [$.expression, $.module_declaration],
    [$.expression, $.module_import_declaration],
    [$._top_level_expression_statement, $.module_declaration],
    [$._top_level_expression_statement, $.module_import_declaration],
    [$._top_level_expression_statement, $.expression_statement],
    [$.expression_statement, $._empty_declaration],
    [$.expression, $._class_name],
    [$.type_specifier, $._class_name],
    [$.type_specifier, $.expression, $._class_name],
    [$.sized_type_specifier],
    [$.attributed_statement],
    [$._declaration_modifiers, $.macro_attribute_replacement_list],
    [$._declaration_modifiers, $.attributed_statement],
    [$._top_level_item, $._top_level_statement],
    [$._top_level_item, $.declaration],
    [$._block_item, $.statement],
    [$._block_item, $.declaration],
    [$._field_declaration_list_item, $.field_declaration],
    [$.type_qualifier, $.extension_expression],

    // C++
    [$.template_function, $.template_type],
    [$.initializer_list],
    [$._block_item, $.preproc_if],
    [$._block_item, $.preproc_ifdef],
    [$._block_item, $.preproc_else],
    [$._block_item, $.preproc_elif],
    [$._block_item, $.preproc_elifdef],
    [$.statement, $.macro_function_definition],
    [$.preproc_if, $.preproc_if_in_top_level],
    [$.preproc_ifdef, $.preproc_ifdef_in_top_level],
    [$._type_declarator, $.template_type],
    [$._type_declarator, $.template_type, $.template_function],
    [$.pointer_type_declarator, $.member_pointer_type_declarator],
    [$.template_function, $.template_type, $.expression],
    [$.template_function, $.template_type, $.qualified_identifier],
    [$.template_function, $.template_type, $.macro_qualified_identifier],
    [$.template_type, $.macro_qualified_identifier],
    [$.template_type, $.qualified_type_identifier],
    [$.qualified_type_identifier, $.qualified_identifier],
    [$.qualified_identifier, $.macro_qualified_identifier],
    [$._declarator, $.macro_qualified_identifier],
    [$.qualified_type_identifier, $._declarator],
    [$._declarator, $._qualified_declaration_type],
    [$._declarator, $._qualified_type_function_declarator],
    [$._declarator, $.expression, $.call_expression, $._qualified_declaration_type, $.qualified_function_declarator],
    [$._declarator, $.qualified_function_declarator],
    [$.comma_expression, $.initializer_list],
    [$.comma_expression, $.preproc_if_in_initializer_list],
    [$.comma_expression, $.preproc_ifdef_in_initializer_list],
    [$.expression, $._template_argument_expression],
    [$.type_specifier, $._template_argument_expression],
    [$.type_specifier, $.expression, $._template_argument_expression],
    [$.expression, $._declarator],
    [$.expression, $.structured_binding_declarator],
    [$.expression, $._declarator, $.type_specifier],
    [$._declarator, $._type_declarator],
    [$.expression, $.identifier_parameter_pack_expansion],
    [$.expression, $._lambda_capture_identifier],
    [$.expression, $._lambda_capture],
    [$.expression, $.structured_binding_declarator, $._lambda_capture_identifier],
    [$.structured_binding_declarator, $._lambda_capture_identifier],
    [$.parameter_list, $.argument_list],
    [$.parameter_list],
    [$.parameter_list, $.abstract_parenthesized_declarator],
    [$.parameter_list, $._fold_operator],
    [$.subscript_argument_list],
    [$.template_parameter_list],
    [$.class_specifier, $.type_parameter_declaration, $.optional_type_parameter_declaration],
    [$.dependent_type, $.type_parameter_declaration, $.optional_type_parameter_declaration],
    [$.type_parameter_declaration, $.optional_type_parameter_declaration],
    [$.requires_parameter_list],
    [$.parameter_declaration, $.variadic_parameter_declaration],
    [$.parameter_declaration, $.optional_parameter_declaration],
    [$.parameter_declaration],
    [$.variadic_declarator],
    [$.variadic_type_parameter_declaration],
    [$.attributed_declarator, $.parameter_declaration],
    [$._class_name, $.type_parameter_declaration, $._scope_resolution],
    [$.type_specifier, $.type_parameter_declaration, $._scope_resolution],
    [$._function_attributes_start, $._function_attributes_end],
    [$.type_specifier, $.call_expression],
    [$._declaration_specifiers, $._constructor_specifiers],
    [$._binary_fold_operator, $._fold_operator],
    [$._function_declarator_seq],
    [$.type_specifier, $.sized_type_specifier],
    [$.type_specifier, $.sized_type_specifier, $.expression],
    [$._type_declarator, $.sized_type_specifier],
    [$.type_specifier, $.expression, $.concatenated_string],
    [$.type_specifier, $.concatenated_string],
    [$.expression, $.concatenated_string],
    [$._declaration_specifiers, $.macro_replacement_list],
    [$._string, $.concatenated_string],
    [$.type_specifier, $.macro_template_declaration],
    [$.macro_call_item, $.expression_statement],
    [$.macro_call_item, $.type_specifier_macro_call, $.macro_call_expression],
    [$.type_specifier_macro_call, $.macro_call_expression],
    [$.macro_call_item, $.macro_prefixed_declaration],
    [$.macro_call_item, $.macro_prefixed_function_definition],
    [$._declarator, $._field_declarator, $._type_declarator],
    [$._declarator, $._field_declarator],
    [$.template_method, $.template_function],
    [$.template_type, $.template_method],
    [$.enum_specifier, $.macro_enum_specifier],
    [$.enumerator_list, $.macro_enumerator_list],
    [$.enumerator, $.expression],
    [$.expression_statement, $.macro_expression_item],
    [$.comma_expression, $.macro_expression_item],
    [$.parenthesized_expression, $._macro_argument_list_item],
    [$.comma_expression, $._macro_argument_list_item, $._unary_right_fold, $._binary_fold],
    [$._macro_argument_list_item, $._unary_left_fold],
    [$._parameter_list_item, $._unary_left_fold],
    [$._declaration_modifiers, $.type_descriptor],
    [$._declaration_specifiers, $.type_descriptor],
    [$.type_descriptor, $.macro_qualified_identifier],
    [$.type_descriptor, $.calling_convention_macro],
    [$.type_descriptor, $.calling_convention_macro, $.macro_qualified_identifier],
    [$._declarator, $._constructor_or_destructor_header],
    [$._declarator],
    [$.type_specifier, $._macro_argument_list_item],
    [$.argument_list, $.macro_argument_sequence],
    [$.argument_list, $.macro_argument_list],
    [$.argument_list, $.braced_argument_list],
    [$.argument_list, $.macro_statement_argument_list],
    [$.argument_list],
    [$._macro_argument_list_item, $._argument_list_item],
    [$._macro_argument_list_item, $.macro_call_statement_item, $._argument_list_item],
    [$._macro_argument_list_item, $.macro_single_statement_argument, $._argument_list_item],
    [$.macro_single_statement_argument, $._argument_list_item],
    [$.macro_call_statement_item, $._argument_list_item],
    [$._macro_argument_list_item, $.macro_call_statement_item],
    [$._macro_argument_list_item, $.macro_call_statement_argument],
    [$._macro_argument_list_item, $.macro_single_statement_argument],
    [$._macro_argument_list_item, $.macro_expression_without_semicolon],
    [$._macro_argument_list_item, $.macro_expression_without_semicolon, $._argument_list_item],
    [$.macro_method_declaration, $._macro_argument_list_item],
    [$.initializer_pair, $.comma_expression],
    [$.initializer_list, $._initializer_list_with_preproc],
    [$.comma_expression, $.initializer_list, $._initializer_list_with_preproc],
    [$.expression_statement, $._for_statement_body],
    [$.init_statement, $._for_statement_body],
    [$.field_expression, $.template_method, $.template_type],
    [$.qualified_field_identifier, $.template_method, $.template_type],
    [$.qualified_field_identifier, $.template_method],
    [$.template_type, $.template_method, $.dependent_field_identifier],
    [$._function_declaration_declarator, $._function_attributes_start],
    [$.top_level_item_macro, $.function_prefix_macro],
    [$.top_level_item_macro, $.function_prefix_macro, $.calling_convention_macro],
    [$.top_level_item_macro, $.function_prefix_macro, $.calling_convention_macro, $.macro_qualified_identifier],
    [$.function_prefix_macro, $.calling_convention_macro],
    [$.calling_convention_macro, $.macro_qualified_identifier],
    [$.function_prefix_macro, $.macro_qualified_identifier],
    [$.function_prefix_macro, $.calling_convention_macro, $.macro_qualified_identifier],
    [$._declaration_modifiers, $.macro_prefixed_function_definition, $.macro_prefixed_declaration],
    [$._declarator, $.reference_argument_declarator],
    [$.if_statement, $.preproc_selected_else_if_statement],
    [$._block_item, $.preproc_selected_else_if_body_item],
    [$.statement, $.preproc_selected_else_if_body_item],
    [$._block_item, $.statement, $.preproc_selected_else_if_body_item],
  ],

  inline: ($, original) => original.concat([
    $._namespace_identifier,
  ]),

  precedences: $ => [
    [$.argument_list, $.type_qualifier],
    [$._expression_not_binary, $._class_name],
  ],

  rules: {
    _top_level_item: ($, original) => choice(
      alias($.qualified_type_function_definition, $.function_definition),
      $.deleted_operator_declaration,
      $.preproc_unbalanced_else_block,
      $.preproc_value_declaration,
      alias($.preproc_guarded_namespace_definition, $.namespace_definition),
      alias($.preproc_if_in_top_level, $.preproc_if),
      alias($.preproc_ifdef_in_top_level, $.preproc_ifdef),
      $.preproc_disabled_block,
      $.top_level_macro_run_item,
      alias($.macro_prefixed_function_definition, $.function_definition),
      alias($.macro_prefixed_declaration, $.declaration),
      $.function_definition,
      $.linkage_specification,
      $.declaration,
      $.attributed_statement,
      $.type_definition,
      $._empty_declaration,
      $.preproc_include,
      $.preproc_def,
      $.preproc_function_def,
      $.preproc_call,
      $.preproc_using,
      $.conditional_extern_c_open,
      $.conditional_extern_c_close,
      $.standalone_attribute_preproc_if,
      $.standalone_qualifier_preproc_if,
      $.namespace_definition,
      $.concept_definition,
      $.module_declaration,
      $.module_import_declaration,
      $.namespace_alias_definition,
      $.using_declaration,
      $.function_pointer_alias_declaration,
      $.function_type_alias_declaration,
      $.deduction_guide_declaration,
      $.alias_declaration,
      $.top_level_item_macro,
      $.macro_function_definition,
      $.preproc_selected_macro_function_definition,
      $.top_level_macro_call_line_item,
      $.macro_call_item,
      prec(1, $.top_level_call_statement),
      $.static_assert_declaration,
      $.template_declaration,
      $.template_instantiation,
      alias($.constructor_or_destructor_definition, $.function_definition),
      alias($.operator_cast_definition, $.function_definition),
      alias($.operator_cast_declaration, $.declaration),
    ),

    _block_item: $ => choice(
      $.preproc_unbalanced_else_block,
      prec(2, $.preproc_call),
      prec(2, $.preproc_def),
      prec(2, $.preproc_function_def),
      $.preproc_value_declaration,
      $.preproc_disabled_block,
      $.block_macro_call_statement_item,
      $.block_macro_call_line_item,
      $.declaration,
      $.statement,
      $.type_definition,
      $._empty_declaration,
      $.preproc_if,
      $.preproc_ifdef,
      $.preproc_include,
      $.preproc_using,
      $.standalone_attribute_preproc_if,
      $.standalone_qualifier_preproc_if,
      $.namespace_alias_definition,
      $.using_declaration,
      $.function_pointer_alias_declaration,
      $.function_type_alias_declaration,
      $.alias_declaration,
      $.static_assert_declaration,
    ),

    declaration_list: $ => seq(
      '{',
      repeat($._top_level_item),
      '}',
    ),

    // Types

    placeholder_type_specifier: $ => prec(1, seq(
      field('constraint', optional($.type_specifier)),
      choice($.auto, alias($.decltype_auto, $.decltype)),
    )),

    auto: _ => 'auto',
    decltype_auto: $ => seq(
      'decltype',
      '(',
      $.auto,
      ')',
    ),
    decltype: $ => seq(
      'decltype',
      '(',
      choice($.expression, $.comma_expression),
      ')',
    ),

    _empty_declaration: ($, original) => choice(
      original,
      ';',
    ),

    type_specifier: $ => choice(
      $.struct_specifier,
      $.union_specifier,
      $.enum_specifier,
      $.class_specifier,
      $.sized_type_specifier,
      $.primitive_type,
      $.type_specifier_macro_call,
      $.template_type,
      $.dependent_type,
      $.placeholder_type_specifier,
      $.decltype,
      prec.right(choice(
        prec.dynamic(1, alias($.qualified_type_identifier, $.qualified_identifier)),
        $._type_identifier,
      )),
    ),

    type_qualifier: ($, original) => choice(
      original,
      'mutable',
      'constinit',
      'consteval',
    ),

    _declaration_specifiers: $ => prec.right(seq(
      repeat($._declaration_modifiers),
      field('type', $.type_specifier),
      repeat($._declaration_modifiers),
      repeat($.post_type_macro_annotation),
    )),

    preproc_def: $ => choice(
      prec(2, seq(
        preprocessor('define'),
        field('name', alias($.raw_macro_definition_identifier, $.identifier)),
        field('value', optional($.raw_macro_replacement)),
        $._preproc_directive_end,
      )),
      prec(1, seq(
        preprocessor('define'),
        field('name', choice($.identifier, $.call_syntax_macro_identifier, $.bare_macro_identifier)),
        choice(
          field('value', $.macro_attribute_replacement_list),
          field('value', $.macro_replacement_list),
          $._preproc_directive_end,
        ),
      )),
    ),

    preproc_function_def: $ => choice(
      prec(2, seq(
        preprocessor('define'),
        field('name', alias($.raw_macro_definition_identifier, $.identifier)),
        field('parameters', $.preproc_params),
        field('value', optional($.raw_macro_replacement)),
        $._preproc_directive_end,
      )),
      prec(1, seq(
        preprocessor('define'),
        field('name', choice($.identifier, $.call_syntax_macro_identifier, $.bare_macro_identifier)),
        field('parameters', $.preproc_params),
        choice(
          field('value', $.macro_attribute_replacement_list),
          field('value', $.macro_replacement_list),
          $._preproc_directive_end,
        ),
      )),
    ),

    preproc_include: $ => seq(
      preprocessorInclude(),
      field('path', choice(
        $.string_literal,
        $.system_lib_string,
        $.identifier,
        alias($.preproc_call_expression, $.call_expression),
      )),
      $._preproc_directive_end,
    ),

    preproc_call: $ => seq(
      field('directive', $.preproc_directive),
      field('argument', optional($.preproc_arg)),
      $._preproc_directive_end,
    ),

    macro_replacement_list: $ => seq(
      choice(
        $._macro_replacement_declaration_sequence,
        $._macro_replacement_fragment_sequence,
        $._macro_replacement_statement_item,
      ),
      $._preproc_directive_end,
    ),

    macro_attribute_replacement_list: $ => seq(
      repeat1($.attribute_declaration),
      $._preproc_directive_end,
    ),

    _macro_replacement_declaration_item: $ => choice(
      alias($.macro_template_declaration, $.template_declaration),
      alias($.macro_enum_declaration, $.declaration),
      $.function_definition,
      $.declaration,
    ),

    _macro_replacement_declaration_sequence: $ => prec.right(seq(
      $._macro_replacement_declaration_item,
      optional($._macro_replacement_declaration_sequence),
    )),

    _macro_replacement_call_unit: $ => seq(
      $.macro_call_replacement_item,
      optional($.initializer_list),
    ),

    _macro_replacement_call_sequence: $ => prec.right(seq(
      $._macro_replacement_call_unit,
      optional($._macro_replacement_call_sequence),
    )),

    _macro_replacement_fragment_sequence: $ => choice(
      $._macro_replacement_call_sequence,
      $.macro_string_replacement_item,
      $.macro_expression_item,
      seq(
        $.macro_function_header_fragment,
        optional($.macro_expression_item),
      ),
      $.macro_declaration_fragment,
      $.macro_arrow_chain,
      $.expression_statement,
      $.ms_call_modifier,
    ),

    _macro_replacement_statement_item: $ => choice(
      alias($.macro_do_statement, $.do_statement),
      $.try_statement,
    ),

    macro_do_statement: $ => prec(1, seq(
      'do',
      field('body', $.compound_statement),
      'while',
      field('condition', $.parenthesized_expression),
    )),

    macro_expression_item: $ => seq(
      $.expression,
      optional(','),
    ),

    macro_call_replacement_item: $ => prec.left(PREC.CALL + 4, seq(
      field('function', $.identifier),
      field('arguments', $.macro_argument_list),
      optional(';'),
    )),

    macro_string_replacement_item: $ => prec(PREC.CALL + 7, choice(
      $.concatenated_string,
      $.suffixed_string_literal,
      $.raw_string_literal,
      $.string_literal,
    )),

    macro_token_paste_identifier: _ => token(prec(1, /[A-Za-z_]\w*(?:##(?:[A-Za-z_]\w*|\d+))+/)),

    macro_declaration_fragment: $ => choice(
      prec(PREC.CALL + 2, seq(
        $._macro_declaration_fragment_type,
        field('declarator', $.macro_call_declarator_fragment),
      )),
      prec(1, seq(
        $._macro_declaration_fragment_type,
        field('declarator', seq(
          field('name', $.identifier),
          '=',
          field('default_value', choice($.expression, $.initializer_list)),
        )),
      )),
    ),

    _macro_declaration_fragment_type: $ => prec(1, seq(
      repeat($._declaration_modifiers),
      field('type', choice(
        alias(choice('signed', 'unsigned', 'long', 'short'), $.primitive_type),
        $.primitive_type,
        $.placeholder_type_specifier,
      )),
      repeat($._declaration_modifiers),
    )),

    macro_call_declarator_fragment: $ => seq(
      field('name', $.identifier),
      field('arguments', $.macro_argument_list),
    ),

    macro_arrow_chain: $ => prec.right(repeat1($.macro_arrow_call)),

    macro_arrow_call: $ => seq(
      '->',
      field('function', $._field_identifier),
      field('arguments', $.macro_argument_list),
    ),

    top_level_item_macro: $ => prec(PREC.CALL + 5, $.bare_macro_identifier),

    top_level_decorator_macro: $ => prec(PREC.CALL + 8, $.macro_decorator_call_item),

    class_bare_macro_item: $ => seq($.bare_macro_identifier, ';'),

    class_macro_call_item: $ => prec.right(PREC.CALL + 8, seq(
      $.macro_call_item,
      repeat($.macro_call_item),
      optional(';'),
    )),

    block_macro_call_line_item: $ => prec.dynamic(10, prec.right(PREC.CALL + 8, seq(
      field('function', $.semicolonless_macro_call_identifier_fallback),
      field('arguments', $.macro_argument_list),
      $._line_break_whitespace,
      repeat(seq(
        field('function', $.semicolonless_macro_call_identifier_fallback),
        field('arguments', $.macro_argument_list),
        $._line_break_whitespace,
      )),
    ))),

    block_macro_call_statement_item: $ => prec.dynamic(10, prec.right(PREC.CALL + 8, seq(
      field('function', $.semicolonless_macro_call_identifier_fallback),
      field('arguments', $.macro_argument_list),
      ';',
      repeat(seq(
        field('function', $.semicolonless_macro_call_identifier_fallback),
        field('arguments', $.macro_argument_list),
        ';',
      )),
    ))),

    top_level_call_statement: $ => prec(PREC.CALL + 4, seq(
      field('function', $.call_syntax_macro_identifier),
      field('arguments', $.macro_argument_list),
      optional(field('suffix', choice(
        $.bare_macro_identifier,
        $.top_level_call_suffix_identifier_fallback,
      ))),
      optional($.macro_arrow_chain),
      ';',
    )),

    top_level_call_suffix_identifier_fallback: _ => token(prec(
      1000,
      /BENCHMARK_THREAD_ARGS/,
    )),

    top_level_macro_call_line_item: $ => prec(PREC.CALL + 8, seq(
      field('function', $.semicolonless_macro_call_identifier_fallback),
      field('arguments', $.macro_argument_list),
      $._line_break_whitespace,
    )),

    semicolonless_macro_call_identifier_fallback: _ => token(prec(
      1000,
      /RET_NAME|UTILS_STRONG_TYPEDEF_REL_OP|RSEQ_INJECT_C|GMOCK_(?:INTERNAL_PARSE_FLAG|KIND_OF_)|THOUSAND_TESTS_|GTEST_IMPL_CMP_HELPER_|GTEST_(?:DISABLE_MSC_(?:DEPRECATED_|WARNINGS_)(?:PUSH_|POP_)|INTENTIONAL_CONST_COND_(?:PUSH_|POP_))/,
    )),

    macro_call_item: $ => prec.right(PREC.CALL + 2, seq(
      field('function', $.macro_call_identifier),
      field('arguments', $.macro_argument_list),
    )),

    macro_call_identifier: $ => choice(
      $.call_syntax_macro_identifier,
      $.semicolonless_macro_call_identifier_fallback,
    ),

    macro_decorator_call_item: $ => prec(PREC.CALL + 8, seq(
      field('function', $.call_syntax_macro_identifier),
      field('arguments', $.macro_argument_list),
    )),

    commented_macro_call_item: $ => prec(PREC.CALL + 1, seq(
      field('function', $.call_syntax_macro_identifier),
      field('arguments', $.commented_macro_argument_list),
    )),

    commented_macro_argument_list: _ => token.immediate(prec(1, /\((?:[^()]|\r?\n)*\/(?:\/|\*)(?:[^()]|\r?\n)*\)/)),

    top_level_macro_run_item: $ => prec(PREC.CALL + 8, seq(
      $.top_level_decorator_macro,
      repeat1($.top_level_decorator_macro),
    )),

    macro_prefixed_function_definition: $ => prec(PREC.CALL + 6, seq(
      $.function_prefix_macro,
      choice(
        $.function_definition,
        alias($.qualified_type_function_definition, $.function_definition),
        alias($.constructor_or_destructor_definition, $.function_definition),
        $.macro_function_definition,
        alias($.preproc_selected_macro_function_definition, $.function_definition),
      ),
    )),

    macro_prefixed_declaration: $ => prec(PREC.CALL + 6, seq(
      $.function_prefix_macro,
      choice(
        $.declaration,
        alias($.constructor_or_destructor_declaration, $.declaration),
      ),
    )),

    macro_prefixed_field_declaration_item: $ => prec(PREC.CALL + 6, seq(
      $.function_prefix_macro,
      choice(
        alias($.inline_method_definition, $.function_definition),
        $.field_declaration,
        alias($.constructor_or_destructor_definition, $.function_definition),
        alias($.constructor_or_destructor_declaration, $.declaration),
        alias($.operator_cast_definition, $.function_definition),
        alias($.operator_cast_declaration, $.declaration),
      ),
    )),

    macro_function_definition: $ => prec.right(PREC.CALL + 4, seq(
      field('name', $.call_syntax_macro_identifier),
      field('arguments', $.macro_argument_list),
      optional(field('declarator', $.parameter_list)),
      field('body', $.compound_statement),
      optional(';'),
    )),

    macro_enum_declaration: $ => seq(
      alias($.macro_enum_specifier, $.enum_specifier),
      ';',
    ),

    macro_enum_specifier: $ => prec.right(seq(
      'enum',
      optional(choice('class', 'struct')),
      choice(
        seq(
          field('name', $._class_name),
          optional($._enum_base_clause),
          optional(field('body', alias($.macro_enumerator_list, $.enumerator_list))),
        ),
        field('body', alias($.macro_enumerator_list, $.enumerator_list)),
      ),
      optional($.attribute_specifier),
    )),

    macro_enumerator_list: $ => seq(
      '{',
      commaSep(choice($.enumerator, $.expression)),
      optional(','),
      '}',
    ),

    preproc_disabled_block: _ => token(prec(
      1,
      /#[ \t]*if[ \t]+0[^\n]*\r?\n(?:[ \t]*(?:[^#\r\n][^\r\n]*)?\r?\n)*#[ \t]*endif(?:[^\r\n]*)?/,
    )),

    preproc_unbalanced_else_block: _ => token(prec(
      1,
      /#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\n(?:[^\r\n]*\r?\n)*?#[ \t]*endif[^\r\n]*(?:\r?\n[ \t]*)*else[ \t]*\{(?:[^\r\n]*\r?\n)*?[ \t]*\}/,
    )),

    ...preprocIf('', $ => $._block_item),
    ...preprocIf('_in_top_level', $ => $._top_level_item),
    ...preprocIf('_in_field_declaration_list', $ => $._field_declaration_list_item, 2),
    ...preprocIf(
      '_in_enumerator_list',
      $ => seq($.enumerator, ','),
      0,
      PREPROC_IFDEF | PREPROC_ELSE,
      false,
    ),
    ...preprocIf('_in_parameter_list', $ => {
      return seq($._parameter_list_item, optional(','));
    }, 3, PREPROC_IFDEF, false),

    ...preprocIf('_in_template_parameter_list', $ => {
      return seq(optional(','), $._preproc_template_parameter_list_item, optional(','));
    }, -1, PREPROC_IFDEF, false),

    ...preprocIf('_in_initializer_list', $ => {
      const item = choice($.initializer_pair, $.expression, $.initializer_list);
      return prec.right(1, seq(repeat(seq(item, ',')), item, optional(',')));
    }, 0, PREPROC_IFDEF | PREPROC_ELSE, false),

    _preproc_template_parameter_list_item: $ => choice(
      $.preproc_template_type_parameter_item,
      $.parameter_declaration,
      $.optional_parameter_declaration,
      $.variadic_parameter_declaration,
      $.variadic_type_parameter_declaration,
      $.optional_type_parameter_declaration,
      $.template_template_parameter_declaration,
    ),

    preproc_template_type_parameter_item: $ => choice(
      prec(4, seq(
        choice('typename', 'class'),
        $._type_identifier,
        ',',
      )),
      prec(3, seq(
        choice('typename', 'class'),
        $._type_identifier,
      )),
    ),

    ...preprocIf(
      '_in_expression_list',
      $ => seq(choice($.expression, $.initializer_list), optional(',')),
      2,
      PREPROC_IFDEF | PREPROC_ELSE,
      false,
    ),

    ...preprocIf(
      '_in_field_initializer_list',
      $ => seq($.field_initializer, optional(',')),
      2,
      PREPROC_IFDEF | PREPROC_ELSE,
      false,
    ),
    ...preprocIf(
      '_in_field_initializer_list_leading_comma',
      $ => seq(',', $.field_initializer, optional(',')),
      3,
      0,
      false,
    ),
    macro_template_declaration: $ => seq(
      'template',
      field('parameters', $.template_parameter_list),
      optional($.requires_clause),
      choice(
        $._empty_declaration,
        $.alias_declaration,
        $.declaration,
        $.template_declaration,
        $.function_definition,
        $.concept_definition,
        $.friend_declaration,
        $.class_specifier,
        $.struct_specifier,
        alias($.constructor_or_destructor_declaration, $.declaration),
        alias($.constructor_or_destructor_definition, $.function_definition),
        alias($.operator_cast_declaration, $.declaration),
        alias($.operator_cast_definition, $.function_definition),
      ),
    ),

    preproc_using: _ => token(prec(1, /#[ \t]*using[^\n]*/)),

    conditional_extern_c_open: _ => token(prec(1, /#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\nextern[ \t]+"C"[ \t]*\{\r?\n#[ \t]*endif/)),

    conditional_extern_c_close: _ => token(prec(
      1,
      /#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\n\}[ \t]*(?:(?:\/\/[^\r\n]*)|(?:\/\*[^*]*\*+(?:[^/*][^*]*\*+)*\/))?[ \t]*\r?\n#[ \t]*endif/,
    )),

    _preproc_expression: ($, original) => choice(
      original,
      $.preproc_scoped_identifier,
      $.string_literal,
      $.raw_string_literal,
      $.system_lib_string,
    ),

    preproc_scoped_identifier: $ => seq(
      $.identifier,
      repeat1(seq('::', $.identifier)),
    ),

    type_descriptor: $ => prec.right(seq(
      repeat($.type_qualifier),
      field('type', $.type_specifier),
      repeat($.type_qualifier),
      repeat($.post_type_macro_annotation),
      field('declarator', optional($._abstract_declarator)),
    )),

    // When used in a trailing return type, these specifiers can now occur immediately before
    // a compound statement. This introduces a shift/reduce conflict that needs to be resolved
    // with an associativity.
    _class_declaration: $ => seq(
      repeat(choice($.attribute_specifier, $.alignas_qualifier, $.attribute_declaration, $.function_prefix_macro)),
      optional($.ms_declspec_modifier),
      $._class_declaration_item,
    ),
    _class_declaration_item: $ => prec.right(seq(
      choice(
        field('name', $._class_name),
        seq(
          optional(field('name', $._class_name)),
          repeat($.virtual_specifier),
          optional($.base_class_clause),
          field('body', $.field_declaration_list),
        ),
      ),
      optional($.attribute_specifier),
    )),

    class_specifier: $ => seq(
      optional('ref'),
      'class',
      $._class_declaration,
    ),

    union_specifier: $ => seq(
      'union',
      $._class_declaration,
    ),

    struct_specifier: $ => seq(
      'struct',
      $._class_declaration,
    ),

    _class_name: $ => prec.right(choice(
      $._type_identifier,
      $.template_type,
      alias($.qualified_type_identifier, $.qualified_identifier),
    )),

    function_definition: $ => prec(1, seq(
      optional($.ms_call_modifier),
      $._declaration_specifiers,
      optional($.ms_call_modifier),
      field('declarator', $._declarator),
      field('body', choice($.compound_statement, $.try_statement, $.delete_method_clause)),
    )),

    _macro_function_definition_prefix: $ => prec.right(PREC.CALL + 7, seq(
      field('name', $.call_syntax_macro_identifier),
      field('arguments', $.macro_argument_list),
      '{',
    )),

    preproc_selected_macro_function_definition: $ => prec(1, seq(
      $.preproc_if_in_macro_function_definition_prefix,
      repeat($._block_item),
      '}',
    )),

    preproc_if_in_macro_function_definition_prefix: $ => prec(PREC.CALL + 7, seq(
      preprocessor('if'),
      field('condition', $._preproc_expression),
      $._preproc_directive_end,
      $._macro_function_definition_prefix,
      field('alternative', optional($.preproc_macro_function_definition_prefix_alternative)),
      preprocessor('endif'),
    )),

    preproc_macro_function_definition_prefix_alternative: $ =>
      $.preproc_else_in_macro_function_definition_prefix,

    preproc_else_in_macro_function_definition_prefix: $ => prec(PREC.CALL + 7, seq(
      preprocessor('else'),
      $._preproc_directive_end,
      $._macro_function_definition_prefix,
    )),

    qualified_type_function_definition: $ => prec(PREC.CALL + 2, seq(
      field('type', $._qualified_declaration_type),
      repeat($.post_type_macro_annotation),
      field('declarator', $._qualified_type_function_declarator),
      field('body', choice($.compound_statement, $.try_statement, $.delete_method_clause)),
    )),

    _qualified_type_function_declarator: $ => choice(
      $.function_declarator,
      alias($.qualified_type_pointer_function_declarator, $.pointer_declarator),
      alias($.qualified_type_reference_function_declarator, $.reference_declarator),
    ),

    qualified_type_pointer_function_declarator: $ => prec.dynamic(1, prec.right(seq(
      optional($.ms_based_modifier),
      '*',
      repeat(choice($.ms_pointer_modifier, $.type_qualifier, $.ms_call_modifier)),
      field('declarator', $._qualified_type_function_declarator),
    ))),

    qualified_type_reference_function_declarator: $ => prec.dynamic(1, prec.right(seq(
      choice('&', '&&', '%'),
      field('declarator', $._qualified_type_function_declarator),
    ))),

    declaration: $ => choice(
      seq(
        field('type', $._qualified_declaration_type),
        $._declaration_declarator_list,
        optional($.declaration_suffix_preproc_ifdef),
        ';',
      ),
      seq(
        $._declaration_specifiers,
        $._declaration_declarator_list,
        optional($.declaration_suffix_preproc_ifdef),
        ';',
      ),
    ),

    _qualified_declaration_type: $ => prec(1, choice(
      alias($.qualified_type_identifier, $.qualified_identifier),
      $.qualified_identifier,
    )),

    _type_definition_declarators: $ => commaSep1(field('declarator', choice(
      alias($.function_type_definition_declarator, $.function_declarator),
      $._type_declarator,
    ))),

    function_type_definition_declarator: $ => prec(1, seq(
      field('declarator', $._type_declarator),
      $._function_declarator_seq,
    )),

    _declaration_declarator_list: $ => commaSep1(field('declarator', choice(
      seq(
        // C uses _declaration_declarator here for some nice macro parsing in function declarators,
        // but this causes a world of pain for C++ so we'll just stick to the normal _declarator here.
        repeat($.post_type_macro_annotation),
        optional($.ms_call_modifier),
        $._declarator,
        optional($.gnu_asm_expression),
      ),
      $.init_declarator,
      prec.dynamic(1, seq(repeat1($.post_type_macro_annotation), $.init_declarator)),
    ))),

    virtual_specifier: _ => choice(
      'final', // the only legal value here for classes
      'override', // legal for functions in addition to final, plus permutations.
      'sealed',
      'abstract',
    ),

    _declaration_modifiers: ($, original) => choice(
      original,
      $.function_prefix_macro,
      'virtual',
    ),

    macro_prefix_identifier_fallback: _ => token(prec(
      1,
      /GTEST_(?:API_|ATTRIBUTE_NO_SANITIZE_ADDRESS_|INTERNAL_EMPTY_BASE_CLASS|NO_TAIL_CALL_)|USERVER_IMPL_FORCE_INLINE/,
    )),

    function_prefix_macro: $ => prec.right(PREC.CALL + 6, choice(
      $.declaration_prefix_macro_identifier,
      seq($.declaration_prefix_macro_identifier, $.macro_argument_list),
      $.macro_prefix_identifier_fallback,
    )),

    ms_call_modifier: ($, original) => choice(
      original,
      $.calling_convention_macro,
    ),

    calling_convention_macro: $ => prec(1, $.bare_macro_identifier),

    explicit_function_specifier: $ => choice(
      'explicit',
      prec(PREC.CALL, seq(
        'explicit',
        '(',
        $.expression,
        ')',
      )),
    ),

    base_class_clause: $ => seq(
      ':',
      commaSep1(seq(
        repeat($.attribute_declaration),
        optional(choice(
          $.access_specifier,
          seq($.access_specifier, optional('virtual')),
          seq('virtual', optional($.access_specifier)),
        )),
        choice($._class_name, $.decltype),
        optional('...'),
      )),
    ),

    enum_specifier: $ => prec.right(seq(
      'enum',
      optional(choice('class', 'struct')),
      repeat($.attribute_declaration),
      choice(
        seq(
          field('name', $._class_name),
          optional($._enum_base_clause),
          optional(field('body', $.enumerator_list)),
        ),
        field('body', $.enumerator_list),
      ),
      optional($.attribute_specifier),
    )),

    enumerator_list: $ => seq(
      '{',
      repeat(choice(
        seq(choice($.enumerator, $.macro_call_item), ','),
        alias($.preproc_if_in_enumerator_list, $.preproc_if),
        alias($.preproc_ifdef_in_enumerator_list, $.preproc_ifdef),
        seq($.preproc_call, ','),
      )),
      optional(choice(
        $.enumerator,
        $.macro_call_item,
        $.preproc_call,
      )),
      '}',
    ),

    _enum_base_clause: $ => prec.left(seq(
      ':',
      field('base', choice(
        alias($.qualified_type_identifier, $.qualified_identifier),
        $._type_identifier,
        $.primitive_type,
        $.sized_type_specifier,
      )),
    )),

    // The `auto` storage class is removed in C++0x in order to allow for the `auto` type.
    storage_class_specifier: (_, original) => choice(
      ...original.members.filter((member) => member.value !== 'auto'),
      'thread_local',
    ),

    dependent_type: $ => prec.dynamic(-1, prec.right(seq(
      'typename',
      $.type_specifier,
    ))),

    // Declarations

    template_declaration: $ => seq(
      'template',
      field('parameters', $.template_parameter_list),
      optional($.requires_clause),
      choice(
        prec.dynamic(10, alias($.constructor_or_destructor_definition, $.function_definition)),
        templateDeclarationItem($),
      ),
    ),

    _template_declaration_item: $ => choice(
      prec.dynamic(10, alias($.constructor_or_destructor_definition, $.function_definition)),
      templateDeclarationItem($),
    ),

    template_instantiation: $ => prec(1, seq(
      optional('extern'),
      'template',
      choice(
        choice($.class_specifier, $.struct_specifier, $.union_specifier),
        seq(
          optional($._declaration_specifiers),
          field('declarator', $._declarator),
        ),
      ),
      ';',
    )),

    declaration_suffix_preproc_ifdef: $ => prec(1, seq(
      choice(preprocessor('ifdef'), preprocessor('ifndef')),
      field('name', $.identifier),
      repeat1(choice(
        $.identifier,
        $.function_suffix_macro,
        $.attribute_specifier,
      )),
      preprocessor('endif'),
    )),

    _template_parameter_list_item: $ => choice(
      $.parameter_declaration,
      $.optional_parameter_declaration,
      $.type_parameter_declaration,
      $.variadic_parameter_declaration,
      $.variadic_type_parameter_declaration,
      $.optional_type_parameter_declaration,
      $.template_template_parameter_declaration,
    ),

    template_parameter_list: $ => seq(
      '<',
      commaSepWithLeadingPreproc(
        $,
        $._template_parameter_list_item,
        '_in_template_parameter_list',
        PREPROC_IFDEF,
      ),
      alias(token(prec(1, '>')), '>'),
    ),

    type_parameter_declaration: $ => choice(
      prec(2, seq(
        choice('typename', 'class'),
        $._type_identifier,
      )),
      prec(1, choice('typename', 'class')),
    ),

    variadic_type_parameter_declaration: $ => choice(
      prec(2, seq(
        choice('typename', 'class'),
        '...',
        $._type_identifier,
      )),
      prec(1, seq(
        choice('typename', 'class'),
        '...',
      )),
    ),

    optional_type_parameter_declaration: $ => seq(
      choice('typename', 'class'),
      optional(field('name', $._type_identifier)),
      '=',
      field('default_type', choice(
        $.member_function_pointer_type_descriptor,
        $.function_pointer_type_descriptor,
        $.type_descriptor,
      )),
    ),

    decltype_function_pointer_type_descriptor: $ => prec(PREC.CALL + 3, seq(
      $.decltype,
      field('declarator', $.abstract_parenthesized_declarator),
      $._function_declarator_seq,
    )),

    function_pointer_type_descriptor: $ => prec(PREC.CALL + 2, seq(
      $._declaration_specifiers,
      $.abstract_function_declarator,
    )),

    member_function_pointer_type_descriptor: $ => prec(PREC.CALL + 3, seq(
      field('return_type', $._class_name),
      '(',
      field('class', $._class_name),
      '::*',
      ')',
      field('parameters', $.parameter_list),
    )),

    template_template_parameter_declaration: $ => seq(
      'template',
      field('parameters', $.template_parameter_list),
      choice(
        $.type_parameter_declaration,
        $.variadic_type_parameter_declaration,
        $.optional_type_parameter_declaration,
      ),
    ),

    _parameter_list_item: $ => choice(
      $.parameter_declaration,
      $.optional_parameter_declaration,
      $.variadic_parameter_declaration,
      '...',
    ),

    parameter_list: $ => seq(
      '(',
      commaSepWithPreproc($, $._parameter_list_item, '_in_parameter_list', PREPROC_IFDEF),
      ')',
    ),

    parameter_declaration: ($, original) => choice(
      original,
      seq(
        $._declaration_specifiers,
        field('declarator', $._abstract_declarator),
      ),
    ),

    optional_parameter_declaration: $ => seq(
      $._declaration_specifiers,
      field('declarator', optional(choice($._declarator, $._abstract_declarator))),
      '=',
      field('default_value', choice($.expression, $.initializer_list)),
    ),

    variadic_parameter_declaration: $ => seq(
      $._declaration_specifiers,
      field('declarator', choice(
        $.variadic_declarator,
        alias($.variadic_reference_declarator, $.reference_declarator),
      )),
    ),

    variadic_declarator: $ => seq(
      prec(1, '...'),
      optional($.identifier),
    ),

    variadic_reference_declarator: $ => seq(
      choice('&&', '&'),
      $.variadic_declarator,
    ),

    init_declarator: ($, original) => choice(
      $.reference_argument_init_declarator,
      original,
      seq(
        field('declarator', $._declarator),
        repeat1($.attribute_specifier),
        '=',
        field('value', choice($.initializer_list, $.expression)),
      ),
      prec.dynamic(2, seq(
        field('declarator', $._declarator),
        field('value', $.primitive_braced_argument_list),
      )),
      prec.dynamic(1, seq(
        field('declarator', $._declarator),
        field('value', $.braced_argument_list),
      )),
      seq(
        field('declarator', $._declarator),
        field('value', choice(
          $.argument_list,
          $.initializer_list,
        )),
      ),
      prec(PREC.CALL + 2, seq(
        field('declarator', $.identifier),
        field('value', $.macro_initializer),
      )),
    ),

    macro_initializer: $ => choice(
      $.bare_macro_identifier,
      $.macro_initializer_fallback,
    ),

    macro_initializer_fallback: _ => token(prec(
      1,
      /ATOMIC_FLAG_INIT|FORMAT_USERVER_ATOMIC_INIT/,
    )),

    reference_argument_init_declarator: $ => prec(1, seq(
      field('declarator', alias($.reference_argument_declarator, $.reference_declarator)),
      field('value', $.argument_list),
    )),

    reference_argument_declarator: $ => seq(
      choice('&', '&&', '%'),
      field('declarator', choice(
        $.identifier,
        $.qualified_identifier,
        $.template_function,
        $.operator_name,
        $.destructor_name,
      )),
    ),

    preproc_value_declaration: $ => prec(1, seq(
      $._declaration_specifiers,
      field('declarator', $._declarator),
      '=',
      field('value', $.preproc_semicolon_initializer),
    )),

    preproc_semicolon_initializer: _ => token(prec(
      1,
      PREPROC_SEMICOLON_RHS,
    )),

    operator_cast: $ => prec.right(1, seq(
      'operator',
      $._declaration_specifiers,
      field('declarator', $._abstract_declarator),
    )),

    // Avoid ambiguity between compound statement and initializer list in a construct like:
    //   A b {};
    compound_statement: (_, original) => prec(-1, original),

    field_initializer_list: $ => {
      const preprocItem = preprocListItem($, '_in_field_initializer_list', PREPROC_IFDEF);
      const leadingPreprocItem = preprocListItem($, '_in_field_initializer_list_leading_comma', 0);
      const tailItem = choice(
        seq(',', $.field_initializer),
        seq(preprocItem, optional($.field_initializer)),
        leadingPreprocItem,
        seq(
          ',',
          preprocListItem($, '_in_field_initializer_list', PREPROC_IFDEF),
          optional($.field_initializer),
        ),
      );
      return seq(
        ':',
        choice(
          $.field_initializer,
          seq(preprocItem, optional($.field_initializer)),
        ),
        repeat(tailItem),
      );
    },

    field_initializer: $ => prec(1, seq(
      repeat($.field_initializer_prefix_macro),
      choice(
        $._field_identifier,
        $.template_method,
        alias($.qualified_field_identifier, $.qualified_identifier),
      ),
      choice($.initializer_list, $.argument_list),
      optional('...'),
    )),

    field_initializer_prefix_macro: $ => $.macro_call_item,

    _field_declaration_list_item: ($, original) => choice(
      $.access_specifier_label,
      alias($.qualified_type_function_definition, $.function_definition),
      $.standalone_attribute_preproc_if,
      $.standalone_qualifier_preproc_if,
      alias($.preproc_if_in_field_declaration_list, $.preproc_if),
      alias($.preproc_ifdef_in_field_declaration_list, $.preproc_ifdef),
      $.macro_prefixed_field_declaration_item,
      $.top_level_decorator_macro,
      $.top_level_macro_run_item,
      alias($.inline_method_definition, $.function_definition),
      alias($.constructor_or_destructor_definition, $.function_definition),
      alias($.constructor_or_destructor_declaration, $.declaration),
      $.class_macro_call_item,
      $.class_bare_macro_item,
      $.macro_method_declaration,
      alias($.qualified_macro_initialized_field_declaration, $.field_declaration),
      $.static_assert_declaration,
      original,
      $.deleted_operator_cast_declaration,
      $.attributed_friend_operator_declaration,
      $.using_operator_pack_declaration,
      $.template_declaration,
      alias($.operator_cast_definition, $.function_definition),
      alias($.operator_cast_declaration, $.declaration),
      $.friend_declaration,
      $.function_pointer_alias_declaration,
      $.function_type_alias_declaration,
      $.alias_declaration,
      $.using_declaration,
      $.type_definition,
      ';',
    ),

    standalone_qualifier_preproc_if: _ => token(prec(
      1,
      /#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\n(?:[ \t]*\/\/[^\n]*\r?\n)*[ \t]*(?:const|constexpr|consteval|static|extern|inline|__inline__?|__forceinline|[A-Za-z_]\w*(?:\([^()\r\n]*\))?)[ \t]*(?:\/\/[^\n]*)?\r?\n(?:#[ \t]*(?:elif|elifdef|elifndef|else)[^\n]*\r?\n(?:[ \t]*\/\/[^\n]*\r?\n)*[ \t]*(?:const|constexpr|consteval|static|extern|inline|__inline__?|__forceinline|[A-Za-z_]\w*(?:\([^()\r\n]*\))?)[ \t]*(?:\/\/[^\n]*)?\r?\n)*#[ \t]*endif/,
    )),

    standalone_attribute_preproc_if: _ => token(prec(
      1,
      /#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\n(?:[ \t]*\/\/[^\n]*\r?\n)*[ \t]*(?:\[\[[^\n]*\]\][ \t]*)+\r?\n(?:#[ \t]*else[^\n]*\r?\n(?:[ \t]*\/\/[^\n]*\r?\n)*[ \t]*(?:\[\[[^\n]*\]\][ \t]*)+\r?\n)?#[ \t]*endif/,
    )),

    deleted_operator_cast_declaration: _ => token(prec(
      1,
      /(?:(?:constexpr|consteval)[ \t]+)?(?:\/\*[^*]*\*\/[ \t]*)?operator[ \t]+[A-Za-z_:][A-Za-z0-9_:<>]*\(\)[^\n;]*=[ \t]*delete;/,
    )),

    attributed_friend_operator_declaration: _ => token(prec(
      1,
      /\[\[[^\]\n]+\]\][ \t]+friend[ \t]+bool[ \t]+operator(?:==|!=)\([^\n]*\)[ \t]*\{[^\n]*\}/,
    )),

    using_operator_pack_declaration: _ => token(prec(1, /using[ \t]+[A-Za-z_]\w*::operator\(\)\.\.\.;/)),

    macro_method_declaration: $ => seq(
      field('function', $.call_syntax_macro_identifier),
      '(',
      field('return_type', $.macro_method_return_type),
      ',',
      field('name', $.identifier),
      ',',
      field('parameters', $.macro_method_parameter_list),
      optional(seq(
        ',',
        field('qualifiers', $.macro_method_qualifier_list),
      )),
      ')',
      ';',
    ),

    macro_method_return_type: $ => prec(1, choice(
      $.macro_function_pointer_type_descriptor,
      $.function_pointer_type_descriptor,
      $.macro_parenthesized_argument,
      $._declaration_specifiers,
    )),

    macro_method_parameter_list: _ => token(prec(1, /\((?:[^()\r\n]|\r?\n[ \t]*|\([^()\r\n]*\))*\)/)),

    macro_method_qualifier_list: $ => seq(
      '(',
      commaSep(choice(
        $.macro_method_call_qualifier,
        $.type_qualifier,
        $.virtual_specifier,
        $.noexcept,
        $.identifier,
      )),
      ')',
    ),

    macro_method_call_qualifier: _ => token(prec(1, /[A-Za-z_]\w*\([^()\r\n]*\)/)),

    qualified_macro_initialized_field_declaration: _ => token(prec(
      2,
      /[A-Za-z_]\w*(?:::[A-Za-z_]\w*)+[ \t]+[A-Za-z_]\w*[ \t]+[A-Z_]\w*[ \t]*;/,
    )),

    field_declaration: $ => choice(
      prec(PREC.CALL + 2, seq(
        field('type', $._qualified_declaration_type),
        optional($.ms_call_modifier),
        $._field_declaration_declarator_list,
        optional($.attribute_specifier),
        ';',
      )),
      seq(
        $._declaration_specifiers,
        optional($.ms_call_modifier),
        optional($._field_declaration_declarator_list),
        optional($.attribute_specifier),
        ';',
      ),
    ),

    _field_declaration_declarator_list: $ => commaSep1(seq(
      repeat($.post_type_macro_annotation),
      field('declarator', $._field_declarator),
      optional(choice(
        $.bitfield_clause,
        field('default_value', $.initializer_list),
        seq('=', field('default_value', choice($.expression, $.initializer_list))),
      )),
    )),

    inline_method_definition: $ => seq(
      $._declaration_specifiers,
      optional($.ms_call_modifier),
      field('declarator', $._field_declarator),
      choice(
        field('body', choice($.compound_statement, $.try_statement)),
        $.default_method_clause,
        $.delete_method_clause,
        $.pure_virtual_clause,
      ),
    ),

    _constructor_specifiers: $ => choice(
      $._declaration_modifiers,
      $.explicit_function_specifier,
    ),

    _constructor_or_destructor_header: $ => seq(
      repeat($._constructor_specifiers),
      field('declarator', choice(
        alias($.qualified_constructor_or_destructor_declarator, $.function_declarator),
        alias($.qualified_function_declarator, $.function_declarator),
        $.function_declarator,
      )),
    ),

    operator_cast_definition: $ => seq(
      repeat($._constructor_specifiers),
      field('declarator', choice(
        $.operator_cast,
        alias($.qualified_operator_cast_identifier, $.qualified_identifier),
      )),
      field('body', choice($.compound_statement, $.try_statement)),
    ),

    operator_cast_declaration: $ => prec(1, seq(
      repeat($._constructor_specifiers),
      field('declarator', choice(
        $.operator_cast,
        alias($.qualified_operator_cast_identifier, $.qualified_identifier),
      )),
      optional(seq('=', field('default_value', $.expression))),
      ';',
    )),

    constructor_try_statement: $ => seq(
      'try',
      optional($.field_initializer_list),
      field('body', $.compound_statement),
      repeat1($.catch_clause),
    ),

    constructor_or_destructor_definition: $ => prec.dynamic(2, prec(PREC.CALL + 2, seq(
      $._constructor_or_destructor_header,
      constructorOrDestructorBody($),
    ))),

    constructor_or_destructor_declaration: $ => prec.dynamic(2, seq(
      $._constructor_or_destructor_header,
      ';',
    )),

    default_method_clause: _ => seq('=', 'default', ';'),
    delete_method_clause: _ => seq('=', 'delete', ';'),
    pure_virtual_clause: _ => seq('=', /0/, ';'),

    friend_declaration: $ => seq(
      optional('constexpr'),
      'friend',
      choice(
        alias($.qualified_type_function_definition, $.function_definition),
        $.declaration,
        $.function_definition,
        seq(
          optional(choice(
            'class',
            'struct',
            'union',
          )),
          $._class_name, ';',
        ),
      ),
    ),

    access_specifier: _ => choice(
      'public',
      'private',
      'protected',
    ),

    access_specifier_label: _ => prec(1, seq(
      choice(
        'public',
        'private',
        'protected',
      ),
      ':',
    )),

    _declarator: ($, original) => choice(
      original,
      $.reference_declarator,
      $.handle_declarator,
      $.member_pointer_declarator,
      $.qualified_identifier,
      $.template_function,
      $.operator_name,
      $.destructor_name,
      $.structured_binding_declarator,
    ),

    _field_declarator: ($, original) => choice(
      original,
      alias($.reference_field_declarator, $.reference_declarator),
      alias($.handle_field_declarator, $.handle_declarator),
      alias($.member_pointer_field_declarator, $.member_pointer_declarator),
      $.template_method,
      $.operator_name,
    ),

    _type_declarator: ($, original) => choice(
      original,
      alias($.reference_type_declarator, $.reference_declarator),
      alias($.handle_type_declarator, $.handle_declarator),
      alias($.member_pointer_type_declarator, $.member_pointer_declarator),
    ),

    _abstract_declarator: ($, original) => choice(
      original,
      $.abstract_reference_declarator,
      $.abstract_handle_declarator,
      $.abstract_member_pointer_declarator,
    ),

    post_type_macro_annotation: $ => prec.right(PREC.CALL + 7, choice(
      seq($.bare_macro_identifier, $.macro_argument_list),
      seq($.post_type_macro_annotation_fallback, $.macro_argument_list),
      $.bare_macro_identifier,
      $.post_type_macro_annotation_fallback,
    )),

    post_type_macro_annotation_fallback: _ => token(prec(
      PREC.CALL + 8,
      /__(?:[A-Za-z0-9]+_)?percpu|ALIGN|WINAPI|CALLBACK/,
    )),

    reference_declarator: $ => prec.dynamic(1, prec.right(seq(choice('&', '&&', '%'), $._declarator))),
    reference_field_declarator: $ => prec.dynamic(1, prec.right(seq(choice('&', '&&', '%'), $._field_declarator))),
    reference_type_declarator: $ => prec.dynamic(1, prec.right(seq(choice('&', '&&', '%'), $._type_declarator))),
    abstract_reference_declarator: $ => prec.right(seq(choice('&', '&&', '%'), optional($._abstract_declarator))),

    pointer_declarator: $ => prec.dynamic(1, prec.right(seq(
      optional($.ms_based_modifier),
      '*',
      repeat(choice($.ms_pointer_modifier, $.type_qualifier, $.ms_call_modifier)),
      field('declarator', $._declarator),
    ))),
    pointer_field_declarator: $ => prec.dynamic(1, prec.right(seq(
      optional($.ms_based_modifier),
      '*',
      repeat(choice($.ms_pointer_modifier, $.type_qualifier, $.ms_call_modifier)),
      field('declarator', $._field_declarator),
    ))),
    pointer_type_declarator: $ => prec.dynamic(1, prec.right(seq(
      optional($.ms_based_modifier),
      '*',
      repeat(choice($.ms_pointer_modifier, $.type_qualifier, $.ms_call_modifier)),
      field('declarator', $._type_declarator),
    ))),
    abstract_pointer_declarator: $ => prec.dynamic(1, prec.right(seq(
      '*',
      repeat(choice($.ms_pointer_modifier, $.type_qualifier, $.ms_call_modifier)),
      field('declarator', optional($._abstract_declarator)),
    ))),

    handle_declarator: $ => prec.dynamic(1, prec.right(seq('^', $._declarator))),
    handle_field_declarator: $ => prec.dynamic(1, prec.right(seq('^', $._field_declarator))),
    handle_type_declarator: $ => prec.dynamic(1, prec.right(seq('^', $._type_declarator))),
    abstract_handle_declarator: $ => prec.right(seq('^', optional($._abstract_declarator))),

    abstract_member_pointer_declarator: $ => prec.dynamic(1, prec.right(seq(
      field('scope', $._scope_resolution),
      '*',
      field('declarator', optional($._abstract_declarator)),
    ))),

    abstract_parenthesized_declarator: $ => prec(1, seq(
      '(',
      optional($.ms_call_modifier),
      $._abstract_declarator,
      ')',
    )),

    member_pointer_declarator: $ => prec.dynamic(1, prec.right(seq(
      field('scope', $._scope_resolution),
      '*',
      field('declarator', $._declarator),
    ))),
    member_pointer_field_declarator: $ => prec.dynamic(1, prec.right(seq(
      field('scope', $._scope_resolution),
      '*',
      field('declarator', $._field_declarator),
    ))),
    member_pointer_type_declarator: $ => prec.dynamic(1, prec.right(seq(
      field('scope', $._scope_resolution),
      '*',
      field('declarator', $._type_declarator),
    ))),

    structured_binding_declarator: $ => prec.dynamic(PREC.STRUCTURED_BINDING, seq(
      '[', commaSep1(choice($.identifier, $.structured_binding_pack_identifier)), ']',
    )),

    structured_binding_pack_identifier: $ => seq(
      '...',
      $.identifier,
    ),

    ref_qualifier: _ => choice('&', '&&'),

    _function_declarator_seq: $ => seq(
      field('parameters', $.parameter_list),
      optional($._function_attributes_start),
      optional($.ref_qualifier),
      optional($._function_exception_specification),
      optional($._function_attributes_end),
      optional($.trailing_return_type),
      optional($._function_postfix),
      optional($._function_attributes_end),
    ),

    _function_attributes_start: $ => prec(1, choice(
      seq(repeat1($.attribute_specifier), repeat($.type_qualifier)),
      seq(repeat($.attribute_specifier), repeat1($.type_qualifier)),
      seq(repeat($.attribute_specifier), repeat1($.function_suffix_macro)),
    )),

    _function_exception_specification: $ => choice(
      $.noexcept,
      $.throw_specifier,
    ),

    _function_attributes_end: $ => prec.right(seq(
      optional($.gnu_asm_expression),
      choice(
        seq(repeat1($.attribute_specifier), repeat($.attribute_declaration)),
        seq(repeat($.attribute_specifier), repeat1($.attribute_declaration)),
        repeat1($.function_suffix_macro),
      ),
    )),

    function_suffix_macro: $ => prec.right(PREC.CALL + 7, choice(
      $.bare_macro_identifier,
      $.function_suffix_macro_identifier_fallback,
      seq($.bare_macro_identifier, $.macro_argument_list),
      seq($.function_suffix_macro_identifier_fallback, $.macro_argument_list),
    )),

    function_suffix_macro_identifier_fallback: _ => token(prec(
      PREC.CALL + 8,
      /GTEST_(?:NO_TAIL_CALL_|LOCK_EXCLUDED_|EXCLUSIVE_LOCK_REQUIRED_)|USERVER_(?:IMPL_LIFETIME_BOUND|FMT_CONST|IMPL_LIBC_INCLUDE_FIXES_THROW)|RAPIDJSON_NOEXCEPT|FORMAT_USERVER_(?:LIFETIME_BOUND|THROW)/,
    )),

    _function_postfix: $ => prec.right(choice(
      repeat1($.virtual_specifier),
      $.requires_clause,
    )),

    function_declarator: $ => prec.dynamic(1, seq(
      field('declarator', $._declarator),
      $._function_declarator_seq,
    )),

    qualified_function_declarator: $ => prec.dynamic(2, prec(3, seq(
      field('declarator', $.qualified_identifier),
      $._function_declarator_seq,
    ))),

    qualified_constructor_or_destructor_declarator: $ => prec.dynamic(3, prec(4, seq(
      field('declarator', $.qualified_identifier),
      $._function_declarator_seq,
    ))),

    function_field_declarator: $ => prec.dynamic(1, seq(
      field('declarator', $._field_declarator),
      $._function_declarator_seq,
    )),

    abstract_function_declarator: $ => seq(
      field('declarator', optional($._abstract_declarator)),
      $._function_declarator_seq,
    ),

    trailing_return_type: $ => seq('->', $.type_descriptor),

    noexcept: $ => prec.right(seq(
      'noexcept',
      optional(
        seq(
          '(',
          optional($.expression),
          ')',
        ),
      ),
    )),

    throw_specifier: $ => seq(
      'throw',
      seq(
        '(',
        commaSep($.type_descriptor),
        ')',
      ),
    ),

    template_type: $ => seq(
      field('name', $._type_identifier),
      field('arguments', $.template_argument_list),
    ),

    template_method: $ => seq(
      field('name', choice($._field_identifier, $.operator_name)),
      field('arguments', $.template_argument_list),
    ),

    template_function: $ => seq(
      field('name', $.identifier),
      field('arguments', $.template_argument_list),
    ),

    template_argument_list: $ => seq(
      '<',
      repeat(choice(
        seq($._template_argument_list_item, ','),
        $._template_argument_list_fragment,
      )),
      optional($._template_argument_list_item),
      alias(token(prec(1, '>')), '>'),
    ),

    _template_argument_list_item: $ => choice(
      prec.dynamic(3, $.type_descriptor),
      prec.dynamic(2, alias($.type_parameter_pack_expansion, $.parameter_pack_expansion)),
      $._template_argument_expression,
    ),

    preproc_template_argument_fragment: _ => token(prec(
      1,
      /#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\n(?:[ \t]*[^#\r\n][^\n]*\r?\n)+(?:#[ \t]*else[^\n]*\r?\n(?:[ \t]*[^#\r\n][^\n]*\r?\n)+)?#[ \t]*endif(?:\r?\n#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\n(?:[ \t]*[^#\r\n][^\n]*\r?\n)+(?:#[ \t]*else[^\n]*\r?\n(?:[ \t]*[^#\r\n][^\n]*\r?\n)+)?#[ \t]*endif)*/,
    )),

    _template_argument_list_fragment: $ => choice(
      $.preproc_template_argument_fragment,
      $.macro_template_argument_fragment,
    ),

    macro_template_argument_fragment: $ => prec(1, $.bare_macro_identifier),

    _template_argument_expression: $ => choice(
      $._expression_not_binary,
      $._template_argument_binary_expression,
    ),

    _template_argument_binary_expression: $ => {
      const table = [
        ['+', PREC.ADD],
        ['-', PREC.ADD],
        ['*', PREC.MULTIPLY],
        ['/', PREC.MULTIPLY],
        ['%', PREC.MULTIPLY],
        ['||', PREC.LOGICAL_OR],
        ['&&', PREC.LOGICAL_AND],
        ['|', PREC.INCLUSIVE_OR],
        ['^', PREC.EXCLUSIVE_OR],
        ['&', PREC.BITWISE_AND],
        ['<=', PREC.RELATIONAL],
        ['>=', PREC.RELATIONAL],
        ['==', PREC.EQUAL],
        ['!=', PREC.EQUAL],
        ['or', PREC.LOGICAL_OR],
        ['and', PREC.LOGICAL_AND],
        ['bitor', PREC.INCLUSIVE_OR],
        ['xor', PREC.EXCLUSIVE_OR],
        ['bitand', PREC.BITWISE_AND],
        ['not_eq', PREC.EQUAL],
      ];

      return choice(...table.map(([operator, precedence]) => {
        const operatorToken = operator === '>=' ? alias(token(prec(2, '>=')), '>=') : operator;
        return prec.left(precedence, seq(
          field('left', $._template_argument_expression),
          // @ts-ignore
          field('operator', operatorToken),
          field('right', $._template_argument_expression),
        ));
      }));
    },

    namespace_definition: $ => prec(1, seq(
      optional('inline'),
      'namespace',
      optional($.attribute_declaration),
      field('name', optional(
        choice(
          $._namespace_identifier,
          $.nested_namespace_specifier,
        ))),
      field('body', $.namespace_declaration_list),
    )),

    preproc_guarded_namespace_definition: $ => prec.dynamic(1, prec(1, seq(
      preprocessor('if'),
      field('condition', $._preproc_expression),
      $._preproc_directive_end,
      'namespace',
      '{',
      repeat($._top_level_item),
      preprocessor('endif'),
      '}',
    ))),

    namespace_declaration_list: $ => seq(
      '{',
      repeat($._top_level_item),
      '}',
    ),

    namespace_alias_definition: $ => seq(
      'namespace',
      field('name', $._namespace_identifier),
      '=',
      choice(
        $._namespace_identifier,
        $.nested_namespace_specifier,
      ),
      ';',
    ),

    module_declaration: $ => seq(
      optional('export'),
      'module',
      optional(choice(
        $.module_name,
        $.module_partition,
        seq($.module_name, $.module_partition),
      )),
      ';',
    ),

    module_import_declaration: $ => seq(
      optional('export'),
      'import',
      choice(
        $.module_name,
        $.module_partition,
        seq($.module_name, $.module_partition),
        $.string_literal,
        $.system_lib_string,
      ),
      ';',
    ),

    module_name: $ => seq($.identifier, repeat(seq('.', $.identifier))),

    module_partition: $ => seq(':', $.module_name),

    _namespace_specifier: $ => seq(
      optional('inline'),
      $._namespace_identifier,
    ),

    nested_namespace_specifier: $ => prec(1, seq(
      optional($._namespace_specifier),
      '::',
      choice(
        $.nested_namespace_specifier,
        $._namespace_specifier,
      ),
    )),

    using_declaration: $ => seq(
      'using',
      optional(choice('namespace', 'enum', 'typename')),
      commaSep1(choice(
        $.identifier,
        $.qualified_identifier,
      )),
      ';',
    ),

    windows_int_function_pointer_alias_declaration: _ => token(prec(
      PREC.CALL + 8,
      /using[ \t]+[A-Za-z_]\w*[ \t]*=[ \t]*int[ \t]*\([ \t]*(?:WINAPI|CALLBACK|__cdecl)[ \t]*\*[ \t]*\)[ \t]*\([^;\r\n]*\)[ \t]*;/,
    )),

    function_pointer_alias_declaration: $ => choice(
      $.windows_int_function_pointer_alias_declaration,
      prec(1, seq(
        'using',
        field('name', $._type_identifier),
        '=',
        field('return_type', choice(
          $.qualified_type_identifier,
          $.qualified_identifier,
          $.template_type,
          $._class_name,
        )),
        '(',
        choice(
          seq(
            field('class', choice($.qualified_type_identifier, $.qualified_identifier, $._class_name)),
            '::*',
          ),
          seq(optional($.ms_call_modifier), '*'),
        ),
        ')',
        $._function_declarator_seq,
        ';',
      )),
    ),

    function_type_alias_declaration: $ => prec(2, seq(
      'using',
      field('name', $._type_identifier),
      '=',
      field('return_type', choice($.type_descriptor, $.qualified_type_identifier, $.qualified_identifier)),
      $._function_declarator_seq,
      ';',
    )),

    deduction_guide_declaration: $ => prec(1, seq(
      field('name', $.identifier),
      field('parameters', $.parameter_list),
      field('return_type', $.trailing_return_type),
      ';',
    )),

    deleted_operator_declaration: _ => token(prec(
      1,
      /(?:template[ \t]*<[^;]*>\r?\n)?void[ \t]+operator(?:==|!=)[ \t]*\([^;]*\)[ \t]*=[ \t]*delete;/,
    )),

    alias_declaration: $ => seq(
      'using',
      field('name', $._type_identifier),
      repeat($.attribute_declaration),
      optional($.alias_suffix_macro),
      '=',
      choice(
        seq(field('type', $.type_descriptor), ';'),
        field('type', $.preproc_semicolon_initializer),
      ),
    ),

    alias_suffix_macro: $ => prec(PREC.CALL + 7, choice(
      $.bare_macro_identifier,
      $.reserved_alias_suffix_macro_identifier,
    )),

    reserved_alias_suffix_macro_identifier: _ => token(prec(
      PREC.CALL + 8,
      /USERVER_IMPL_NODEBUG|FORMAT_USERVER_NODEBUG_FUNC/,
    )),

    static_assert_declaration: $ => seq(
      'static_assert',
      '(',
      field('condition', $.expression),
      optional(seq(
        ',',
        field('message', $._string),
      )),
      ')',
      ';',
    ),

    concept_definition: $ => seq(
      'concept',
      field('name', $.identifier),
      '=',
      choice(
        seq($.expression, ';'),
        $.preproc_semicolon_initializer,
      ),
    ),

    // Statements

    _top_level_statement: ($, original) => choice(
      $.macro_call_item,
      $.top_level_call_statement,
      $.throw_cast_statement,
      original,
      $.co_return_statement,
      $.co_yield_statement,
      $.for_each_statement,
      $.for_range_loop,
      $.try_statement,
      $.throw_statement,
    ),

    _top_level_expression_statement: ($, original) => choice(
      $.top_level_call_statement,
      original,
    ),

    _non_case_statement: ($, original) => choice(
      $.bare_macro_statement,
      alias($.macro_statement_argument_expression_statement, $.expression_statement),
      $.macro_call_item,
      $.top_level_call_statement,
      $.throw_cast_statement,
      $.preproc_case_label_fragment,
      $.preproc_assignment_statement,
      $.preproc_guarded_assignment_statement,
      $.preproc_if,
      $.preproc_ifdef,
      $.preproc_selected_else_if_statement,
      $.preproc_selected_braced_if_else_statement,
      $.preproc_selected_if_statement,
      original,
      $.co_return_statement,
      $.co_yield_statement,
      $.for_each_statement,
      $.for_range_loop,
      $.try_statement,
      $.throw_statement,
    ),

    bare_macro_statement: $ => prec(1, $.bare_macro_identifier),

    switch_statement: $ => seq(
      'switch',
      field('condition', $.condition_clause),
      field('body', choice($.compound_statement, $.case_statement)),
    ),

    preproc_case_label_fragment: _ => token(prec(
      1,
      /#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\n(?:[ \t]*(?:(?:case[^\r\n:]*:)|(?:default[ \t]*:))[ \t]*(?:\/\/[^\n]*)?\r?\n)+(?:#[ \t]*(?:elif|else)[^\n]*\r?\n(?:[ \t]*(?:(?:case[^\r\n:]*:)|(?:default[ \t]*:))[ \t]*(?:\/\/[^\n]*)?\r?\n)+)*#[ \t]*endif/,
    )),

    preproc_endif_fragment: _ => token(prec(1, /#[ \t]*endif/)),

    preproc_guarded_assignment_statement: _ => token(prec(
      1,
      /#[ \t]*if[ \t]+[^\n]*\r?\n(?:[^#\r\n][^\r\n]*\r?\n)*#[ \t]*endif[^\n]*\r?\n[ \t]*[A-Za-z_]\w*[ \t]*=[^\n;]*;/,
    )),

    preproc_assignment_statement: _ => token(prec(
      1,
      /[A-Za-z_][^=\n;]*=[ \t]*(?:\r?\n)?#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\n(?:[ \t]*[^# \t\r\n][^\r\n]*\r?\n)*[ \t]*[^# \t\r\n][^;\r\n]*;[^\r\n]*\r?\n(?:#[ \t]*(?:elif|else)[^\n]*\r?\n(?:[ \t]*[^# \t\r\n][^\r\n]*\r?\n)*[ \t]*[^# \t\r\n][^;\r\n]*;[^\r\n]*\r?\n)*#[ \t]*endif/,
    )),

    preproc_selected_if_statement: $ => prec.right(seq(
      field('condition', $.preproc_selected_if_header),
      field('consequence', $.statement),
      optional(field('alternative', $.else_clause)),
    )),

    preproc_selected_if_header: _ => token(prec(
      1,
      /#[ \t]*if[^\n]*\r?\n[ \t]*if[^\n]*\r?\n(?:#[ \t]*else[^\n]*\r?\n[ \t]*if[^\n]*\r?\n)?#[ \t]*endif/,
    )),

    preproc_selected_braced_if_else_statement: _ => token(prec(
      1,
      /#[ \t]*if[^\n]*\r?\n[ \t]*if[^\n]*\{\r?\n#[ \t]*else[^\n]*\r?\n[ \t]*if[^\n]*\{\r?\n#[ \t]*endif\r?\n(?:[ \t]*[^#{}\r\n][^\r\n]*\r?\n)*[ \t]*\}[ \t]*else[ \t]*\{\r?\n(?:[ \t]*[^#{}\r\n][^\r\n]*\r?\n)*[ \t]*\}/,
    )),

    preproc_selected_else_if_statement: $ => prec.right(2, seq(
      'if',
      optional('constexpr'),
      field('condition', $.condition_clause),
      '{',
      repeat($.preproc_selected_else_if_body_item),
      $.preproc_selected_else_if_clause,
    )),

    preproc_selected_else_if_body_item: $ => choice(
      $.declaration,
      $.expression_statement,
      $.return_statement,
      $.macro_call_item,
      $.top_level_call_statement,
    ),

    preproc_selected_else_if_clause: _ => token(prec(
      5,
      /#[ \t]*if[^\n]*\r?\n[ \t]*\}[ \t]*else[ \t]+if[ \t]*\([^{}\r\n]*\)[ \t]*\{\r?\n(?:[ \t]*(?:[^#{}\r\n \t][^\r\n]*)?\r?\n)*#[ \t]*endif[^\r\n]*\r?\n[ \t]*\}[ \t]*else[ \t]+if[ \t]*\([^{}\r\n]*\)[ \t]*\{\r?\n(?:[ \t]*(?:[^#{}\r\n \t][^\r\n]*)?\r?\n)*[ \t]*\}/,
    )),

    while_statement: $ => seq(
      'while',
      field('condition', $.condition_clause),
      field('body', $.statement),
    ),

    if_statement: $ => prec.right(seq(
      'if',
      optional('constexpr'),
      field('condition', $.condition_clause),
      field('consequence', $.statement),
      optional(field('alternative', $.else_clause)),
    )),

    // Using prec(1) instead of prec.dynamic(1) causes issues with the
    // range loop's declaration specifiers if `int` is passed in, it'll
    // always prefer the standard for loop and give us a parse error.
    _for_statement_body: ($, original) => prec.dynamic(1, original),
    for_range_loop: $ => seq(
      'for',
      '(',
      $._for_range_loop_body,
      ')',
      field('body', $.statement),
    ),
    _for_range_loop_body: $ => seq(
      field('initializer', optional($.init_statement)),
      choice(
        $._qualified_type_for_range_loop_body,
        seq(
          $._declaration_specifiers,
          field('declarator', $._declarator),
        ),
      ),
      ':',
      field('right', choice(
        $.expression,
        $.initializer_list,
      )),
    ),

    _qualified_type_for_range_loop_body: $ => prec(2, seq(
      field('type', choice(
        alias($.qualified_template_type_identifier, $.qualified_identifier),
        alias($.qualified_type_identifier, $.qualified_identifier),
      )),
      field('declarator', choice(
        $.identifier,
        $.pointer_declarator,
        $.reference_declarator,
      )),
    )),

    qualified_template_type_identifier: $ => prec(2, seq(
      $._scope_resolution,
      field('name', choice(
        $.template_type,
        alias($.qualified_template_type_identifier, $.qualified_identifier),
      )),
    )),

    for_each_statement: $ => seq(
      'for',
      'each',
      '(',
      $._declaration_specifiers,
      field('declarator', $._declarator),
      'in',
      field('right', $.expression),
      ')',
      field('body', $.statement),
    ),

    init_statement: $ => choice(
      $.alias_declaration,
      $.type_definition,
      $.declaration,
      $.expression_statement,
    ),

    _condition_init_statement: $ => choice(
      alias($.condition_init_declaration, $.declaration),
      $.expression_statement,
    ),

    condition_init_declaration: $ => choice(
      seq($.condition_declaration, ';'),
      seq(
        $._declaration_specifiers,
        field('declarator', $._declarator),
        ';',
      ),
    ),

    condition_clause: $ => choice(
      prec.dynamic(2, seq(
        '(',
        field('value', choice($.expression, $.comma_expression)),
        ')',
      )),
      seq(
        '(',
        field('value', choice(
          $.preproc_condition_expression,
          alias($.condition_declaration, $.declaration),
        )),
        ')',
      ),
      prec.dynamic(-1, seq(
        '(',
        field('initializer', alias($._condition_init_statement, $.init_statement)),
        field('value', choice(
          $.expression,
          $.comma_expression,
          alias($.condition_declaration, $.declaration),
        )),
        ')',
      )),
    ),

    preproc_condition_expression: _ => token(prec(
      1,
      /#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\n(?:[ \t]*[^#\r\n][^\n]*\r?\n)*(?:#[ \t]*else[^\n]*\r?\n(?:[ \t]*[^#\r\n][^\n]*\r?\n)*)?#[ \t]*endif/,
    )),

    condition_declaration: $ => prec.dynamic(-1, seq(
      $._declaration_specifiers,
      field('declarator', $._declarator),
      choice(
        seq(
          '=',
          field('value', $.expression),
        ),
        field('value', $.initializer_list),
      ),
    )),

    return_statement: ($, original) => seq(
      choice(
        original,
        seq('return', $.initializer_list, ';'),
      ),
    ),

    co_return_statement: $ => seq(
      'co_return',
      optional($.expression),
      ';',
    ),

    co_yield_statement: $ => seq(
      'co_yield',
      $.expression,
      ';',
    ),

    throw_expression: $ => prec.right(PREC.UNARY, seq(
      'throw',
      optional($.expression),
    )),

    throw_statement: $ => seq(
      'throw',
      optional(choice(
        alias($.throw_fold_expression, $.fold_expression),
        $.expression,
      )),
      ';',
    ),

    throw_cast_statement: _ => token(prec(
      2,
      /throw[ \t]+\([A-Za-z_:][A-Za-z0-9_:]*\)[ \t]*(?:[A-Za-z_]\w*|\d+)[ \t]*;/,
    )),

    throw_fold_expression: _ => token(prec(1, /\([^;\n]*<<[ \t]*\.\.\.[ \t]*<<[^;\n]*\)/)),

    try_statement: $ => prec.right(seq(
      'try',
      field('body', $.compound_statement),
      choice(
        repeat1($.catch_clause),
        seq(repeat($.catch_clause), $.finally_clause),
      ),
    )),

    finally_clause: $ => seq(
      'finally',
      field('body', $.compound_statement),
    ),

    catch_clause: $ => seq(
      'catch',
      field('parameters', $.parameter_list),
      field('body', $.compound_statement),
    ),

    type_specifier_macro_call: $ => prec(PREC.CALL + 8, seq(
      field('function', $.type_specifier_macro_identifier),
      field('arguments', $.macro_argument_list),
    )),

    macro_argument_list: $ => seq(
      '(',
      optional($.macro_argument_sequence),
      ')',
    ),

    macro_argument_sequence: $ => {
      const separator = ',';
      const comment = choice($.comment, alias($.macro_comment_argument, $.comment));
      const item = seq(repeat(comment), $._macro_argument_list_item);
      const items = seq(item, repeat(seq(separator, item)), optional(separator));
      return choice(
        alias($.macro_comment_argument, $.comment),
        items,
        seq(repeat(comment), separator, optional(items)),
      );
    },

    macro_comment_argument: _ => token(prec(4, choice(
      seq('//', /(\\+(.|\r?\n)|[^\\\n])*/),
      seq(
        '/*',
        /[^*]*\*+([^/*][^*]*\*+)*/,
        '/',
      ),
    ))),

    _macro_argument_list_item: $ => choice(
      '...',
      $.macro_empty_statement_argument,
      $.macro_function_pointer_type_descriptor,
      $.function_pointer_type_descriptor,
      $.macro_template_params_argument,
      $.macro_expression_without_semicolon,
      $.macro_function_type_argument,
      $.macro_signature_argument,
      $.macro_warning_flag_argument,
      $.macro_primitive_type_argument,
      $.macro_numeric_sequence_argument,
      $.macro_call_identifier_sequence_argument,
      $.macro_type_reference_argument,
      $.macro_dependent_type_argument,
      $.macro_address_argument,
      prec(1, $.number_literal),
      $.expression,
      $.macro_comparison_operator_argument,
      $.primitive_type,
      $.sized_type_specifier,
      $.type_descriptor,
      $.initializer_list,
      $.compound_statement,
      $.macro_parenthesized_argument,
      $.virtual_specifier,
      $.type_qualifier,
      $.noexcept,
      $.macro_statement_call_argument,
      $.macro_raw_argument,
    ),

    macro_empty_statement_argument: _ => ';',

    macro_warning_flag_argument: _ => token(prec(3, /(?:[A-Za-z_]\w*\+\+\d*(?:-[A-Za-z_]\w*)*|[A-Za-z_]\w*(?:\+\+\d*)?(?:-[A-Za-z_]\w*)+)/)),

    macro_statement_call_argument: _ => token(prec(
      1,
      /(?:return|throw)[ \t]+[A-Za-z_:][A-Za-z0-9_:]*(?:<[^>\r\n]+>)?\([^()\r\n]*\)/,
    )),

    macro_template_params_argument: _ => token(prec(
      3,
      /HAS_[0-9]+_TEMPLATE_PARAMS\((?:[^()\r\n]|\r?\n[ \t]*)*\)/,
    )),

    macro_auto_declaration_argument: _ => token(prec(
      3,
      /(?:const[ \t]+)?(?:auto|[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*(?:<[^<>\r\n]*(?:<[^<>\r\n]*>[^<>\r\n]*)*>)?)[ \t]+[A-Za-z_]\w*[ \t]*=[ \t]*(?:[^,();\r\n]|\((?:[^()]|\([^()]*\))*\))+/
    )),

    macro_return_argument: $ => prec(1, seq(
      'return',
      optional($.expression),
    )),

    macro_return_statement_argument: $ => prec(2, seq(
      'return',
      optional($.expression),
      ';',
    )),

    macro_function_pointer_type_descriptor: _ => token(prec(
      3,
      /(?:const[ \t]+)?[A-Za-z_:][A-Za-z0-9_:]*(?:<[^>\r\n]+>)?(?:[ \t]+[A-Za-z_:][A-Za-z0-9_:]*(?:<[^>\r\n]+>)?)*[ \t]*\([ \t]*\*[ \t]*(?:[A-Za-z_]\w*)?[ \t]*\)[ \t]*\([^()\r\n]*\)/,
    )),

    macro_function_type_argument: _ => token(prec(
      -1,
      /(?:(?:const|volatile)[ \t]+)*(?:void|char|char8_t|char16_t|char32_t|wchar_t|bool|short|int|long|float|double|signed|unsigned)(?:[ \t]+(?:const|volatile|short|int|long|signed|unsigned))*[ \t]*(?:[&*]+[ \t]*)?\([^()\r\n]*\)(?:[ \t]*(?:const|volatile|noexcept(?:\([^()\r\n]*\))?|&&?))*/,
    )),

    macro_primitive_type_argument: _ => token(prec(
      2,
      /(?:(?:const|volatile)[ \t]+)*(?:void|char|char8_t|char16_t|char32_t|wchar_t|bool|short|int|long|float|double|signed|unsigned)(?:[ \t]+(?:const|volatile|short|int|long|signed|unsigned|char|wchar_t))+(?:[ \t]*[&*]+)?/,
    )),

    macro_signature_argument: _ => token(prec(
      2,
      /(?:const[ \t]+)?[A-Za-z_:][A-Za-z0-9_:]*(?:<[^>\r\n]+>)?[ \t]*[&*]+[ \t]*\([^()\r\n]*\)/,
    )),

    macro_type_reference_argument: _ => token(prec(
      -1,
      /(?:(?:const|volatile)[ \t]+)*(?:void|char|char8_t|char16_t|char32_t|wchar_t|bool|short|int|long|float|double|signed|unsigned|[A-Z_]\w*|[A-Za-z_]\w*(?:::[A-Za-z_]\w*)+)(?:[ \t]+(?:const|volatile|short|int|long|signed|unsigned))*(?:<[^>\r\n]+>)?[ \t]*[&*]+/,
    )),

    macro_dependent_type_argument: $ => prec(3, seq(
      'typename',
      $._class_name,
      optional(choice(
        $.abstract_pointer_declarator,
        $.abstract_reference_declarator,
      )),
    )),

    macro_numeric_sequence_argument: _ => token(prec(
      3,
      /\d+(?:[ \t]+(?:\d+|[A-Za-z_]\w*|\/\*[^*]*\*+(?:[^/*][^*]*\*+)*\/))+/,
    )),

    macro_comparison_operator_argument: _ => token(prec(
      3,
      /==|!=|<=|>=|<=>|<|>/,
    )),

    macro_address_argument: _ => token(prec(2, /&[A-Za-z_]\w*(?:(?:::[A-Za-z_]\w*)|(?:->|\.)[A-Za-z_]\w*)*/)),

    macro_call_identifier_sequence_argument: _ => token(prec(
      1,
      /[A-Za-z_]\w*\([^()\r\n]*\)[ \t]+[A-Za-z_]\w*/,
    )),

    macro_raw_argument: _ => token(prec(-1, /(?:[A-Za-z_]\w*(?:\([^()\r\n]*\))?|\d+)(?:[ \t]+(?:[A-Za-z_]\w*(?:\([^()\r\n]*\))?|\d+)){2,}/)),

    macro_parenthesized_argument: $ => seq(
      '(',
      optional($.macro_argument_sequence),
      ')',
    ),

    // Expressions

    _expression_not_binary: ($, original) => choice(
      $.macro_qualified_identifier,
      original,
      $.macro_call_expression,
      $.qualified_address_expression,
      $.delete_array_expression,
      $.throw_expression,
      alias('ref', $.identifier),
      $.co_await_expression,
      $.requires_expression,
      $.requires_clause,
      alias('import', $.identifier),
      alias('module', $.identifier),
      alias($.macro_token_paste_identifier, $.identifier),
      $.suffixed_string_literal,
      $.template_function,
      $.qualified_identifier,
      $.typeid_expression,
      $.cpp_cast_expression,
      $.new_expression,
      $.gcnew_expression,
      $.delete_expression,
      $.lambda_expression,
      $.parameter_pack_expansion,
      $.this,
      $.user_defined_literal,
      $.fold_expression,
    ),

    preproc_expression_item_fragment: _ => token(prec(
      -1,
      PREPROC_EXPRESSION_FRAGMENT,
    )),

    initializer_list: $ => {
      const item = choice(
        $.initializer_pair,
        $.expression,
        $.initializer_list,
      );
      return seq(
        '{',
        choice(
          commaSep(item),
          $._initializer_list_with_preproc,
        ),
        optional(','),
        '}',
      );
    },

    _initializer_list_with_preproc: $ => {
      const item = choice(
        $.initializer_pair,
        $.expression,
        $.initializer_list,
      );
      const preprocItem = preprocListItem($, '_in_initializer_list', PREPROC_IFDEF | PREPROC_ELSE);
      return prec(-1, seq(
        repeat(seq(item, ',')),
        preprocItem,
        repeat(choice(
          preprocItem,
          seq(item, ','),
          seq(item, optional(','), preprocItem),
        )),
        optional(item),
      ));
    },

    macro_statement_argument_list: $ => seq(
      '(',
      field('statement', choice(
        $.macro_call_statement_argument,
        $._macro_statement_argument_expression,
        $._argument_list_item,
      )),
      repeat(seq(
        ',',
        optional(alias($.macro_comment_argument, $.comment)),
        field('argument', $._macro_argument_list_item),
      )),
      optional(','),
      ')',
    ),

    macro_statement_argument_call: $ => prec(PREC.CALL, seq(
      field('function', $.statement_argument_macro_identifier),
      field('arguments', $.macro_statement_argument_list),
    )),

    macro_statement_argument_expression_statement: $ => seq(
      $._macro_statement_argument_expression,
      ';',
    ),

    _macro_statement_argument_expression: $ => choice(
      $.macro_statement_argument_call,
      alias($.macro_statement_argument_stream_expression, $.binary_expression),
    ),

    macro_statement_argument_stream_expression: $ => prec.left(PREC.SHIFT, seq(
      field('left', choice(
        $.macro_statement_argument_call,
        alias($.macro_statement_argument_stream_expression, $.binary_expression),
      )),
      field('operator', '<<'),
      field('right', $.expression),
    )),

    macro_call_statement_argument: $ => choice(
      $.macro_single_statement_argument,
      $.macro_statement_sequence_argument,
    ),

    macro_statement_sequence_argument: $ => seq(
      repeat1(seq($.macro_complete_statement_item, ';')),
      $.macro_call_statement_item,
    ),

    macro_complete_statement_item: $ => choice(
      $.macro_empty_statement_argument,
      alias($.macro_auto_declaration_argument, $.declaration),
      alias($._macro_statement_argument_expression, $.expression_statement),
      alias($.macro_expression_without_semicolon, $.expression_statement),
      $.compound_statement,
      $.if_statement,
      $.for_statement,
      $.while_statement,
      $.switch_statement,
      $.try_statement,
      $.macro_return_statement_argument,
      $.macro_return_argument,
    ),

    macro_call_statement_item: $ => choice(
      $.macro_empty_statement_argument,
      alias($.macro_initialized_declaration_fragment, $.declaration),
      alias($.macro_declaration_without_semicolon, $.declaration),
      alias($._macro_statement_argument_expression, $.expression_statement),
      alias($.macro_expression_without_semicolon, $.expression_statement),
      $.compound_statement,
      $.if_statement,
      $.for_statement,
      $.while_statement,
      $.switch_statement,
      $.try_statement,
      $.macro_return_statement_argument,
      $.macro_return_argument,
    ),

    macro_single_statement_argument: $ => choice(
      $.macro_empty_statement_argument,
      alias($.macro_auto_declaration_argument, $.declaration),
      alias($.macro_initialized_declaration_fragment, $.declaration),
      alias($.macro_declaration_without_semicolon, $.declaration),
      $.compound_statement,
      $.if_statement,
      $.for_statement,
      $.while_statement,
      $.switch_statement,
      $.try_statement,
      $.macro_return_statement_argument,
      $.macro_return_argument,
    ),

    macro_initialized_declaration_fragment: $ => prec(1, seq(
      $._declaration_specifiers,
      field('declarator', prec(1, $.identifier)),
      '=',
      field('default_value', choice($.expression, $.initializer_list)),
    )),

    macro_function_header_fragment: $ => prec(PREC.CALL + 2, seq(
      $._declaration_specifiers,
      field('declarator', choice(
        prec(1, seq(
          field('declarator', $.identifier),
          $._function_declarator_seq,
        )),
        $.function_declarator,
      )),
    )),

    macro_declaration_without_semicolon: $ => prec(1, seq(
      $._declaration_specifiers,
      field('declarator', choice(
        seq(
          optional($.ms_call_modifier),
          $._declarator,
          optional($.gnu_asm_expression),
          '=',
          field('value', choice($.initializer_list, $.expression)),
        ),
        seq(
          optional($.ms_call_modifier),
          $.function_declarator,
          optional($.gnu_asm_expression),
        ),
      )),
      optional($.declaration_suffix_preproc_ifdef),
    )),

    macro_expression_without_semicolon: $ => prec(PREC.CALL + 2, $.expression),

    macro_call_expression: $ => prec(PREC.CALL, seq(
      field('function', choice($.call_syntax_macro_identifier, $.semicolonless_macro_call_identifier_fallback)),
      field('arguments', $.macro_argument_list),
    )),

    macro_qualified_identifier: $ => choice(
      seq(
        $.bare_macro_identifier,
        $.identifier,
      ),
      $.macro_qualified_identifier_fallback,
    ),

    macro_qualified_identifier_fallback: _ => token(prec(
      1,
      /(?:(?:[A-Z][A-Z0-9]*|_[A-Z0-9]+)(?:_[A-Z0-9]+)+[ \t]+(?:[A-Z][A-Za-z0-9]*|_[A-Za-z0-9]+)(?:_[A-Za-z0-9]+)+)|(?:[A-Z][A-Z0-9_]*_NAMESPACE[ \t]+[a-z]\w*)/,
    )),

    qualified_address_expression: _ => token(prec(
      3,
      /&[A-Za-z_]\w*(?:<[^<>\r\n]*(?:<[^<>\r\n]*>[^<>\r\n]*)*>)?(?:::(?:template[ \t]+)?(?:[A-Za-z_]\w*(?:<[^<>\r\n]*(?:<[^<>\r\n]*>[^<>\r\n]*)*>)?|operator\(\)))+/,
    )),

    delete_array_expression: _ => token(prec(1, /delete[ \t]*\[\][ \t]*[A-Za-z_]\w*/)),

    typeid_expression: $ => prec(PREC.CALL, seq(
      'typeid',
      '(',
      field('value', choice($.expression, $.type_descriptor)),
      ')',
    )),

    cpp_cast_expression: $ => prec(PREC.CALL, seq(
      field('function', choice(
        'static_cast',
        'reinterpret_cast',
        'const_cast',
        'dynamic_cast',
      )),
      '<',
      field('type', $.type_descriptor),
      '>',
      field('argument', $.argument_list),
    )),

    _string: $ => choice(
      $.string_literal,
      $.raw_string_literal,
      $.concatenated_string,
    ),

    raw_string_literal: $ => seq(
      choice('R"', 'LR"', 'uR"', 'UR"', 'u8R"'),
      choice(
        seq(
          field('delimiter', $.raw_string_delimiter),
          '(',
          $.raw_string_content,
          ')',
          $.raw_string_delimiter,
        ),
        seq('(', $.raw_string_content, ')'),
      ),
      '"',
    ),

    subscript_expression: $ => prec(PREC.SUBSCRIPT, seq(
      field('argument', $.expression),
      field('indices', $.subscript_argument_list),
    )),

    subscript_argument_list: $ => seq(
      '[',
      commaSepWithPreproc($, choice($.expression, $.initializer_list), '_in_expression_list', 0),
      ']',
    ),

    call_expression: ($, original) => choice(
      original,
      prec(PREC.CALL, seq(
        field('function', choice(
          $.qualified_identifier,
          $.template_function,
        )),
        field('arguments', choice($.argument_list, $.bare_macro_identifier)),
      )),
      seq(
        field('function', choice($.primitive_type, $.dependent_type)),
        field('arguments', $.argument_list),
      ),
    ),

    co_await_expression: $ => prec.left(PREC.UNARY, seq(
      field('operator', 'co_await'),
      field('argument', $.expression),
    )),

    new_expression: $ => prec.right(PREC.NEW, seq(
      optional('::'),
      'new',
      field('placement', optional($.argument_list)),
      field('type', $.new_type_specifier),
      field('declarator', optional($.new_declarator)),
      field('arguments', optional(choice(
        $.argument_list,
        $.initializer_list,
      ))),
    )),

    gcnew_expression: $ => prec.right(PREC.NEW, seq(
      'gcnew',
      field('type', $.new_type_specifier),
      field('declarator', optional($.new_declarator)),
      field('arguments', optional(choice(
        $.argument_list,
        $.initializer_list,
      ))),
    )),

    new_type_specifier: $ => seq(
      repeat($._declaration_modifiers),
      $.type_specifier,
    ),

    new_declarator: $ => prec.right(choice(
      seq(
        repeat1('*'),
        optional($.new_array_declarator),
      ),
      $.new_array_declarator,
    )),

    new_array_declarator: $ => prec.right(seq(
      '[',
      field('length', $.expression),
      ']',
      optional($.new_array_declarator),
    )),

    delete_expression: $ => seq(
      optional('::'),
      choice(
        seq('delete', '[', ']'),
        'delete',
      ),
      $.expression,
    ),

    field_expression: $ => seq(
      prec(PREC.FIELD, seq(
        field('argument', $.expression),
        field('operator', choice('.', '.*', '->', '->*')),
      )),
      field('field', choice(
        prec.dynamic(1, $._field_identifier),
        alias($.qualified_field_identifier, $.qualified_identifier),
        $.destructor_name,
        alias($.operator_cast_field_identifier, $.operator_cast),
        $.template_method,
        alias($.dependent_field_identifier, $.dependent_name),
      )),
    ),

    operator_cast_field_identifier: _ => token(prec(
      1,
      /operator[ \t]+(?:unsigned[ \t]+)?(?:int|long|short|char|bool|float|double)/,
    )),

    type_requirement: $ => seq('typename', $._class_name),

    nested_requirement: $ => prec(1, seq(
      'requires',
      field('constraint', $._requirement_clause_constraint),
      ';',
    )),

    compound_requirement: $ => seq(
      '{', $.expression, '}',
      optional('noexcept'),
      optional($.trailing_return_type),
      ';',
    ),

    _requirement: $ => choice(
      alias($.expression_statement, $.simple_requirement),
      $.type_requirement,
      $.nested_requirement,
      $.compound_requirement,
    ),

    requirement_seq: $ => seq('{', repeat($._requirement), '}'),

    constraint_conjunction: $ => prec.left(PREC.LOGICAL_AND, seq(
      field('left', $._requirement_clause_constraint),
      field('operator', choice('&&', 'and')),
      field('right', $._requirement_clause_constraint)),
    ),

    constraint_disjunction: $ => prec.left(PREC.LOGICAL_OR, seq(
      field('left', $._requirement_clause_constraint),
      field('operator', choice('||', 'or')),
      field('right', $._requirement_clause_constraint)),
    ),

    _requirement_clause_constraint: $ => choice(
      // Primary expressions"
      $.true,
      $.false,
      $._class_name,
      $.fold_expression,
      $.lambda_expression,
      $.requires_expression,
      $.unary_expression,

      // Parenthesized expressions
      seq('(', $.expression, ')'),

      // conjunction or disjunction of the above
      $.constraint_conjunction,
      $.constraint_disjunction,
    ),

    requires_clause: $ => seq(
      'requires',
      field('constraint', $._requirement_clause_constraint),
    ),

    _requires_parameter_list_item: $ => choice(
      $.parameter_declaration,
      $.optional_parameter_declaration,
      $.variadic_parameter_declaration,
    ),

    requires_parameter_list: $ => seq(
      '(',
      commaSep($._requires_parameter_list_item),
      ')',
    ),

    requires_expression: $ => seq(
      'requires',
      field('parameters', optional(alias($.requires_parameter_list, $.parameter_list))),
      field('requirements', $.requirement_seq),
    ),

    lambda_expression: $ => seq(
      field('captures', $.lambda_capture_specifier),
      optional(seq(
        field('template_parameters', $.template_parameter_list),
        optional(field('constraint', $.requires_clause)),
      )),
      optional(field('declarator', $.abstract_function_declarator)),
      field('body', $.compound_statement),
    ),

    lambda_capture_specifier: $ => prec(PREC.LAMBDA, seq(
      '[',
      choice(
        $.lambda_default_capture,
        commaSep($._lambda_capture),
        seq(
          $.lambda_default_capture,
          ',', commaSep1($._lambda_capture),
        ),
      ),
      ']',
    )),

    lambda_default_capture: _ => choice('=', '&'),

    _lambda_capture_identifier: $ => seq(
      optional('&'),
      choice(
        $.identifier,
        $.qualified_identifier,
        alias($.identifier_parameter_pack_expansion, $.parameter_pack_expansion),
      ),
    ),

    lambda_capture_initializer: $ => seq(
      optional('&'),
      optional('...'),
      field('left', $.identifier),
      choice(
        seq('=', field('right', $.expression)),
        field('right', $.initializer_list),
      ),
    ),

    _lambda_capture: $ => choice(
      seq(optional('*'), $.this),
      $._lambda_capture_identifier,
      $.lambda_capture_initializer,
    ),

    _fold_operator: _ => choice(...FOLD_OPERATORS),
    _binary_fold_operator: _ => choice(...FOLD_OPERATORS.map((operator) => seq(field('operator', operator), '...', operator))),

    _unary_left_fold: $ => seq(
      field('left', '...'),
      field('operator', $._fold_operator),
      field('right', $.expression),
    ),
    _unary_right_fold: $ => seq(
      field('left', $.expression),
      field('operator', $._fold_operator),
      field('right', '...'),
    ),
    _binary_fold: $ => seq(
      field('left', $.expression),
      $._binary_fold_operator,
      field('right', $.expression),
    ),

    fold_expression: $ => seq(
      '(',
      choice(
        $._unary_right_fold,
        $._unary_left_fold,
        $._binary_fold,
      ),
      ')',
    ),

    parameter_pack_expansion: $ => prec(-1, seq(
      field('pattern', $.expression),
      '...',
    )),

    type_parameter_pack_expansion: $ => seq(
      field('pattern', $.type_descriptor),
      '...',
    ),

    identifier_parameter_pack_expansion: $ => seq(
      field('pattern', $.identifier),
      '...',
    ),

    sizeof_expression: ($, original) => choice(
      prec(PREC.CALL + 3, seq(
        'sizeof',
        '(',
        field('type', choice(
          $.decltype_function_pointer_type_descriptor,
          $.function_pointer_type_descriptor,
        )),
        ')',
      )),
      prec.right(PREC.SIZEOF, choice(
        original,
        seq(
          'sizeof', '...',
          '(',
          field('value', $.identifier),
          ')',
        ),
      )),
    ),

    unary_expression: ($, original) => choice(
      original,
      prec.left(PREC.UNARY, seq(
        field('operator', choice('not', 'compl')),
        field('argument', $.expression),
      )),
    ),

    binary_expression: $ => {
      const table = [
        ['+', PREC.ADD],
        ['-', PREC.ADD],
        ['*', PREC.MULTIPLY],
        ['/', PREC.MULTIPLY],
        ['%', PREC.MULTIPLY],
        ['||', PREC.LOGICAL_OR, true],
        ['&&', PREC.LOGICAL_AND, true],
        ['|', PREC.INCLUSIVE_OR],
        ['^', PREC.EXCLUSIVE_OR],
        ['&', PREC.BITWISE_AND],
        ['==', PREC.EQUAL],
        ['!=', PREC.EQUAL],
        ['>', PREC.RELATIONAL, true],
        ['>=', PREC.RELATIONAL, true],
        ['<=', PREC.RELATIONAL, true],
        ['<', PREC.RELATIONAL, true],
        ['<<', PREC.SHIFT],
        ['>>', PREC.SHIFT],
        ['<=>', PREC.THREE_WAY],
        ['or', PREC.LOGICAL_OR],
        ['and', PREC.LOGICAL_AND],
        ['bitor', PREC.INCLUSIVE_OR],
        ['xor', PREC.EXCLUSIVE_OR],
        ['bitand', PREC.BITWISE_AND],
        ['not_eq', PREC.EQUAL],
      ];

      return choice(
        prec.left(PREC.INCLUSIVE_OR, seq(
          field('left', $.expression),
          field('operator', '|'),
          $.preproc_argument_fragment,
          field('right', $.expression),
        )),
        prec.left(PREC.LOGICAL_OR, seq(
          field('left', $.expression),
          $.preproc_logical_expression_fragment,
        )),
        prec.left(PREC.LOGICAL_AND, seq(
          field('left', $.expression),
          field('operator', '&&'),
          $.preproc_logical_tail_expression_fragment,
        )),
        ...table.map(([operator, precedence, preferBinary]) => {
          const rule = prec.left(precedence, seq(
            field('left', $.expression),
            // @ts-ignore
            field('operator', operator),
            field('right', $.expression),
          ));
          // Prefer real binary expressions over template-id recovery for
          // relational/logical chains such as "value < min || value > max".
          return preferBinary ? prec.dynamic(1, rule) : rule;
        }));
    },

    preproc_logical_expression_fragment: _ => token(prec(
      1,
      /#[ \t]*if[^\n]*\r?\n[ \t]*(?:\|\||&&)[^\r\n]*(?:\r?\n[ \t]*[^#\r\n][^\r\n]*)*\r?\n#[ \t]*endif/,
    )),

    preproc_logical_tail_expression_fragment: _ => token(prec(
      1,
      /#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\n(?:[ \t]*\/\/[^\r\n]*\r?\n)*[ \t]*(?:true|false|!?[A-Za-z_][^\r\n]*)(?:\r?\n[ \t]*[^#\r\n][^\r\n]*)*\r?\n(?:#[ \t]*else[^\n]*\r?\n(?:[ \t]*\/\/[^\r\n]*\r?\n)*[ \t]*(?:true|false|!?[A-Za-z_][^\r\n]*)(?:\r?\n[ \t]*[^#\r\n][^\r\n]*)*\r?\n)?#[ \t]*endif/,
    )),

    // The compound_statement is added to parse macros taking statements as arguments, e.g. MYFORLOOP(1, 10, i, { foo(i); bar(i); })
    argument_list: $ => seq(
      '(',
      choice(
        seq(
          commaSepWithPreproc($, $._argument_list_item, '_in_expression_list', PREPROC_IFDEF),
          optional(','),
        ),
        seq(',', commaSep1($._argument_list_item)),
        seq(repeat1($.preproc_argument_fragment), commaSep($._argument_list_item)),
      ),
      ')',
    ),

    _argument_list_item: $ => choice(
      $.tagged_type_argument,
      $.expression,
      $.initializer_list,
      $.compound_statement,
    ),

    tagged_type_argument: $ => prec(PREC.CALL + 2, choice(
      alias(seq('struct', field('name', $._type_identifier)), $.struct_specifier),
      alias(seq('union', field('name', $._type_identifier)), $.union_specifier),
      alias(seq('enum', field('name', $._type_identifier)), $.enum_specifier),
      alias(seq('class', field('name', $._type_identifier)), $.class_specifier),
    )),

    braced_argument_list: $ => prec(PREC.CALL + 1, seq(
      '(',
      choice(
        seq($._braced_argument_list_item, repeat(seq(',', $._argument_list_item)), optional(',')),
        seq(repeat1(seq($._argument_list_item, ',')), $._braced_argument_list_item, repeat(seq(',', $._argument_list_item)), optional(',')),
        seq(',', $._braced_argument_list_item, repeat(seq(',', $._argument_list_item)), optional(',')),
      ),
      ')',
    )),

    _braced_argument_list_item: $ => prec(PREC.CALL + 2, choice(
      $.primitive_braced_argument,
      $.compound_literal_expression,
      $.initializer_list,
    )),

    primitive_braced_argument: $ => seq(
      $.primitive_type,
      $.initializer_list,
    ),

    primitive_braced_argument_list: _ => token(prec(
      2,
      /\((?:[^(){}\r\n,]+[ \t]*,[ \t]*)*(?:bool|char|char8_t|char16_t|char32_t|double|float|int|long|short|signed|unsigned|wchar_t)[ \t]*\{[^{}\r\n]*\}(?:[ \t]*,[ \t]*[^(){}\r\n,]+)*[ \t]*,?\)/,
    )),

    preproc_argument_fragment: _ => token(prec(
      1,
      /#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\n(?:[ \t]*(?:(?:\/\/[^\n]*)|(?:[^#\r\n]*[+\-*\/%&|^<>=!?:.][ \t]*))\r?\n)*(?:#[ \t]*else[^\n]*\r?\n(?:[ \t]*(?:(?:\/\/[^\n]*)|(?:[^#\r\n]*[+\-*\/%&|^<>=!?:.][ \t]*))\r?\n)*)?#[ \t]*endif/,
    )),

    destructor_name: $ => prec(1, seq('~', $.identifier)),

    compound_literal_expression: ($, original) => choice(
      original,
      prec(PREC.CALL, seq(
        field('type', choice(
          $._class_name,
          $.primitive_type,
          $.decltype,
          $.dependent_type,
          $.template_type,
          $.qualified_type_identifier,
          $.qualified_identifier,
        )),
        field('value', $.initializer_list),
      )),
      prec(PREC.CALL, seq(
        field('type', $.decltype),
        field('value', $.argument_list),
      )),
    ),

    dependent_identifier: $ => seq('template', $.template_function),
    dependent_field_identifier: $ => seq('template', choice($.template_method, $._field_identifier)),
    dependent_type_identifier: $ => seq('template', $.template_type),

    _scope_resolution: $ => prec(1, seq(
      field('scope', optional(seq(
        $._scope_name,
        repeat(seq('::', $._scope_name)),
      ))),
      '::',
    )),

    _scope_name: $ => prec(1, choice(
      $._namespace_identifier,
      $.template_type,
      $.decltype,
      alias($.dependent_type_identifier, $.dependent_name),
    )),

    qualified_field_identifier: $ => seq(
      $._scope_resolution,
      field('name', choice(
        alias($.dependent_field_identifier, $.dependent_name),
        alias($.qualified_field_identifier, $.qualified_identifier),
        $.template_method,
        $.operator_name,
        prec.dynamic(1, $._field_identifier),
      )),
    ),

    qualified_identifier: $ => seq(
      $._scope_resolution,
      field('name', choice(
        alias($.dependent_identifier, $.dependent_name),
        $.qualified_identifier,
        $.template_function,
        seq(optional('template'), $.identifier),
        $.operator_name,
        $.destructor_name,
        $.pointer_type_declarator,
      )),
    ),

    qualified_type_identifier: $ => seq(
      $._scope_resolution,
      field('name', choice(
        alias($.dependent_type_identifier, $.dependent_name),
        alias($.qualified_type_identifier, $.qualified_identifier),
        $.template_type,
        $._type_identifier,
      )),
    ),

    qualified_operator_cast_identifier: $ => seq(
      $._scope_resolution,
      field('name', choice(
        alias($.qualified_operator_cast_identifier, $.qualified_identifier),
        $.operator_cast,
      )),
    ),

    _assignment_left_expression: ($, original) => choice(
      original,
      $.template_function,
      $.qualified_identifier,
      $.compound_literal_expression,
      $.cpp_cast_expression,
      alias('ref', $.identifier),
      alias('import', $.identifier),
      alias('module', $.identifier),
      $.user_defined_literal,
    ),

    assignment_expression: $ => prec.right(PREC.ASSIGNMENT, seq(
      field('left', $._assignment_left_expression),
      field('operator', choice(...ASSIGNMENT_OPERATORS)),
      field('right', choice($.expression, $.initializer_list)),
    )),

    _assignment_expression_lhs: $ => seq(
      field('left', $.expression),
      field('operator', choice(...ASSIGNMENT_OPERATORS)),
      field('right', choice($.expression, $.initializer_list)),
    ),

    // This prevents an ambiguity between fold expressions
    // and assignment expressions within parentheses.
    parenthesized_expression: ($, original) => choice(
      prec(1, seq(
        '(',
        alias($._assignment_expression_lhs, $.assignment_expression),
        ',',
        choice($.expression, $.comma_expression),
        ')',
      )),
      seq('(', $.comma_expression, ')'),
      original,
      seq('(', alias($._assignment_expression_lhs, $.assignment_expression), ')'),
    ),

    operator_name: $ => prec(1, seq(
      'operator',
      choice(
        'co_await',
        '+', '-', '*', '/', '%',
        '^', '&', '|', '~',
        '!', '=', '<', '>',
        '+=', '-=', '*=', '/=', '%=', '^=', '&=', '|=',
        '<<', '>>', '>>=', '<<=',
        '==', '!=', '<=', '>=',
        '<=>',
        '&&', '||',
        '++', '--',
        ',',
        '->*',
        '->',
        '()', '[]',
        'xor', 'bitand', 'bitor', 'compl',
        'not', 'xor_eq', 'and_eq', 'or_eq', 'not_eq',
        'and', 'or',
        seq(choice('new', 'delete'), optional('[]')),
        seq('""', $.identifier),
      ),
    )),

    this: _ => 'this',

    concatenated_string: $ => {
      const stringFragment = choice($.suffixed_string_literal, $.raw_string_literal, $.string_literal);
      const macroStringFragment = $.macro_interleaved_concatenated_string;
      const fragment = choice(
        stringFragment,
        $.identifier,
        $.macro_stringification,
        $.preproc_string_literal_fragment,
        macroStringFragment,
      );
      return prec.right(2, choice(
        macroStringFragment,
        seq(
          choice(
            seq($.identifier, stringFragment),
            seq(stringFragment, fragment),
            seq($.preproc_string_literal_fragment, choice(stringFragment, $.identifier, macroStringFragment)),
            seq(macroStringFragment, choice(stringFragment, $.identifier, $.macro_stringification, $.preproc_string_literal_fragment)),
          ),
          repeat(fragment),
        ),
      ));
    },

    macro_interleaved_concatenated_string: _ => {
      const macro = /[A-Z_][A-Za-z0-9_]*_?/;
      const literal = /(?:L|u|U|u8)?"(?:\\.|[^"\\\n])*"/;
      const separator = /(?:[ \t]|\r?\n[ \t]*)+/;
      return token(prec(3, choice(
        seq(
          repeat1(seq(macro, separator)),
          literal,
          repeat(seq(separator, choice(macro, literal))),
        ),
        seq(
          repeat1(seq(literal, separator)),
          macro,
          repeat(seq(separator, choice(macro, literal))),
        ),
      )));
    },

    suffixed_string_literal: _ => token(prec(
      2,
      /(?:L|u|U|u8)?"(?:\\.|[^"\\\n])*"[a-zA-Z_]\w*/,
    )),

    macro_stringification: _ => token(prec(1, /#[A-Za-z_]\w*/)),

    preproc_string_literal_fragment: _ => token(prec(
      0,
      /#[ \t]*(?:if|ifdef|ifndef)[^\n]*\r?\n[ \t]*(?:(?:L|u|U|u8)?"|[A-Za-z_]\w*[^"\r\n]*")[^;{}\r\n]*\r?\n(?:(?:[ \t]*(?:(?:L|u|U|u8)?"|[A-Za-z_]\w*)[^;{}\r\n]*\r?\n)|(?:#[ \t]*(?:elif|elifdef|elifndef|else)[^\n]*\r?\n))*#[ \t]*endif[^\r\n]*/,
    )),

    number_literal: $ => {
      const sign = /[-\+]/;
      const separator = '\'';
      const binary = /[01]/;
      const binaryDigits = seq(repeat1(binary), repeat(seq(separator, repeat1(binary))));
      const decimal = /[0-9]/;
      const firstDecimal = /[1-9]/;
      const intDecimalDigits = seq(firstDecimal, repeat(decimal), repeat(seq(separator, repeat1(decimal))));
      const floatDecimalDigits = seq(repeat1(decimal), repeat(seq(separator, repeat1(decimal))));
      const hex = /[0-9a-fA-F]/;
      const hexDigits = seq(repeat1(hex), repeat(seq(separator, repeat1(hex))));
      const octal = /[0-7]/;
      const octalDigits = seq('0', repeat(octal), repeat(seq(separator, repeat1(octal))));
      const hexExponent = seq(/[pP]/, optional(sign), floatDecimalDigits);
      const decimalExponent = seq(/[eE]/, optional(sign), floatDecimalDigits);
      const intSuffix = /(ll|LL)[uU]?|[uU](ll|LL)?|[uU][lL]?|[uU][zZ]?|[lL][uU]?|[zZ][uU]?/;
      const floatSuffix = /([fF](16|32|64|128)?)|[lL]|(bf16|BF16)/;

      return token(seq(
        optional(sign),
        choice(
          seq(
            choice(
              seq(choice('0b', '0B'), binaryDigits),
              intDecimalDigits,
              seq(choice('0x', '0X'), hexDigits),
              octalDigits,
            ),
            optional(intSuffix),
          ),
          seq(
            choice(
              seq(floatDecimalDigits, decimalExponent),
              seq(floatDecimalDigits, '.', optional(floatDecimalDigits), optional(decimalExponent)),
              seq('.', floatDecimalDigits, optional(decimalExponent)),
              seq(
                choice('0x', '0X'),
                choice(
                  hexDigits,
                  seq(hexDigits, '.', optional(hexDigits)),
                  seq('.', hexDigits)),
                hexExponent,
              ),
            ),
            optional(floatSuffix),
          ),
        ),
      ));
    },

    literal_suffix: _ => token.immediate(/[a-zA-Z_]\w*/),

    user_defined_literal: $ => seq(
      choice(
        $.number_literal,
        $.char_literal,
        $._string,
      ),
      $.literal_suffix,
    ),

    _namespace_identifier: $ => alias($.identifier, $.namespace_identifier),
  },
});

/**
 * Creates a rule to optionally match one or more of the rules separated by a comma
 *
 * @param {Rule} rule
 *
 * @returns {ChoiceRule}
 */
function commaSep(rule) {
  return optional(commaSep1(rule));
}

function commaSepWithPreproc($, rule, suffix, forms = PREPROC_ALL_BRANCH_FORMS) {
  return seq(
    repeat(choice(
      seq(rule, ','),
      preprocListItem($, suffix, forms),
    )),
    optional(rule),
  );
}

function commaSepWithLeadingPreproc($, rule, suffix, forms = PREPROC_ALL_BRANCH_FORMS) {
  return choice(
    commaSepWithPreproc($, rule, suffix, forms),
    seq(
      repeat(seq(rule, ',')),
      rule,
      preprocListItem($, suffix, forms),
      repeat(choice(seq(',', rule), preprocListItem($, suffix, forms))),
    ),
  );
}

/**
 * Creates a rule to match one or more of the rules separated by a comma
 *
 * @param {Rule} rule
 *
 * @returns {SeqRule}
 */
function commaSep1(rule) {
  return seq(rule, repeat(seq(',', rule)));
}

function commaSep1WithRequiredPreproc($, rule, suffix, forms) {
  return seq(
    repeat(seq(rule, ',')),
    preprocListItem($, suffix, forms),
    repeat(choice(seq(rule, ','), preprocListItem($, suffix, forms))),
    optional(rule),
  );
}

function preprocListItem($, suffix, forms = PREPROC_ALL_BRANCH_FORMS) {
  const items = [alias($['preproc_if' + suffix], $.preproc_if)];
  if (forms & PREPROC_IFDEF) {
    items.push(alias($['preproc_ifdef' + suffix], $.preproc_ifdef));
  }
  return items.length === 1 ? items[0] : choice(...items);
}

function preprocIf(suffix, content, precedence = 0, forms = PREPROC_ALL_BRANCH_FORMS, repeatContent = true) {
  function branchContent($) {
    return repeatContent ? repeat(content($)) : content($);
  }

  function alternativeBlock($) {
    const alternatives = [];
    if (forms & PREPROC_ELSE) {
      alternatives.push(alias($['preproc_else' + suffix], $.preproc_else));
    }
    if (forms & PREPROC_ELIF) {
      alternatives.push(alias($['preproc_elif' + suffix], $.preproc_elif));
      if (suffix === '') {
        alternatives.push(alias($['preproc_elifdef' + suffix], $.preproc_elifdef));
      }
    }
    return alternatives.length === 1 ? alternatives[0] : choice(...alternatives);
  }

  function alternativeField($) {
    return forms & (PREPROC_ELSE | PREPROC_ELIF)
      ? [field('alternative', optional(alternativeBlock($)))]
      : [];
  }

  const rules = {
    ['preproc_if' + suffix]: $ => prec(precedence, seq(
      preprocessor('if'),
      field('condition', $._preproc_expression),
      $._preproc_directive_end,
      branchContent($),
      ...alternativeField($),
      preprocessor('endif'),
    )),
  };

  if (forms & PREPROC_IFDEF) {
    rules['preproc_ifdef' + suffix] = $ => prec(precedence, seq(
      choice(preprocessor('ifdef'), preprocessor('ifndef')),
      field('name', $.identifier),
      $._preproc_directive_end,
      branchContent($),
      ...alternativeField($),
      preprocessor('endif'),
    ));
  }

  if (forms & PREPROC_ELSE) {
    rules['preproc_else' + suffix] = $ => prec(precedence, seq(
      preprocessor('else'),
      $._preproc_directive_end,
      branchContent($),
    ));
  }

  if (forms & PREPROC_ELIF) {
    rules['preproc_elif' + suffix] = $ => prec(precedence, seq(
      preprocessor('elif'),
      field('condition', $._preproc_expression),
      $._preproc_directive_end,
      branchContent($),
      ...alternativeField($),
    ));

    if (suffix === '') {
      rules['preproc_elifdef' + suffix] = $ => prec(precedence, seq(
        choice(preprocessor('elifdef'), preprocessor('elifndef')),
        field('name', $.identifier),
        $._preproc_directive_end,
        branchContent($),
        ...alternativeField($),
      ));
    }
  }

  return rules;
}

function preprocessor(command) {
  const needsArgumentSeparator = command === 'if' ||
    command === 'ifdef' ||
    command === 'ifndef' ||
    command === 'elif' ||
    command === 'elifdef' ||
    command === 'elifndef' ||
    command === 'define';
  const pattern = needsArgumentSeparator ? '#[ \\t]*' + command + '[ \\t]+' : '#[ \\t]*' + command;
  return alias(token(prec(1, new RegExp(pattern))), '#' + command);
}

function preprocessorInclude() {
  return alias(token(prec(1, /#[ \t]*include[ \t]+/)), '#include');
}
