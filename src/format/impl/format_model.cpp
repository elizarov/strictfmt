#include "format/impl/format_model.h"

#include <array>
#include <stdexcept>
#include <tree_sitter_cpp.h>
#include <unordered_map>

namespace {

struct SyntaxKindMapping {
    SyntaxNodeKind kind = SyntaxNodeKind::Unknown;
    std::string_view treeType;
    std::string_view tokenText;
    std::uint64_t classes = 0;
};

struct SyntaxKindInfo {
    std::string_view tokenText;
    std::uint64_t classes = 0;
};

constexpr std::uint64_t Bit(SyntaxNodeClass syntaxNodeClass) { return static_cast<std::uint64_t>(syntaxNodeClass); }

constexpr SyntaxKindMapping Kind(SyntaxNodeKind kind, std::uint64_t classes = 0) { return {kind, {}, {}, classes}; }

constexpr SyntaxKindMapping Tree(SyntaxNodeKind kind, std::string_view treeType, std::uint64_t classes = 0) {
    return {kind, treeType, {}, Bit(SyntaxNodeClass::Tree) | classes};
}

constexpr SyntaxKindMapping Token(SyntaxNodeKind kind, std::string_view tokenText, std::uint64_t classes = 0) {
    return {kind, {}, tokenText, Bit(SyntaxNodeClass::Known) | classes};
}

constexpr SyntaxKindMapping Keyword(SyntaxNodeKind kind, std::string_view tokenText, std::uint64_t classes = 0) {
    return Token(kind, tokenText, Bit(SyntaxNodeClass::Keyword) | classes);
}

constexpr std::uint64_t kStringLikeClasses =
    Bit(SyntaxNodeClass::Literal) | Bit(SyntaxNodeClass::StringLike) | Bit(SyntaxNodeClass::LexicalAtom);
constexpr std::uint64_t kNumberLiteralClasses = Bit(SyntaxNodeClass::Literal) | Bit(SyntaxNodeClass::LexicalAtom);
constexpr std::uint64_t kCommentClasses = Bit(SyntaxNodeClass::Comment) | Bit(SyntaxNodeClass::Trivia);
constexpr std::uint64_t kAtomicPreprocessorClasses = Bit(SyntaxNodeClass::AtomicPreprocessor);
constexpr std::uint64_t kSupportedPreprocessorPlacementClasses = Bit(SyntaxNodeClass::SupportedPreprocessorPlacement);

constexpr std::uint64_t kDeclarationModifierPreprocessorClasses = kAtomicPreprocessorClasses |
    kSupportedPreprocessorPlacementClasses |
    Bit(SyntaxNodeClass::DeclarationModifierPreprocessor);

constexpr std::uint64_t kConditionalRhsPreprocessorClasses = kAtomicPreprocessorClasses |
    kSupportedPreprocessorPlacementClasses |
    Bit(SyntaxNodeClass::ConditionalRhsPreprocessor);

constexpr std::uint64_t kChainBinaryClasses =
    Bit(SyntaxNodeClass::BinaryOperator) | Bit(SyntaxNodeClass::ChainOperator);
constexpr std::uint64_t kAllowedPreprocessorContainerClasses = Bit(SyntaxNodeClass::AllowedPreprocessorContainer);
constexpr std::uint64_t kAllowedListPreprocessorContainerClasses =
    kAllowedPreprocessorContainerClasses | Bit(SyntaxNodeClass::AllowedListPreprocessorContainer);

constexpr std::uint64_t kPreprocessorSplitListClasses = kAllowedListPreprocessorContainerClasses |
    Bit(SyntaxNodeClass::PreprocessorSplitList) |
    Bit(SyntaxNodeClass::SemanticDelimitedParent);

constexpr std::uint64_t kPreprocessingTokenArgumentListClasses =
    kPreprocessorSplitListClasses | Bit(SyntaxNodeClass::PreserveTrailingComma);

constexpr std::uint64_t kConditionalPreprocessorTreeClasses = kAllowedPreprocessorContainerClasses |
    kSupportedPreprocessorPlacementClasses |
    Bit(SyntaxNodeClass::ConditionalPreprocessorTree) |
    Bit(SyntaxNodeClass::ConditionalPreprocessorDirective);

constexpr std::uint64_t kConditionalPreprocessorOpenClasses = kConditionalPreprocessorTreeClasses |
    Bit(SyntaxNodeClass::ConditionalPreprocessorOpen) |
    Bit(SyntaxNodeClass::ConditionalOpeningDirective) |
    Bit(SyntaxNodeClass::CheckedPreprocessorDirective);

constexpr std::uint64_t kPreprocessorDirectiveClasses = Bit(SyntaxNodeClass::PreprocessorDirective);
constexpr std::uint64_t kCheckedPreprocessorDirectiveClasses =
    kPreprocessorDirectiveClasses | Bit(SyntaxNodeClass::CheckedPreprocessorDirective);
constexpr std::uint64_t kConditionalPreprocessorDirectiveClasses =
    kPreprocessorDirectiveClasses | Bit(SyntaxNodeClass::ConditionalPreprocessorDirective);

constexpr std::uint64_t kConditionalOpeningDirectiveClasses = kConditionalPreprocessorDirectiveClasses |
    Bit(SyntaxNodeClass::ConditionalOpeningDirective) |
    Bit(SyntaxNodeClass::CheckedPreprocessorDirective);

constexpr std::uint64_t kConditionalBranchSeparatorDirectiveClasses =
    kConditionalPreprocessorDirectiveClasses | Bit(SyntaxNodeClass::ConditionalBranchSeparatorDirective);
constexpr std::uint64_t kEndifDirectiveClasses =
    kConditionalBranchSeparatorDirectiveClasses | Bit(SyntaxNodeClass::EndifDirective);

constexpr std::uint64_t kSymbolLocalClasses = Bit(SyntaxNodeClass::OpaqueSource) |
    Bit(SyntaxNodeClass::LexicalAtom) |
    Bit(SyntaxNodeClass::AtomicPreprocessor) |
    Bit(SyntaxNodeClass::DeclarationModifierPreprocessor) |
    Bit(SyntaxNodeClass::ConditionalRhsPreprocessor) |
    Bit(SyntaxNodeClass::PreserveTrailingComma) |
    Bit(SyntaxNodeClass::ConditionalFunctionHeader) |
    Bit(SyntaxNodeClass::LeadingStreamOperatorChain) |
    Bit(SyntaxNodeClass::ConditionalStreamOperatorChain) |
    Bit(SyntaxNodeClass::DeclarationScope) |
    Bit(SyntaxNodeClass::DeclarationGroupType) |
    Bit(SyntaxNodeClass::DeclarationGroupForwardType) |
    Bit(SyntaxNodeClass::DeclarationGroupCallable) |
    Bit(SyntaxNodeClass::DeclarationGroupObject) |
    Bit(SyntaxNodeClass::DeclarationGroupAlias) |
    Bit(SyntaxNodeClass::Expression) |
    Bit(SyntaxNodeClass::QualifiedName);

constexpr auto kSyntaxKindMappings = std::to_array<SyntaxKindMapping>({
    Kind(SyntaxNodeKind::Tree, Bit(SyntaxNodeClass::Tree)),
    Kind(SyntaxNodeKind::Comment, kCommentClasses | Bit(SyntaxNodeClass::ListForceSplitMarker)),
    Kind(SyntaxNodeKind::TrailingComment, kCommentClasses | Bit(SyntaxNodeClass::ListForceSplitMarker)),
    Kind(SyntaxNodeKind::BlankLine, Bit(SyntaxNodeClass::Trivia) | Bit(SyntaxNodeClass::ListForceSplitMarker)),
    Kind(SyntaxNodeKind::Error, Bit(SyntaxNodeClass::Tree)),
    Kind(SyntaxNodeKind::Missing, Bit(SyntaxNodeClass::Tree)),
    Tree(
        SyntaxNodeKind::TranslationUnit,
        "translation_unit",
        kAllowedPreprocessorContainerClasses |
            Bit(SyntaxNodeClass::SourceItemScope) |
            Bit(SyntaxNodeClass::DeclarationScope)
    ),
    Kind(SyntaxNodeKind::IncludeRun, Bit(SyntaxNodeClass::Tree)),
    Tree(SyntaxNodeKind::MacroReplacementList, "macro_replacement_list"),
    Tree(
        SyntaxNodeKind::Declaration,
        "declaration",
        Bit(SyntaxNodeClass::MacroDeclarationFragment) | Bit(SyntaxNodeClass::DeclarationNode)
    ),
    Tree(SyntaxNodeKind::Declaration, "macro_declaration_fragment", Bit(SyntaxNodeClass::MacroDeclarationFragment)),
    Tree(SyntaxNodeKind::Declaration, "preproc_value_declaration", Bit(SyntaxNodeClass::MacroDeclarationFragment)),
    Tree(
        SyntaxNodeKind::Declaration,
        "concept_definition",
        Bit(SyntaxNodeClass::MacroDeclarationFragment) | Bit(SyntaxNodeClass::DeclarationGroupType)
    ),
    Tree(
        SyntaxNodeKind::FieldDeclaration,
        "field_declaration",
        Bit(SyntaxNodeClass::MacroDeclarationFragment) | Bit(SyntaxNodeClass::DeclarationNode)
    ),
    Tree(SyntaxNodeKind::FieldDeclaration, "macro_method_declaration", Bit(SyntaxNodeClass::MacroDeclarationFragment)),
    Tree(SyntaxNodeKind::AliasDeclaration, "alias_declaration", Bit(SyntaxNodeClass::MacroDeclarationFragment)),
    Tree(
        SyntaxNodeKind::FunctionPointerAliasDeclaration,
        "function_pointer_alias_declaration",
        Bit(SyntaxNodeClass::MacroDeclarationFragment)
    ),
    Tree(SyntaxNodeKind::Declaration, "deduction_guide_declaration", Bit(SyntaxNodeClass::MacroDeclarationFragment)),
    Tree(SyntaxNodeKind::Declaration, "module_declaration"),
    Tree(SyntaxNodeKind::Declaration, "module_import_declaration"),
    Tree(SyntaxNodeKind::FunctionDefinition, "function_definition", Bit(SyntaxNodeClass::MacroDeclarationFragment)),
    Tree(
        SyntaxNodeKind::FunctionDefinition, "macro_function_definition", Bit(SyntaxNodeClass::MacroDeclarationFragment)
    ),
    Tree(
        SyntaxNodeKind::CompoundStatement,
        "compound_statement",
        Bit(SyntaxNodeClass::CompoundBlock) |
            kAllowedPreprocessorContainerClasses |
            Bit(SyntaxNodeClass::SourceItemScope)
    ),
    Tree(
        SyntaxNodeKind::FieldDeclarationList,
        "field_declaration_list",
        Bit(SyntaxNodeClass::CompoundBlock) |
            kAllowedPreprocessorContainerClasses |
            Bit(SyntaxNodeClass::SourceItemScope) |
            Bit(SyntaxNodeClass::DeclarationScope)
    ),
    Tree(
        SyntaxNodeKind::EnumeratorList,
        "enumerator_list",
        Bit(SyntaxNodeClass::CompoundBlock) |
            kAllowedListPreprocessorContainerClasses |
            Bit(SyntaxNodeClass::SourceItemScope)
    ),
    Tree(
        SyntaxNodeKind::InitializerList,
        "initializer_list",
        kPreprocessorSplitListClasses | Bit(SyntaxNodeClass::NamedList)
    ),
    Tree(
        SyntaxNodeKind::FieldInitializerList,
        "field_initializer_list",
        Bit(SyntaxNodeClass::PrefixList) | Bit(SyntaxNodeClass::SemanticDelimitedParent)
    ),
    Tree(SyntaxNodeKind::FieldInitializer, "field_initializer"),
    Tree(
        SyntaxNodeKind::DeclarationList,
        "declaration_list",
        Bit(SyntaxNodeClass::CompoundBlock) |
            kAllowedPreprocessorContainerClasses |
            Bit(SyntaxNodeClass::SourceItemScope) |
            Bit(SyntaxNodeClass::DeclarationScope)
    ),
    Tree(
        SyntaxNodeKind::DeclarationList,
        "namespace_declaration_list",
        Bit(SyntaxNodeClass::CompoundBlock) |
            kAllowedPreprocessorContainerClasses |
            Bit(SyntaxNodeClass::SourceItemScope) |
            Bit(SyntaxNodeClass::DeclarationScope)
    ),
    Tree(SyntaxNodeKind::NamespaceDefinition, "namespace_definition"),
    Tree(SyntaxNodeKind::LinkageSpecification, "linkage_specification"),
    Tree(
        SyntaxNodeKind::EnumSpecifier,
        "enum_specifier",
        Bit(SyntaxNodeClass::MacroDeclarationFragment) | Bit(SyntaxNodeClass::DeclaredTypeSpecifier)
    ),
    Tree(
        SyntaxNodeKind::ClassSpecifier,
        "class_specifier",
        Bit(SyntaxNodeClass::MacroDeclarationFragment) | Bit(SyntaxNodeClass::DeclaredTypeSpecifier)
    ),
    Tree(
        SyntaxNodeKind::StructSpecifier,
        "struct_specifier",
        Bit(SyntaxNodeClass::MacroDeclarationFragment) | Bit(SyntaxNodeClass::DeclaredTypeSpecifier)
    ),
    Tree(
        SyntaxNodeKind::UnionSpecifier,
        "union_specifier",
        Bit(SyntaxNodeClass::MacroDeclarationFragment) | Bit(SyntaxNodeClass::DeclaredTypeSpecifier)
    ),
    Tree(SyntaxNodeKind::BaseClassClause, "base_class_clause", Bit(SyntaxNodeClass::PrefixList)),
    Tree(SyntaxNodeKind::AccessSpecifier, "access_specifier"),
    Tree(SyntaxNodeKind::AccessSpecifier, "access_specifier_label"),
    Tree(
        SyntaxNodeKind::IfStatement,
        "if_statement",
        Bit(SyntaxNodeClass::ControlHeader) | Bit(SyntaxNodeClass::FlatLogicalHeader)
    ),
    Tree(SyntaxNodeKind::ElseClause, "else_clause"),
    Tree(SyntaxNodeKind::ForStatement, "for_statement", Bit(SyntaxNodeClass::ControlHeader)),
    Tree(SyntaxNodeKind::ForStatement, "for_range_loop", Bit(SyntaxNodeClass::ControlHeader)),
    Tree(SyntaxNodeKind::ForStatement, "for_each_statement", Bit(SyntaxNodeClass::ControlHeader)),
    Tree(
        SyntaxNodeKind::WhileStatement,
        "while_statement",
        Bit(SyntaxNodeClass::ControlHeader) | Bit(SyntaxNodeClass::FlatLogicalHeader)
    ),
    Tree(SyntaxNodeKind::DoStatement, "do_statement"),
    Tree(
        SyntaxNodeKind::SwitchStatement,
        "switch_statement",
        Bit(SyntaxNodeClass::ControlHeader) | Bit(SyntaxNodeClass::FlatLogicalHeader)
    ),
    Tree(SyntaxNodeKind::CaseStatement, "case_statement"),
    Tree(SyntaxNodeKind::ReturnStatement, "return_statement"),
    Tree(SyntaxNodeKind::CoReturnStatement, "co_return_statement"),
    Tree(
        SyntaxNodeKind::ConditionClause,
        "condition_clause",
        Bit(SyntaxNodeClass::ControlHeader) |
            Bit(SyntaxNodeClass::FlatLogicalHeader) |
            Bit(SyntaxNodeClass::SemanticDelimitedParent)
    ),
    Tree(SyntaxNodeKind::InitStatement, "init_statement"),
    Tree(SyntaxNodeKind::PreprocCall, "preproc_call", kAtomicPreprocessorClasses),
    Tree(SyntaxNodeKind::MacroDefinition, "preproc_def", Bit(SyntaxNodeClass::MacroDefinition)),
    Tree(SyntaxNodeKind::MacroDefinition, "preproc_function_def", Bit(SyntaxNodeClass::MacroDefinition)),
    Tree(
        SyntaxNodeKind::PreprocInclude,
        "preproc_include",
        kAtomicPreprocessorClasses |
            Bit(SyntaxNodeClass::IncludeDirective) |
            Bit(SyntaxNodeClass::CheckedPreprocessorDirective)
    ),
    Tree(
        SyntaxNodeKind::PreprocIf,
        "preproc_if",
        kConditionalPreprocessorOpenClasses | Bit(SyntaxNodeClass::SourceItemScope)
    ),
    Tree(
        SyntaxNodeKind::PreprocIfdef,
        "preproc_ifdef",
        kConditionalPreprocessorOpenClasses | Bit(SyntaxNodeClass::SourceItemScope)
    ),
    Tree(
        SyntaxNodeKind::PreprocElse,
        "preproc_else",
        kConditionalPreprocessorTreeClasses | Bit(SyntaxNodeClass::ConditionalBranchSeparatorDirective)
    ),
    Tree(
        SyntaxNodeKind::PreprocElif,
        "preproc_elif",
        kConditionalPreprocessorTreeClasses | Bit(SyntaxNodeClass::ConditionalBranchSeparatorDirective)
    ),
    Tree(SyntaxNodeKind::PreprocUsing, "preproc_using", kAtomicPreprocessorClasses),
    Tree(SyntaxNodeKind::PreprocParams, "preproc_params"),
    Tree(SyntaxNodeKind::PreprocArg, "preproc_arg"),
    Tree(SyntaxNodeKind::RawMacroReplacement, "raw_macro_replacement", Bit(SyntaxNodeClass::OpaqueSource)),
    Tree(SyntaxNodeKind::Tree, "macro_arrow_chain"),
    Tree(SyntaxNodeKind::Tree, "top_level_call_statement"),
    Tree(SyntaxNodeKind::BareMacroItem, "top_level_item_macro"),
    Tree(SyntaxNodeKind::BareMacroItem, "bare_macro_statement"),
    Tree(SyntaxNodeKind::MacroCallItem, "block_macro_call_line_item"),
    Tree(SyntaxNodeKind::MacroCallItem, "block_macro_call_statement_item"),
    Tree(SyntaxNodeKind::MacroCallItem, "top_level_macro_call_line_item"),
    Tree(SyntaxNodeKind::MacroCallItem, "macro_call_item"),
    Tree(SyntaxNodeKind::MacroCallItem, "macro_call_replacement_item"),
    Tree(SyntaxNodeKind::Tree, "function_pointer_type_descriptor"),
    Tree(SyntaxNodeKind::Tree, "type_specifier_macro_call"),
    Tree(SyntaxNodeKind::Tree, "preprocessing_token_macro_call"),
    Tree(SyntaxNodeKind::ArgumentList, "preprocessing_token_argument_list", kPreprocessingTokenArgumentListClasses),
    Tree(SyntaxNodeKind::Tree, "preprocessing_token_argument"),
    Tree(SyntaxNodeKind::Tree, "preprocessing_parenthesized_tokens", Bit(SyntaxNodeClass::PreserveTrailingComma)),
    Tree(SyntaxNodeKind::Tree, "macro_expression_without_semicolon"),
    Tree(SyntaxNodeKind::Tree, "macro_token_paste_expression"),
    Tree(SyntaxNodeKind::Tree, "macro_preprocessing_token_sequence_argument"),
    Tree(SyntaxNodeKind::Tree, "macro_preprocessing_token_call"),
    Tree(SyntaxNodeKind::Tree, "disabled_code_placeholder_statement"),
    Tree(SyntaxNodeKind::Tree, "disabled_code_placeholder_field"),
    Tree(SyntaxNodeKind::Tree, "throw_expression"),
    Tree(SyntaxNodeKind::Tree, "typeid_expression"),
    Tree(SyntaxNodeKind::Tree, "cpp_cast_expression"),
    Tree(SyntaxNodeKind::Tree, "functional_cast_type_specifier"),
    Tree(
        SyntaxNodeKind::PreprocIf,
        "preproc_selected_else_if_statement",
        kAtomicPreprocessorClasses | kSupportedPreprocessorPlacementClasses
    ),
    Tree(
        SyntaxNodeKind::PreprocIf,
        "preproc_selected_else_if_clause",
        kAtomicPreprocessorClasses | kSupportedPreprocessorPlacementClasses
    ),
    Tree(SyntaxNodeKind::PreprocIf, "preproc_selected_braced_if_else_statement", kAtomicPreprocessorClasses),
    Tree(
        SyntaxNodeKind::PreprocIf,
        "preproc_selected_if_header",
        kAtomicPreprocessorClasses | kSupportedPreprocessorPlacementClasses
    ),
    Tree(SyntaxNodeKind::PreprocIf, "standalone_qualifier_preproc_if", kDeclarationModifierPreprocessorClasses),
    Tree(SyntaxNodeKind::PreprocIf, "standalone_attribute_preproc_if", kDeclarationModifierPreprocessorClasses),
    Tree(
        SyntaxNodeKind::PreprocIfdef,
        "declaration_suffix_preproc_ifdef",
        kAtomicPreprocessorClasses | kSupportedPreprocessorPlacementClasses
    ),
    Tree(
        SyntaxNodeKind::PreprocIfdef,
        "conditional_extern_c_open",
        kAtomicPreprocessorClasses | kSupportedPreprocessorPlacementClasses
    ),
    Tree(
        SyntaxNodeKind::PreprocIfdef,
        "conditional_extern_c_close",
        kAtomicPreprocessorClasses | kSupportedPreprocessorPlacementClasses
    ),
    Tree(SyntaxNodeKind::PreprocIf, "preproc_logical_expression_fragment", kAtomicPreprocessorClasses),
    Tree(SyntaxNodeKind::PreprocIf, "preproc_logical_tail_expression_fragment", kAtomicPreprocessorClasses),
    Tree(SyntaxNodeKind::PreprocIf, "preproc_condition_expression", kAtomicPreprocessorClasses),
    Tree(
        SyntaxNodeKind::PreprocIf,
        "preproc_case_label_fragment",
        kAtomicPreprocessorClasses | kSupportedPreprocessorPlacementClasses
    ),
    Tree(SyntaxNodeKind::PreprocIf, "preproc_semicolon_initializer", kConditionalRhsPreprocessorClasses),
    Tree(SyntaxNodeKind::PreprocIf, "preproc_template_argument_fragment", kAtomicPreprocessorClasses),
    Tree(SyntaxNodeKind::PreprocIfdef, "preproc_argument_fragment", kAtomicPreprocessorClasses),
    Tree(SyntaxNodeKind::PreprocIf, "preproc_if_argument_fragment", kAtomicPreprocessorClasses),
    Tree(SyntaxNodeKind::Tree, "preproc_trailing_argument_expression"),
    Tree(
        SyntaxNodeKind::PreprocIf,
        "preproc_string_literal_fragment",
        kAtomicPreprocessorClasses | kSupportedPreprocessorPlacementClasses
    ),
    Tree(
        SyntaxNodeKind::PreprocIf,
        "preproc_if_in_macro_function_definition_prefix",
        kSupportedPreprocessorPlacementClasses | Bit(SyntaxNodeClass::ConditionalFunctionHeader)
    ),
    Tree(
        SyntaxNodeKind::PreprocElse,
        "preproc_else_in_macro_function_definition_prefix",
        kSupportedPreprocessorPlacementClasses | Bit(SyntaxNodeClass::ConditionalFunctionHeader)
    ),
    Tree(
        SyntaxNodeKind::PreprocIf,
        "preproc_if_in_function_definition_prefix",
        kAllowedPreprocessorContainerClasses |
            kSupportedPreprocessorPlacementClasses |
            Bit(SyntaxNodeClass::ConditionalFunctionHeader)
    ),
    Tree(
        SyntaxNodeKind::PreprocIfdef,
        "preproc_ifdef_in_function_definition_prefix",
        kAllowedPreprocessorContainerClasses |
            kSupportedPreprocessorPlacementClasses |
            Bit(SyntaxNodeClass::ConditionalFunctionHeader)
    ),
    Tree(
        SyntaxNodeKind::PreprocElse,
        "preproc_else_in_function_definition_prefix",
        kAllowedPreprocessorContainerClasses |
            kSupportedPreprocessorPlacementClasses |
            Bit(SyntaxNodeClass::ConditionalFunctionHeader)
    ),
    Tree(
        SyntaxNodeKind::BinaryExpression,
        "stream_operator_chain_suffix",
        Bit(SyntaxNodeClass::LeadingStreamOperatorChain)
    ),
    Tree(SyntaxNodeKind::PreprocElif, "preproc_elifdef"),
    Tree(SyntaxNodeKind::BinaryExpression, "binary_expression"),
    Tree(SyntaxNodeKind::BinaryExpression, "constraint_conjunction"),
    Tree(SyntaxNodeKind::BinaryExpression, "constraint_disjunction"),
    Tree(SyntaxNodeKind::UnaryExpression, "unary_expression"),
    Tree(SyntaxNodeKind::UnaryExpression, "pointer_expression"),
    Tree(SyntaxNodeKind::UnaryExpression, "qualified_address_expression"),
    Tree(SyntaxNodeKind::ConditionalExpression, "conditional_expression"),
    Tree(SyntaxNodeKind::CommaExpression, "comma_expression"),
    Tree(SyntaxNodeKind::AssignmentExpression, "assignment_expression"),
    Tree(SyntaxNodeKind::AssignmentExpression, "preproc_assignment_statement"),
    Tree(SyntaxNodeKind::InitDeclarator, "init_declarator", kAllowedPreprocessorContainerClasses),
    Tree(SyntaxNodeKind::CastExpression, "cast_expression"),
    Tree(SyntaxNodeKind::PointerDeclarator, "pointer_declarator", Bit(SyntaxNodeClass::DeclaratorReferenceParent)),
    Tree(
        SyntaxNodeKind::AbstractPointerDeclarator,
        "abstract_pointer_declarator",
        Bit(SyntaxNodeClass::DeclaratorReferenceParent)
    ),
    Tree(SyntaxNodeKind::ReferenceDeclarator, "reference_declarator", Bit(SyntaxNodeClass::DeclaratorReferenceParent)),
    Tree(
        SyntaxNodeKind::AbstractReferenceDeclarator,
        "abstract_reference_declarator",
        Bit(SyntaxNodeClass::DeclaratorReferenceParent)
    ),
    Tree(SyntaxNodeKind::HandleDeclarator, "handle_declarator", Bit(SyntaxNodeClass::DeclaratorReferenceParent)),
    Tree(
        SyntaxNodeKind::AbstractHandleDeclarator,
        "abstract_handle_declarator",
        Bit(SyntaxNodeClass::DeclaratorReferenceParent)
    ),
    Tree(
        SyntaxNodeKind::MemberPointerDeclarator,
        "member_pointer_declarator",
        Bit(SyntaxNodeClass::DeclaratorReferenceParent) | Bit(SyntaxNodeClass::QualifiedName)
    ),
    Tree(SyntaxNodeKind::Tree, "abstract_member_pointer_declarator", Bit(SyntaxNodeClass::QualifiedName)),
    Tree(SyntaxNodeKind::FunctionDeclarator, "function_declarator"),
    Tree(SyntaxNodeKind::AbstractFunctionDeclarator, "abstract_function_declarator"),
    Tree(
        SyntaxNodeKind::ParenthesizedDeclarator,
        "parenthesized_declarator",
        Bit(SyntaxNodeClass::ParenthesizedDeclarator) | Bit(SyntaxNodeClass::SemanticDelimitedParent)
    ),
    Tree(
        SyntaxNodeKind::AbstractParenthesizedDeclarator,
        "abstract_parenthesized_declarator",
        Bit(SyntaxNodeClass::ParenthesizedDeclarator) | Bit(SyntaxNodeClass::SemanticDelimitedParent)
    ),
    Tree(
        SyntaxNodeKind::ParameterList, "parameter_list", kPreprocessorSplitListClasses | Bit(SyntaxNodeClass::NamedList)
    ),
    Tree(SyntaxNodeKind::ParameterList, "macro_method_parameter_list"),
    Tree(
        SyntaxNodeKind::ArgumentList, "argument_list", kPreprocessorSplitListClasses | Bit(SyntaxNodeClass::NamedList)
    ),
    Tree(SyntaxNodeKind::ArgumentList, "primitive_braced_argument_list", kPreprocessorSplitListClasses),
    Tree(SyntaxNodeKind::ArgumentList, "macro_argument_list", kPreprocessorSplitListClasses),
    Tree(SyntaxNodeKind::ArgumentList, "macro_statement_argument_list", kPreprocessorSplitListClasses),
    Tree(SyntaxNodeKind::MacroStatementSequence, "macro_statement_sequence_argument"),
    Tree(SyntaxNodeKind::MacroStatementSequence, "structured_statement_macro_argument"),
    Tree(SyntaxNodeKind::SubscriptArgumentList, "subscript_argument_list", kPreprocessorSplitListClasses),
    Tree(SyntaxNodeKind::TemplateParameterList, "template_parameter_list", kPreprocessorSplitListClasses),
    Tree(SyntaxNodeKind::TemplateArgumentList, "template_argument_list", kPreprocessorSplitListClasses),
    Tree(SyntaxNodeKind::TemplateDeclaration, "template_declaration", Bit(SyntaxNodeClass::MacroDeclarationFragment)),
    Tree(SyntaxNodeKind::TemplateInstantiation, "template_instantiation"),
    Tree(SyntaxNodeKind::RequiresClause, "requires_clause"),
    Tree(SyntaxNodeKind::RequiresExpression, "requires_expression"),
    Tree(SyntaxNodeKind::RequirementSeq, "requirement_seq", Bit(SyntaxNodeClass::CompoundBlock)),
    Tree(SyntaxNodeKind::NestedRequirement, "nested_requirement"),
    Tree(SyntaxNodeKind::RefQualifier, "ref_qualifier"),
    Tree(SyntaxNodeKind::LambdaExpression, "lambda_expression"),
    Tree(SyntaxNodeKind::LambdaCaptureSpecifier, "lambda_capture_specifier"),
    Tree(SyntaxNodeKind::StructuredBindingDeclarator, "structured_binding_declarator"),
    Tree(SyntaxNodeKind::Tree, "structured_binding_pack_identifier"),
    Tree(SyntaxNodeKind::SpliceSpecifier, "splice_specifier", Bit(SyntaxNodeClass::SemanticDelimitedParent)),
    Tree(SyntaxNodeKind::UnaryExpression, "reflect_expression"),
    Tree(SyntaxNodeKind::FieldDesignator, "field_designator"),
    Tree(SyntaxNodeKind::FieldExpression, "field_expression"),
    Tree(SyntaxNodeKind::TrailingReturnType, "trailing_return_type"),
    Tree(SyntaxNodeKind::OperatorName, "operator_name"),
    Tree(SyntaxNodeKind::OperatorCast, "operator_cast"),
    Tree(SyntaxNodeKind::LabeledStatement, "labeled_statement"),
    Tree(SyntaxNodeKind::AttributeSpecifier, "attribute_specifier"),
    Tree(SyntaxNodeKind::AttributeDeclaration, "attribute_declaration"),
    Tree(SyntaxNodeKind::Attribute, "attribute"),
    Tree(SyntaxNodeKind::AttributedStatement, "attributed_statement"),
    Tree(SyntaxNodeKind::Tree, "preproc_declaration_modifier", Bit(SyntaxNodeClass::DeclarationModifierPreprocessor)),
    Tree(SyntaxNodeKind::MsCallModifier, "ms_call_modifier"),
    Tree(SyntaxNodeKind::MsDeclspecModifier, "ms_declspec_modifier"),
    Tree(SyntaxNodeKind::FunctionSuffixMacro, "function_suffix_macro"),
    Tree(SyntaxNodeKind::FunctionSuffixMacro, "alias_suffix_macro"),
    Tree(SyntaxNodeKind::PureVirtualClause, "pure_virtual_clause"),
    Tree(SyntaxNodeKind::Tree, "virtual_specifier"),
    Tree(SyntaxNodeKind::Identifier, "macro_initializer"),
    Tree(
        SyntaxNodeKind::ConcatenatedString,
        "concatenated_string",
        Bit(SyntaxNodeClass::Literal) | Bit(SyntaxNodeClass::StringLike)
    ),
    Tree(SyntaxNodeKind::UserDefinedLiteral, "user_defined_literal", Bit(SyntaxNodeClass::Literal)),
    Tree(SyntaxNodeKind::StringLiteral, "suffixed_string_literal", kStringLikeClasses),
    Tree(SyntaxNodeKind::RawStringLiteral, "raw_string_literal", kStringLikeClasses),
    Tree(SyntaxNodeKind::StringLiteral, "string_literal", kStringLikeClasses),
    Tree(SyntaxNodeKind::SystemLibString, "system_lib_string", kStringLikeClasses),
    Tree(SyntaxNodeKind::CharacterLiteral, "char_literal", kStringLikeClasses),
    Tree(SyntaxNodeKind::NumberLiteral, "number_literal", kNumberLiteralClasses),
    Tree(SyntaxNodeKind::NumberLiteral, "pure_virtual_zero", kNumberLiteralClasses),
    Tree(SyntaxNodeKind::NumberLiteral, "preprocessing_number", kNumberLiteralClasses),
    Tree(SyntaxNodeKind::Identifier, "identifier"),
    Tree(SyntaxNodeKind::Identifier, "bare_macro_identifier"),
    Tree(SyntaxNodeKind::Identifier, "declaration_prefix_macro_identifier"),
    Tree(SyntaxNodeKind::Identifier, "call_syntax_macro_identifier"),
    Tree(SyntaxNodeKind::Identifier, "semicolonless_call_macro_identifier"),
    Tree(SyntaxNodeKind::Identifier, "statement_argument_macro_identifier"),
    Tree(SyntaxNodeKind::Identifier, "type_specifier_macro_identifier"),
    Tree(SyntaxNodeKind::Identifier, "preprocessor_argument_macro_identifier"),
    Tree(SyntaxNodeKind::Identifier, "field_identifier"),
    Tree(SyntaxNodeKind::Identifier, "namespace_identifier"),
    Tree(SyntaxNodeKind::Tree, "nested_namespace_specifier", Bit(SyntaxNodeClass::QualifiedName)),
    Tree(SyntaxNodeKind::Identifier, "type_identifier"),
    Tree(SyntaxNodeKind::Identifier, "qualified_identifier", Bit(SyntaxNodeClass::QualifiedName)),
    Tree(SyntaxNodeKind::Identifier, "macro_qualified_identifier", Bit(SyntaxNodeClass::QualifiedName)),
    Token(
        SyntaxNodeKind::PreprocessorDirectiveInclude,
        "#include",
        kCheckedPreprocessorDirectiveClasses | Bit(SyntaxNodeClass::IncludeDirective)
    ),
    Token(SyntaxNodeKind::PreprocessorDirectiveDefine, "#define", kPreprocessorDirectiveClasses),
    Token(SyntaxNodeKind::PreprocessorDirectiveIf, "#if", kConditionalOpeningDirectiveClasses),
    Token(SyntaxNodeKind::PreprocessorDirectiveIfdef, "#ifdef", kConditionalOpeningDirectiveClasses),
    Token(SyntaxNodeKind::PreprocessorDirectiveIfndef, "#ifndef", kConditionalOpeningDirectiveClasses),
    Token(SyntaxNodeKind::PreprocessorDirectiveElif, "#elif", kConditionalBranchSeparatorDirectiveClasses),
    Token(SyntaxNodeKind::PreprocessorDirectiveElifdef, "#elifdef", kConditionalBranchSeparatorDirectiveClasses),
    Token(SyntaxNodeKind::PreprocessorDirectiveElifndef, "#elifndef", kConditionalBranchSeparatorDirectiveClasses),
    Token(SyntaxNodeKind::PreprocessorDirectiveElse, "#else", kConditionalBranchSeparatorDirectiveClasses),
    Token(SyntaxNodeKind::PreprocessorDirectiveEndif, "#endif", kEndifDirectiveClasses),
    Token(SyntaxNodeKind::PreprocessorDirectiveUndef, "#undef", kPreprocessorDirectiveClasses),
    Token(SyntaxNodeKind::PreprocessorDirectivePragma, "#pragma", kPreprocessorDirectiveClasses),
    Token(SyntaxNodeKind::PreprocessorDirectiveError, "#error", kPreprocessorDirectiveClasses),
    Token(SyntaxNodeKind::PreprocessorDirectiveWarning, "#warning", kPreprocessorDirectiveClasses),
    Token(SyntaxNodeKind::PreprocessorDirectiveLine, "#line", kPreprocessorDirectiveClasses),
    Token(SyntaxNodeKind::PreprocessorDirectiveUsing, "#using", kPreprocessorDirectiveClasses),
    Token(SyntaxNodeKind::Hash, "#"),
    Token(SyntaxNodeKind::LeftParen, "(", Bit(SyntaxNodeClass::OpeningDelimiter)),
    Token(SyntaxNodeKind::RightParen, ")"),
    Token(SyntaxNodeKind::LeftBracket, "[", Bit(SyntaxNodeClass::OpeningDelimiter)),
    Token(SyntaxNodeKind::RightBracket, ":]"),
    Token(SyntaxNodeKind::RightBracket, "]"),
    Token(SyntaxNodeKind::LeftBrace, "{", Bit(SyntaxNodeClass::OpeningDelimiter)),
    Token(SyntaxNodeKind::RightBrace, "}"),
    Token(SyntaxNodeKind::Less, "<", Bit(SyntaxNodeClass::BinaryOperator) | Bit(SyntaxNodeClass::OpeningDelimiter)),
    Token(SyntaxNodeKind::Greater, ">", Bit(SyntaxNodeClass::BinaryOperator)),
    Token(SyntaxNodeKind::LessEqual, "<=", Bit(SyntaxNodeClass::BinaryOperator)),
    Token(SyntaxNodeKind::GreaterEqual, ">=", Bit(SyntaxNodeClass::BinaryOperator)),
    Token(SyntaxNodeKind::EqualEqual, "==", Bit(SyntaxNodeClass::BinaryOperator)),
    Token(SyntaxNodeKind::BangEqual, "!=", Bit(SyntaxNodeClass::BinaryOperator)),
    Token(SyntaxNodeKind::Spaceship, "<=>", Bit(SyntaxNodeClass::BinaryOperator)),
    Token(SyntaxNodeKind::Plus, "+", kChainBinaryClasses | Bit(SyntaxNodeClass::UnaryOperator)),
    Token(SyntaxNodeKind::Minus, "-", Bit(SyntaxNodeClass::BinaryOperator) | Bit(SyntaxNodeClass::UnaryOperator)),
    Token(
        SyntaxNodeKind::Star,
        "*",
        kChainBinaryClasses | Bit(SyntaxNodeClass::UnaryOperator) | Bit(SyntaxNodeClass::DeclaratorReferenceToken)
    ),
    Token(SyntaxNodeKind::Slash, "/", Bit(SyntaxNodeClass::BinaryOperator)),
    Token(
        SyntaxNodeKind::Percent,
        "%",
        Bit(SyntaxNodeClass::BinaryOperator) | Bit(SyntaxNodeClass::DeclaratorReferenceToken)
    ),
    Token(SyntaxNodeKind::Caret, "^", kChainBinaryClasses | Bit(SyntaxNodeClass::DeclaratorReferenceToken)),
    Token(SyntaxNodeKind::ReflectOperator, "^^", Bit(SyntaxNodeClass::UnaryOperator)),
    Token(
        SyntaxNodeKind::Ampersand,
        "&",
        kChainBinaryClasses | Bit(SyntaxNodeClass::UnaryOperator) | Bit(SyntaxNodeClass::DeclaratorReferenceToken)
    ),
    Token(SyntaxNodeKind::Pipe, "|", kChainBinaryClasses),
    Token(SyntaxNodeKind::Bang, "!", Bit(SyntaxNodeClass::UnaryOperator)),
    Token(SyntaxNodeKind::Tilde, "~", Bit(SyntaxNodeClass::UnaryOperator)),
    Token(SyntaxNodeKind::Equal, "=", Bit(SyntaxNodeClass::AssignmentOperator)),
    Token(SyntaxNodeKind::PlusEqual, "+=", Bit(SyntaxNodeClass::AssignmentOperator)),
    Token(SyntaxNodeKind::MinusEqual, "-=", Bit(SyntaxNodeClass::AssignmentOperator)),
    Token(SyntaxNodeKind::StarEqual, "*=", Bit(SyntaxNodeClass::AssignmentOperator)),
    Token(SyntaxNodeKind::SlashEqual, "/=", Bit(SyntaxNodeClass::AssignmentOperator)),
    Token(SyntaxNodeKind::PercentEqual, "%=", Bit(SyntaxNodeClass::AssignmentOperator)),
    Token(SyntaxNodeKind::CaretEqual, "^=", Bit(SyntaxNodeClass::AssignmentOperator)),
    Token(SyntaxNodeKind::AmpersandEqual, "&=", Bit(SyntaxNodeClass::AssignmentOperator)),
    Token(SyntaxNodeKind::PipeEqual, "|=", Bit(SyntaxNodeClass::AssignmentOperator)),
    Token(SyntaxNodeKind::LessLess, "<<", kChainBinaryClasses),
    Token(SyntaxNodeKind::GreaterGreater, ">>", kChainBinaryClasses),
    Token(SyntaxNodeKind::LessLessEqual, "<<=", Bit(SyntaxNodeClass::AssignmentOperator)),
    Token(SyntaxNodeKind::GreaterGreaterEqual, ">>=", Bit(SyntaxNodeClass::AssignmentOperator)),
    Token(
        SyntaxNodeKind::AmpersandAmpersand, "&&", kChainBinaryClasses | Bit(SyntaxNodeClass::DeclaratorReferenceToken)
    ),
    Token(SyntaxNodeKind::AmpersandAmpersand, "and", kChainBinaryClasses),
    Token(SyntaxNodeKind::PipePipe, "||", kChainBinaryClasses),
    Token(SyntaxNodeKind::PipePipe, "or", kChainBinaryClasses),
    Token(SyntaxNodeKind::PlusPlus, "++", Bit(SyntaxNodeClass::UnaryOperator)),
    Token(SyntaxNodeKind::MinusMinus, "--", Bit(SyntaxNodeClass::UnaryOperator)),
    Token(SyntaxNodeKind::Arrow, "->", Bit(SyntaxNodeClass::MemberOperator)),
    Token(SyntaxNodeKind::Dot, ".", Bit(SyntaxNodeClass::MemberOperator)),
    Token(SyntaxNodeKind::ArrowStar, "->*", Bit(SyntaxNodeClass::MemberOperator)),
    Token(SyntaxNodeKind::DotStar, ".*", Bit(SyntaxNodeClass::MemberOperator)),
    Token(SyntaxNodeKind::ColonColon, "::", Bit(SyntaxNodeClass::MemberOperator)),
    Token(SyntaxNodeKind::Question, "?"),
    Token(SyntaxNodeKind::Colon, ":"),
    Token(SyntaxNodeKind::Semicolon, ";"),
    Token(SyntaxNodeKind::Comma, ",", Bit(SyntaxNodeClass::ChainOperator)),
    Token(SyntaxNodeKind::Ellipsis, "..."),
    Keyword(SyntaxNodeKind::KeywordAlignas, "alignas"),
    Keyword(SyntaxNodeKind::KeywordAlignof, "alignof"),
    Keyword(SyntaxNodeKind::KeywordAsm, "asm"),
    Keyword(SyntaxNodeKind::KeywordAuto, "auto"),
    Keyword(SyntaxNodeKind::KeywordBool, "bool"),
    Keyword(SyntaxNodeKind::KeywordBreak, "break"),
    Keyword(SyntaxNodeKind::KeywordCase, "case"),
    Keyword(
        SyntaxNodeKind::KeywordCatch,
        "catch",
        Bit(SyntaxNodeClass::ControlKeyword) | Bit(SyntaxNodeClass::AttachAfterBlockKeyword)
    ),
    Keyword(SyntaxNodeKind::KeywordChar, "char"),
    Keyword(SyntaxNodeKind::KeywordChar16T, "char16_t"),
    Keyword(SyntaxNodeKind::KeywordChar32T, "char32_t"),
    Keyword(SyntaxNodeKind::KeywordClass, "class"),
    Keyword(SyntaxNodeKind::KeywordConcept, "concept"),
    Keyword(SyntaxNodeKind::KeywordConst, "const"),
    Keyword(SyntaxNodeKind::KeywordConsteval, "consteval"),
    Keyword(SyntaxNodeKind::KeywordConstexpr, "constexpr"),
    Keyword(SyntaxNodeKind::KeywordConstinit, "constinit"),
    Keyword(SyntaxNodeKind::KeywordConstCast, "const_cast"),
    Keyword(SyntaxNodeKind::KeywordContinue, "continue"),
    Keyword(SyntaxNodeKind::KeywordDecltype, "decltype"),
    Keyword(SyntaxNodeKind::KeywordDefault, "default"),
    Keyword(SyntaxNodeKind::KeywordDelete, "delete"),
    Keyword(SyntaxNodeKind::KeywordDo, "do"),
    Keyword(SyntaxNodeKind::KeywordDouble, "double"),
    Keyword(SyntaxNodeKind::KeywordDynamicCast, "dynamic_cast"),
    Keyword(SyntaxNodeKind::KeywordElse, "else", Bit(SyntaxNodeClass::AttachAfterBlockKeyword)),
    Keyword(SyntaxNodeKind::KeywordEnum, "enum"),
    Keyword(SyntaxNodeKind::KeywordExplicit, "explicit"),
    Keyword(SyntaxNodeKind::KeywordExport, "export"),
    Keyword(SyntaxNodeKind::KeywordExtern, "extern"),
    Keyword(SyntaxNodeKind::KeywordFalse, "false", Bit(SyntaxNodeClass::Literal)),
    Keyword(SyntaxNodeKind::KeywordFinal, "final"),
    Keyword(SyntaxNodeKind::KeywordFinally, "finally", Bit(SyntaxNodeClass::AttachAfterBlockKeyword)),
    Keyword(SyntaxNodeKind::KeywordFloat, "float"),
    Keyword(SyntaxNodeKind::KeywordFor, "for", Bit(SyntaxNodeClass::ControlKeyword)),
    Keyword(SyntaxNodeKind::KeywordFriend, "friend"),
    Keyword(SyntaxNodeKind::KeywordGoto, "goto"),
    Keyword(SyntaxNodeKind::KeywordIf, "if", Bit(SyntaxNodeClass::ControlKeyword)),
    Keyword(SyntaxNodeKind::KeywordInline, "inline"),
    Keyword(SyntaxNodeKind::KeywordInt, "int"),
    Keyword(SyntaxNodeKind::KeywordLong, "long"),
    Keyword(SyntaxNodeKind::KeywordMutable, "mutable"),
    Keyword(SyntaxNodeKind::KeywordNamespace, "namespace"),
    Keyword(SyntaxNodeKind::KeywordNew, "new"),
    Keyword(SyntaxNodeKind::KeywordNoexcept, "noexcept"),
    Keyword(SyntaxNodeKind::KeywordNullptr, "nullptr", Bit(SyntaxNodeClass::Literal)),
    Keyword(SyntaxNodeKind::KeywordOperator, "operator"),
    Keyword(SyntaxNodeKind::KeywordOverride, "override"),
    Keyword(SyntaxNodeKind::KeywordPrivate, "private", Bit(SyntaxNodeClass::AccessKeyword)),
    Keyword(SyntaxNodeKind::KeywordProtected, "protected", Bit(SyntaxNodeClass::AccessKeyword)),
    Keyword(SyntaxNodeKind::KeywordPublic, "public", Bit(SyntaxNodeClass::AccessKeyword)),
    Keyword(SyntaxNodeKind::KeywordRegister, "register"),
    Keyword(SyntaxNodeKind::KeywordReinterpretCast, "reinterpret_cast"),
    Keyword(SyntaxNodeKind::KeywordRequires, "requires"),
    Keyword(SyntaxNodeKind::KeywordReturn, "return", Bit(SyntaxNodeClass::KeywordOwnedValue)),
    Keyword(SyntaxNodeKind::KeywordShort, "short"),
    Keyword(SyntaxNodeKind::KeywordSigned, "signed"),
    Keyword(SyntaxNodeKind::KeywordSizeof, "sizeof"),
    Keyword(SyntaxNodeKind::KeywordStatic, "static"),
    Keyword(SyntaxNodeKind::KeywordStaticAssert, "static_assert"),
    Keyword(SyntaxNodeKind::KeywordStaticCast, "static_cast"),
    Keyword(SyntaxNodeKind::KeywordStruct, "struct"),
    Keyword(SyntaxNodeKind::KeywordSwitch, "switch", Bit(SyntaxNodeClass::ControlKeyword)),
    Keyword(SyntaxNodeKind::KeywordTemplate, "template"),
    Keyword(SyntaxNodeKind::KeywordThis, "this"),
    Keyword(SyntaxNodeKind::KeywordThreadLocal, "thread_local"),
    Keyword(SyntaxNodeKind::KeywordThrow, "throw", Bit(SyntaxNodeClass::KeywordOwnedValue)),
    Keyword(SyntaxNodeKind::KeywordTrue, "true", Bit(SyntaxNodeClass::Literal)),
    Keyword(SyntaxNodeKind::KeywordTry, "try"),
    Keyword(SyntaxNodeKind::KeywordTypedef, "typedef"),
    Keyword(SyntaxNodeKind::KeywordTypeid, "typeid"),
    Keyword(SyntaxNodeKind::KeywordTypename, "typename"),
    Keyword(SyntaxNodeKind::KeywordUnion, "union"),
    Keyword(SyntaxNodeKind::KeywordUnsigned, "unsigned"),
    Keyword(SyntaxNodeKind::KeywordUsing, "using"),
    Keyword(SyntaxNodeKind::KeywordVirtual, "virtual"),
    Keyword(SyntaxNodeKind::KeywordVoid, "void"),
    Keyword(SyntaxNodeKind::KeywordVolatile, "volatile"),
    Keyword(SyntaxNodeKind::KeywordWcharT, "wchar_t"),
    Keyword(
        SyntaxNodeKind::KeywordWhile,
        "while",
        Bit(SyntaxNodeClass::ControlKeyword) | Bit(SyntaxNodeClass::AttachAfterBlockKeyword)
    ),
    Keyword(SyntaxNodeKind::KeywordCdecl, "__cdecl"),
    Keyword(SyntaxNodeKind::KeywordDeclspec, "__declspec"),
    Keyword(SyntaxNodeKind::KeywordCoAwait, "co_await"),
    Keyword(SyntaxNodeKind::KeywordCoReturn, "co_return", Bit(SyntaxNodeClass::KeywordOwnedValue)),
    Keyword(SyntaxNodeKind::KeywordCoYield, "co_yield", Bit(SyntaxNodeClass::KeywordOwnedValue)),
});

consteval bool HasOnlyRawMacroOpaqueSourceMapping() {
    size_t opaqueCount = 0;
    for (const SyntaxKindMapping& mapping : kSyntaxKindMappings) {
        if ((mapping.classes & Bit(SyntaxNodeClass::OpaqueSource)) == 0) {
            continue;
        }
        ++opaqueCount;
        if (mapping.kind != SyntaxNodeKind::RawMacroReplacement || mapping.treeType != "raw_macro_replacement") {
            return false;
        }
    }
    return opaqueCount == 1;
}

consteval bool HasOnlyLiteralLexicalAtomMappings() {
    for (const SyntaxKindMapping& mapping : kSyntaxKindMappings) {
        if (
            (mapping.classes & Bit(SyntaxNodeClass::LexicalAtom)) != 0 &&
            (mapping.classes & Bit(SyntaxNodeClass::Literal)) == 0
        ) {
            return false;
        }
    }
    return true;
}

static_assert(
    HasOnlyRawMacroOpaqueSourceMapping(),
    "RawMacroDefinitions replacement text is the only permitted opaque source node"
);
static_assert(
    HasOnlyLiteralLexicalAtomMappings(), "Only lexical literals may suppress tree-sitter's internal lexical children"
);

constexpr size_t KindIndex(SyntaxNodeKind kind) { return static_cast<size_t>(kind); }

constexpr size_t kSyntaxNodeKindCount = KindIndex(SyntaxNodeKind::KeywordCoYield) + 1;

constexpr auto BuildSyntaxKindInfoByKind() {
    std::array<SyntaxKindInfo, kSyntaxNodeKindCount> result{};
    for (const SyntaxKindMapping& mapping : kSyntaxKindMappings) {
        SyntaxKindInfo& info = result[KindIndex(mapping.kind)];
        info.classes |= mapping.classes & ~kSymbolLocalClasses;
        if (!mapping.tokenText.empty()) {
            info.tokenText = mapping.tokenText;
        }
    }
    return result;
}

constexpr size_t MaxTokenTextLength() {
    size_t result = 0;
    for (const SyntaxKindMapping& mapping : kSyntaxKindMappings) {
        if (!mapping.tokenText.empty() && mapping.tokenText.size() > result) {
            result = mapping.tokenText.size();
        }
    }
    return result;
}

constexpr auto kSyntaxKindInfoByKind = BuildSyntaxKindInfoByKind();
constexpr size_t kMaxTokenTextLength = MaxTokenTextLength();

const std::unordered_map<std::string_view, SyntaxNodeKind>& SyntaxKindByTreeType() {
    static const std::unordered_map<std::string_view, SyntaxNodeKind> kindsByTreeType = [] {
        std::unordered_map<std::string_view, SyntaxNodeKind> result;
        result.reserve(kSyntaxKindMappings.size());
        for (const SyntaxKindMapping& mapping : kSyntaxKindMappings) {
            if (!mapping.treeType.empty()) {
                result.emplace(mapping.treeType, mapping.kind);
            }
        }
        return result;
    }();
    return kindsByTreeType;
}

const std::unordered_map<std::string_view, SyntaxNodeKind>& SyntaxKindByTokenText() {
    static const std::unordered_map<std::string_view, SyntaxNodeKind> tokens = [] {
        std::unordered_map<std::string_view, SyntaxNodeKind> result;
        result.reserve(kSyntaxKindMappings.size());
        for (const SyntaxKindMapping& mapping : kSyntaxKindMappings) {
            if (!mapping.tokenText.empty()) {
                result.emplace(mapping.tokenText, mapping.kind);
            }
        }
        return result;
    }();
    return tokens;
}

using SymbolInfoTable = std::vector<SyntaxSymbolInfo>;

SymbolInfoTable MakeSymbolInfoTable() { return SymbolInfoTable(ts_language_symbol_count(tree_sitter_cpp())); }

void
    StoreTreeSymbolInfo(SymbolInfoTable& table, std::string_view name, SyntaxNodeKind kind, std::uint64_t classes = 0)
{
    const TSSymbol symbol =
        ts_language_symbol_for_name(tree_sitter_cpp(), name.data(), static_cast<uint32_t>(name.size()), true);
    if (std::string_view(ts_language_symbol_name(tree_sitter_cpp(), symbol)) != name) {
        throw std::logic_error("formatter tree mapping names a missing grammar symbol: " + std::string(name));
    }
    if (static_cast<size_t>(symbol) < table.size()) {
        table[symbol].treeKind = kind;
        table[symbol].classes |= classes;
    }
}

void StoreTokenSymbolInfo(
    SymbolInfoTable& table, std::string_view name, bool isNamed, SyntaxNodeKind kind, std::uint64_t classes = 0
) {
    const TSSymbol symbol =
        ts_language_symbol_for_name(tree_sitter_cpp(), name.data(), static_cast<uint32_t>(name.size()), isNamed);
    if (static_cast<size_t>(symbol) < table.size()) {
        table[symbol].tokenKind = kind;
        table[symbol].classes |= classes;
    }
}

void StoreTreeSymbolClasses(SymbolInfoTable& table, std::string_view name, std::uint64_t classes) {
    const TSSymbol symbol =
        ts_language_symbol_for_name(tree_sitter_cpp(), name.data(), static_cast<uint32_t>(name.size()), true);
    if (std::string_view(ts_language_symbol_name(tree_sitter_cpp(), symbol)) != name) {
        throw std::logic_error("formatter class mapping names a missing grammar symbol: " + std::string(name));
    }
    if (static_cast<size_t>(symbol) < table.size()) {
        table[symbol].classes |= classes;
    }
}

void StoreSymbolInfoRole(SymbolInfoTable& table, std::string_view name, SyntaxWrapperRole role) {
    const TSSymbol symbol =
        ts_language_symbol_for_name(tree_sitter_cpp(), name.data(), static_cast<uint32_t>(name.size()), true);
    if (std::string_view(ts_language_symbol_name(tree_sitter_cpp(), symbol)) != name) {
        throw std::logic_error("formatter wrapper mapping names a missing grammar symbol: " + std::string(name));
    }
    if (static_cast<size_t>(symbol) < table.size()) {
        table[symbol].wrapperRole = role;
    }
}

const SymbolInfoTable& SyntaxInfoBySymbol() {
    static const SymbolInfoTable symbols = [] {
        SymbolInfoTable result = MakeSymbolInfoTable();
        for (const SyntaxKindMapping& mapping : kSyntaxKindMappings) {
            if (!mapping.treeType.empty()) {
                StoreTreeSymbolInfo(result, mapping.treeType, mapping.kind, mapping.classes);
            }
            if (!mapping.tokenText.empty()) {
                StoreTokenSymbolInfo(result, mapping.tokenText, false, mapping.kind, mapping.classes);
                StoreTokenSymbolInfo(result, mapping.tokenText, true, mapping.kind, mapping.classes);
            }
        }
        // Keep the grammar's expression supertype as one formatter category even when
        // individual expression wrappers are flattened in the format model.
        constexpr std::string_view expressionNames[] = {
            "alignof_expression",
            "assignment_expression",
            "binary_expression",
            "call_expression",
            "cast_expression",
            "char_literal",
            "co_await_expression",
            "compound_literal_expression",
            "concatenated_string",
            "conditional_expression",
            "cpp_cast_expression",
            "delete_expression",
            "extension_expression",
            "false",
            "field_expression",
            "fold_expression",
            "gcnew_expression",
            "generic_expression",
            "gnu_asm_expression",
            "identifier",
            "lambda_expression",
            "macro_call_expression",
            "macro_qualified_identifier",
            "new_expression",
            "null",
            "number_literal",
            "offsetof_expression",
            "parameter_pack_expansion",
            "parenthesized_expression",
            "pointer_expression",
            "preprocessing_token_macro_call",
            "qualified_address_expression",
            "qualified_identifier",
            "raw_string_literal",
            "reflect_expression",
            "requires_clause",
            "requires_expression",
            "sizeof_expression",
            "splice_specifier",
            "string_literal",
            "subscript_expression",
            "suffixed_string_literal",
            "template_function",
            "this",
            "throw_expression",
            "true",
            "typeid_expression",
            "unary_expression",
            "update_expression",
            "user_defined_literal",
        };
        for (const std::string_view name : expressionNames) {
            StoreTreeSymbolClasses(result, name, Bit(SyntaxNodeClass::Expression));
        }
        constexpr std::string_view prefixListNames[] =
            {"gnu_asm_output_operand_list", "gnu_asm_input_operand_list", "gnu_asm_clobber_list", "gnu_asm_goto_list"};
        for (const std::string_view name : prefixListNames) {
            StoreTreeSymbolClasses(result, name, Bit(SyntaxNodeClass::PrefixList));
        }
        StoreTreeSymbolInfo(result, "comment", SyntaxNodeKind::Comment, kCommentClasses);

        constexpr std::string_view flattenNames[] = {
            "call_expression",
            "compound_literal_expression",
            "initializer_pair",
            "macro_argument_sequence",
            "optional_parameter_declaration",
            "optional_type_parameter_declaration",
            "parameter_declaration",
            "sized_type_specifier",
            "subscript_expression",
            "template_function",
            "template_method",
            "template_template_parameter_declaration",
            "template_type",
            "type_descriptor",
        };
        constexpr std::string_view wholeTokenNames[] =
            {"null", "placeholder_type_specifier", "primitive_type", "storage_class_specifier", "type_qualifier"};
        for (const std::string_view name : flattenNames) {
            StoreSymbolInfoRole(result, name, SyntaxWrapperRole::Flatten);
        }
        for (const std::string_view name : wholeTokenNames) {
            StoreSymbolInfoRole(result, name, SyntaxWrapperRole::LexicalWrapper);
        }
        return result;
    }();
    return symbols;
}

bool CallableBodyHasDisqualifier(
    const SyntaxNode& node,
    const SyntaxNode& body,
    const SyntaxNode* statement,
    bool inBody = false,
    bool inStatement = false
) {
    inBody = inBody || &node == &body;
    inStatement = inStatement || &node == statement;
    if (
        SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::PreprocessorDirective) ||
        (inBody && SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::Comment)) ||
        (inStatement && SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::CompoundBlock))
    ) {
        return true;
    }
    for (const SyntaxNode* child : node.children) {
        if (child != nullptr && CallableBodyHasDisqualifier(*child, body, statement, inBody, inStatement)) {
            return true;
        }
    }
    return false;
}

bool IsNullItem(const SyntaxNode& node) {
    const SyntaxNode* item = &node;
    while (item->children.size() == 1 && item->children.front() != nullptr) {
        item = item->children.front();
    }
    return item->kind == SyntaxNodeKind::Semicolon;
}

const SyntaxNode* OnlyContentChild(const SyntaxNode& node) {
    const SyntaxNode* contentChild = nullptr;
    for (const SyntaxNode* child : node.children) {
        if (
            child == nullptr ||
            SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::Trivia) ||
            SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::Known) ||
            IsNullItem(*child)
        ) {
            continue;
        }
        if (contentChild != nullptr) {
            return nullptr;
        }
        contentChild = child;
    }
    return contentChild;
}

}  // namespace

SyntaxNode::SyntaxNode(std::pmr::memory_resource* childResource) : children(childResource) {}

FormatModel::FormatModel() : childStorage(std::make_unique<std::pmr::monotonic_buffer_resource>()) {}

SyntaxNodeKind SyntaxNodeKindFromTreeType(std::string_view type) {
    const auto& kindsByTreeType = SyntaxKindByTreeType();
    const auto found = kindsByTreeType.find(type);
    return found == kindsByTreeType.end() ? SyntaxNodeKind::Unknown : found->second;
}

SyntaxNodeKind SyntaxNodeKindFromTokenText(std::string_view text) {
    if (text.size() > kMaxTokenTextLength) {
        return SyntaxNodeKind::Unknown;
    }
    const auto& tokens = SyntaxKindByTokenText();
    const auto found = tokens.find(text);
    return found == tokens.end() ? SyntaxNodeKind::Unknown : found->second;
}

SyntaxNodeKind SyntaxNodeKindFromPreprocessorDirectiveLine(std::string_view line) {
    while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
        line.remove_prefix(1);
    }
    if (line.empty() || line.front() != '#') {
        return SyntaxNodeKind::Unknown;
    }
    line.remove_prefix(1);
    while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
        line.remove_prefix(1);
    }

    std::string tokenText = "#";
    while (!line.empty()) {
        const char ch = line.front();
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_')) {
            break;
        }
        tokenText.push_back(ch);
        line.remove_prefix(1);
    }
    return tokenText.size() == 1 ? SyntaxNodeKind::Unknown : SyntaxNodeKindFromTokenText(tokenText);
}

SyntaxSymbolInfo SyntaxSymbolInfoForSymbol(TSSymbol symbol) {
    const auto& symbols = SyntaxInfoBySymbol();
    return static_cast<size_t>(symbol) < symbols.size() ? symbols[symbol] : SyntaxSymbolInfo{};
}

std::string_view SyntaxNodeKindTokenText(SyntaxNodeKind kind) {
    const size_t index = KindIndex(kind);
    if (index >= kSyntaxKindInfoByKind.size()) {
        return {};
    }
    return kSyntaxKindInfoByKind[index].tokenText;
}

std::string_view SyntaxNodeKindName(SyntaxNodeKind kind) {
    switch (kind) {
        case SyntaxNodeKind::Unknown:
            return "Unknown";
        case SyntaxNodeKind::Tree:
            return "Tree";
        case SyntaxNodeKind::LexicalToken:
            return "LexicalToken";
        case SyntaxNodeKind::Comment:
            return "Comment";
        case SyntaxNodeKind::TrailingComment:
            return "TrailingComment";
        case SyntaxNodeKind::BlankLine:
            return "BlankLine";
        case SyntaxNodeKind::Error:
            return "Error";
        case SyntaxNodeKind::Missing:
            return "Missing";
        case SyntaxNodeKind::TranslationUnit:
            return "TranslationUnit";
        case SyntaxNodeKind::IncludeRun:
            return "IncludeRun";
        case SyntaxNodeKind::MacroReplacementList:
            return "MacroReplacementList";
        case SyntaxNodeKind::Declaration:
            return "Declaration";
        case SyntaxNodeKind::FieldDeclaration:
            return "FieldDeclaration";
        case SyntaxNodeKind::AliasDeclaration:
            return "AliasDeclaration";
        case SyntaxNodeKind::FunctionPointerAliasDeclaration:
            return "FunctionPointerAliasDeclaration";
        case SyntaxNodeKind::FunctionDefinition:
            return "FunctionDefinition";
        case SyntaxNodeKind::CompoundStatement:
            return "CompoundStatement";
        case SyntaxNodeKind::FieldDeclarationList:
            return "FieldDeclarationList";
        case SyntaxNodeKind::EnumeratorList:
            return "EnumeratorList";
        case SyntaxNodeKind::InitializerList:
            return "InitializerList";
        case SyntaxNodeKind::FieldInitializerList:
            return "FieldInitializerList";
        case SyntaxNodeKind::FieldInitializer:
            return "FieldInitializer";
        case SyntaxNodeKind::DeclarationList:
            return "DeclarationList";
        case SyntaxNodeKind::NamespaceDefinition:
            return "NamespaceDefinition";
        case SyntaxNodeKind::LinkageSpecification:
            return "LinkageSpecification";
        case SyntaxNodeKind::EnumSpecifier:
            return "EnumSpecifier";
        case SyntaxNodeKind::ClassSpecifier:
            return "ClassSpecifier";
        case SyntaxNodeKind::StructSpecifier:
            return "StructSpecifier";
        case SyntaxNodeKind::UnionSpecifier:
            return "UnionSpecifier";
        case SyntaxNodeKind::BaseClassClause:
            return "BaseClassClause";
        case SyntaxNodeKind::AccessSpecifier:
            return "AccessSpecifier";
        case SyntaxNodeKind::IfStatement:
            return "IfStatement";
        case SyntaxNodeKind::ElseClause:
            return "ElseClause";
        case SyntaxNodeKind::ForStatement:
            return "ForStatement";
        case SyntaxNodeKind::WhileStatement:
            return "WhileStatement";
        case SyntaxNodeKind::DoStatement:
            return "DoStatement";
        case SyntaxNodeKind::SwitchStatement:
            return "SwitchStatement";
        case SyntaxNodeKind::CaseStatement:
            return "CaseStatement";
        case SyntaxNodeKind::ReturnStatement:
            return "ReturnStatement";
        case SyntaxNodeKind::CoReturnStatement:
            return "CoReturnStatement";
        case SyntaxNodeKind::ConditionClause:
            return "ConditionClause";
        case SyntaxNodeKind::InitStatement:
            return "InitStatement";
        case SyntaxNodeKind::PreprocCall:
            return "PreprocCall";
        case SyntaxNodeKind::MacroDefinition:
            return "MacroDefinition";
        case SyntaxNodeKind::PreprocInclude:
            return "PreprocInclude";
        case SyntaxNodeKind::PreprocIf:
            return "PreprocIf";
        case SyntaxNodeKind::PreprocIfdef:
            return "PreprocIfdef";
        case SyntaxNodeKind::PreprocElse:
            return "PreprocElse";
        case SyntaxNodeKind::PreprocElif:
            return "PreprocElif";
        case SyntaxNodeKind::PreprocUsing:
            return "PreprocUsing";
        case SyntaxNodeKind::PreprocParams:
            return "PreprocParams";
        case SyntaxNodeKind::PreprocArg:
            return "PreprocArg";
        case SyntaxNodeKind::RawMacroReplacement:
            return "RawMacroReplacement";
        case SyntaxNodeKind::BinaryExpression:
            return "BinaryExpression";
        case SyntaxNodeKind::UnaryExpression:
            return "UnaryExpression";
        case SyntaxNodeKind::ConditionalExpression:
            return "ConditionalExpression";
        case SyntaxNodeKind::CommaExpression:
            return "CommaExpression";
        case SyntaxNodeKind::AssignmentExpression:
            return "AssignmentExpression";
        case SyntaxNodeKind::InitDeclarator:
            return "InitDeclarator";
        case SyntaxNodeKind::CastExpression:
            return "CastExpression";
        case SyntaxNodeKind::PointerDeclarator:
            return "PointerDeclarator";
        case SyntaxNodeKind::AbstractPointerDeclarator:
            return "AbstractPointerDeclarator";
        case SyntaxNodeKind::ReferenceDeclarator:
            return "ReferenceDeclarator";
        case SyntaxNodeKind::AbstractReferenceDeclarator:
            return "AbstractReferenceDeclarator";
        case SyntaxNodeKind::HandleDeclarator:
            return "HandleDeclarator";
        case SyntaxNodeKind::AbstractHandleDeclarator:
            return "AbstractHandleDeclarator";
        case SyntaxNodeKind::MemberPointerDeclarator:
            return "MemberPointerDeclarator";
        case SyntaxNodeKind::FunctionDeclarator:
            return "FunctionDeclarator";
        case SyntaxNodeKind::AbstractFunctionDeclarator:
            return "AbstractFunctionDeclarator";
        case SyntaxNodeKind::ParenthesizedDeclarator:
            return "ParenthesizedDeclarator";
        case SyntaxNodeKind::AbstractParenthesizedDeclarator:
            return "AbstractParenthesizedDeclarator";
        case SyntaxNodeKind::ParameterList:
            return "ParameterList";
        case SyntaxNodeKind::ArgumentList:
            return "ArgumentList";
        case SyntaxNodeKind::SubscriptArgumentList:
            return "SubscriptArgumentList";
        case SyntaxNodeKind::TemplateParameterList:
            return "TemplateParameterList";
        case SyntaxNodeKind::TemplateArgumentList:
            return "TemplateArgumentList";
        case SyntaxNodeKind::TemplateDeclaration:
            return "TemplateDeclaration";
        case SyntaxNodeKind::TemplateInstantiation:
            return "TemplateInstantiation";
        case SyntaxNodeKind::RequiresClause:
            return "RequiresClause";
        case SyntaxNodeKind::RequiresExpression:
            return "RequiresExpression";
        case SyntaxNodeKind::RequirementSeq:
            return "RequirementSeq";
        case SyntaxNodeKind::NestedRequirement:
            return "NestedRequirement";
        case SyntaxNodeKind::RefQualifier:
            return "RefQualifier";
        case SyntaxNodeKind::LambdaExpression:
            return "LambdaExpression";
        case SyntaxNodeKind::LambdaCaptureSpecifier:
            return "LambdaCaptureSpecifier";
        case SyntaxNodeKind::StructuredBindingDeclarator:
            return "StructuredBindingDeclarator";
        case SyntaxNodeKind::SpliceSpecifier:
            return "SpliceSpecifier";
        case SyntaxNodeKind::FieldDesignator:
            return "FieldDesignator";
        case SyntaxNodeKind::FieldExpression:
            return "FieldExpression";
        case SyntaxNodeKind::TrailingReturnType:
            return "TrailingReturnType";
        case SyntaxNodeKind::OperatorName:
            return "OperatorName";
        case SyntaxNodeKind::OperatorCast:
            return "OperatorCast";
        case SyntaxNodeKind::LabeledStatement:
            return "LabeledStatement";
        case SyntaxNodeKind::AttributeSpecifier:
            return "AttributeSpecifier";
        case SyntaxNodeKind::AttributeDeclaration:
            return "AttributeDeclaration";
        case SyntaxNodeKind::Attribute:
            return "Attribute";
        case SyntaxNodeKind::AttributedStatement:
            return "AttributedStatement";
        case SyntaxNodeKind::MacroCallItem:
            return "MacroCallItem";
        case SyntaxNodeKind::BareMacroItem:
            return "BareMacroItem";
        case SyntaxNodeKind::MacroStatementSequence:
            return "MacroStatementSequence";
        case SyntaxNodeKind::MsCallModifier:
            return "MsCallModifier";
        case SyntaxNodeKind::MsDeclspecModifier:
            return "MsDeclspecModifier";
        case SyntaxNodeKind::FunctionSuffixMacro:
            return "FunctionSuffixMacro";
        case SyntaxNodeKind::PureVirtualClause:
            return "PureVirtualClause";
        case SyntaxNodeKind::ConcatenatedString:
            return "ConcatenatedString";
        case SyntaxNodeKind::RawStringLiteral:
            return "RawStringLiteral";
        case SyntaxNodeKind::StringLiteral:
            return "StringLiteral";
        case SyntaxNodeKind::UserDefinedLiteral:
            return "UserDefinedLiteral";
        case SyntaxNodeKind::SystemLibString:
            return "SystemLibString";
        case SyntaxNodeKind::CharacterLiteral:
            return "CharacterLiteral";
        case SyntaxNodeKind::NumberLiteral:
            return "NumberLiteral";
        case SyntaxNodeKind::Identifier:
            return "Identifier";
        case SyntaxNodeKind::PreprocessorDirectiveInclude:
            return "PreprocessorDirectiveInclude";
        case SyntaxNodeKind::PreprocessorDirectiveDefine:
            return "PreprocessorDirectiveDefine";
        case SyntaxNodeKind::PreprocessorDirectiveIf:
            return "PreprocessorDirectiveIf";
        case SyntaxNodeKind::PreprocessorDirectiveIfdef:
            return "PreprocessorDirectiveIfdef";
        case SyntaxNodeKind::PreprocessorDirectiveIfndef:
            return "PreprocessorDirectiveIfndef";
        case SyntaxNodeKind::PreprocessorDirectiveElif:
            return "PreprocessorDirectiveElif";
        case SyntaxNodeKind::PreprocessorDirectiveElifdef:
            return "PreprocessorDirectiveElifdef";
        case SyntaxNodeKind::PreprocessorDirectiveElifndef:
            return "PreprocessorDirectiveElifndef";
        case SyntaxNodeKind::PreprocessorDirectiveElse:
            return "PreprocessorDirectiveElse";
        case SyntaxNodeKind::PreprocessorDirectiveEndif:
            return "PreprocessorDirectiveEndif";
        case SyntaxNodeKind::PreprocessorDirectiveUndef:
            return "PreprocessorDirectiveUndef";
        case SyntaxNodeKind::PreprocessorDirectivePragma:
            return "PreprocessorDirectivePragma";
        case SyntaxNodeKind::PreprocessorDirectiveError:
            return "PreprocessorDirectiveError";
        case SyntaxNodeKind::PreprocessorDirectiveWarning:
            return "PreprocessorDirectiveWarning";
        case SyntaxNodeKind::PreprocessorDirectiveLine:
            return "PreprocessorDirectiveLine";
        case SyntaxNodeKind::PreprocessorDirectiveUsing:
            return "PreprocessorDirectiveUsing";
        case SyntaxNodeKind::Hash:
            return "Hash";
        case SyntaxNodeKind::LeftParen:
            return "LeftParen";
        case SyntaxNodeKind::RightParen:
            return "RightParen";
        case SyntaxNodeKind::LeftBracket:
            return "LeftBracket";
        case SyntaxNodeKind::RightBracket:
            return "RightBracket";
        case SyntaxNodeKind::LeftBrace:
            return "LeftBrace";
        case SyntaxNodeKind::RightBrace:
            return "RightBrace";
        case SyntaxNodeKind::Less:
            return "Less";
        case SyntaxNodeKind::Greater:
            return "Greater";
        case SyntaxNodeKind::LessEqual:
            return "LessEqual";
        case SyntaxNodeKind::GreaterEqual:
            return "GreaterEqual";
        case SyntaxNodeKind::EqualEqual:
            return "EqualEqual";
        case SyntaxNodeKind::BangEqual:
            return "BangEqual";
        case SyntaxNodeKind::Spaceship:
            return "Spaceship";
        case SyntaxNodeKind::Plus:
            return "Plus";
        case SyntaxNodeKind::Minus:
            return "Minus";
        case SyntaxNodeKind::Star:
            return "Star";
        case SyntaxNodeKind::Slash:
            return "Slash";
        case SyntaxNodeKind::Percent:
            return "Percent";
        case SyntaxNodeKind::Caret:
            return "Caret";
        case SyntaxNodeKind::ReflectOperator:
            return "ReflectOperator";
        case SyntaxNodeKind::Ampersand:
            return "Ampersand";
        case SyntaxNodeKind::Pipe:
            return "Pipe";
        case SyntaxNodeKind::Bang:
            return "Bang";
        case SyntaxNodeKind::Tilde:
            return "Tilde";
        case SyntaxNodeKind::Equal:
            return "Equal";
        case SyntaxNodeKind::PlusEqual:
            return "PlusEqual";
        case SyntaxNodeKind::MinusEqual:
            return "MinusEqual";
        case SyntaxNodeKind::StarEqual:
            return "StarEqual";
        case SyntaxNodeKind::SlashEqual:
            return "SlashEqual";
        case SyntaxNodeKind::PercentEqual:
            return "PercentEqual";
        case SyntaxNodeKind::CaretEqual:
            return "CaretEqual";
        case SyntaxNodeKind::AmpersandEqual:
            return "AmpersandEqual";
        case SyntaxNodeKind::PipeEqual:
            return "PipeEqual";
        case SyntaxNodeKind::LessLess:
            return "LessLess";
        case SyntaxNodeKind::GreaterGreater:
            return "GreaterGreater";
        case SyntaxNodeKind::LessLessEqual:
            return "LessLessEqual";
        case SyntaxNodeKind::GreaterGreaterEqual:
            return "GreaterGreaterEqual";
        case SyntaxNodeKind::AmpersandAmpersand:
            return "AmpersandAmpersand";
        case SyntaxNodeKind::PipePipe:
            return "PipePipe";
        case SyntaxNodeKind::PlusPlus:
            return "PlusPlus";
        case SyntaxNodeKind::MinusMinus:
            return "MinusMinus";
        case SyntaxNodeKind::Arrow:
            return "Arrow";
        case SyntaxNodeKind::Dot:
            return "Dot";
        case SyntaxNodeKind::ArrowStar:
            return "ArrowStar";
        case SyntaxNodeKind::DotStar:
            return "DotStar";
        case SyntaxNodeKind::ColonColon:
            return "ColonColon";
        case SyntaxNodeKind::Question:
            return "Question";
        case SyntaxNodeKind::Colon:
            return "Colon";
        case SyntaxNodeKind::Semicolon:
            return "Semicolon";
        case SyntaxNodeKind::Comma:
            return "Comma";
        case SyntaxNodeKind::Ellipsis:
            return "Ellipsis";
        case SyntaxNodeKind::KeywordAlignas:
            return "KeywordAlignas";
        case SyntaxNodeKind::KeywordAlignof:
            return "KeywordAlignof";
        case SyntaxNodeKind::KeywordAsm:
            return "KeywordAsm";
        case SyntaxNodeKind::KeywordAuto:
            return "KeywordAuto";
        case SyntaxNodeKind::KeywordBool:
            return "KeywordBool";
        case SyntaxNodeKind::KeywordBreak:
            return "KeywordBreak";
        case SyntaxNodeKind::KeywordCase:
            return "KeywordCase";
        case SyntaxNodeKind::KeywordCatch:
            return "KeywordCatch";
        case SyntaxNodeKind::KeywordChar:
            return "KeywordChar";
        case SyntaxNodeKind::KeywordChar16T:
            return "KeywordChar16T";
        case SyntaxNodeKind::KeywordChar32T:
            return "KeywordChar32T";
        case SyntaxNodeKind::KeywordClass:
            return "KeywordClass";
        case SyntaxNodeKind::KeywordConcept:
            return "KeywordConcept";
        case SyntaxNodeKind::KeywordConst:
            return "KeywordConst";
        case SyntaxNodeKind::KeywordConsteval:
            return "KeywordConsteval";
        case SyntaxNodeKind::KeywordConstexpr:
            return "KeywordConstexpr";
        case SyntaxNodeKind::KeywordConstinit:
            return "KeywordConstinit";
        case SyntaxNodeKind::KeywordConstCast:
            return "KeywordConstCast";
        case SyntaxNodeKind::KeywordContinue:
            return "KeywordContinue";
        case SyntaxNodeKind::KeywordDecltype:
            return "KeywordDecltype";
        case SyntaxNodeKind::KeywordDefault:
            return "KeywordDefault";
        case SyntaxNodeKind::KeywordDelete:
            return "KeywordDelete";
        case SyntaxNodeKind::KeywordDo:
            return "KeywordDo";
        case SyntaxNodeKind::KeywordDouble:
            return "KeywordDouble";
        case SyntaxNodeKind::KeywordDynamicCast:
            return "KeywordDynamicCast";
        case SyntaxNodeKind::KeywordElse:
            return "KeywordElse";
        case SyntaxNodeKind::KeywordEnum:
            return "KeywordEnum";
        case SyntaxNodeKind::KeywordExplicit:
            return "KeywordExplicit";
        case SyntaxNodeKind::KeywordExport:
            return "KeywordExport";
        case SyntaxNodeKind::KeywordExtern:
            return "KeywordExtern";
        case SyntaxNodeKind::KeywordFalse:
            return "KeywordFalse";
        case SyntaxNodeKind::KeywordFinal:
            return "KeywordFinal";
        case SyntaxNodeKind::KeywordFinally:
            return "KeywordFinally";
        case SyntaxNodeKind::KeywordFloat:
            return "KeywordFloat";
        case SyntaxNodeKind::KeywordFor:
            return "KeywordFor";
        case SyntaxNodeKind::KeywordFriend:
            return "KeywordFriend";
        case SyntaxNodeKind::KeywordGoto:
            return "KeywordGoto";
        case SyntaxNodeKind::KeywordIf:
            return "KeywordIf";
        case SyntaxNodeKind::KeywordInline:
            return "KeywordInline";
        case SyntaxNodeKind::KeywordInt:
            return "KeywordInt";
        case SyntaxNodeKind::KeywordLong:
            return "KeywordLong";
        case SyntaxNodeKind::KeywordMutable:
            return "KeywordMutable";
        case SyntaxNodeKind::KeywordNamespace:
            return "KeywordNamespace";
        case SyntaxNodeKind::KeywordNew:
            return "KeywordNew";
        case SyntaxNodeKind::KeywordNoexcept:
            return "KeywordNoexcept";
        case SyntaxNodeKind::KeywordNullptr:
            return "KeywordNullptr";
        case SyntaxNodeKind::KeywordOperator:
            return "KeywordOperator";
        case SyntaxNodeKind::KeywordOverride:
            return "KeywordOverride";
        case SyntaxNodeKind::KeywordPrivate:
            return "KeywordPrivate";
        case SyntaxNodeKind::KeywordProtected:
            return "KeywordProtected";
        case SyntaxNodeKind::KeywordPublic:
            return "KeywordPublic";
        case SyntaxNodeKind::KeywordRegister:
            return "KeywordRegister";
        case SyntaxNodeKind::KeywordReinterpretCast:
            return "KeywordReinterpretCast";
        case SyntaxNodeKind::KeywordRequires:
            return "KeywordRequires";
        case SyntaxNodeKind::KeywordReturn:
            return "KeywordReturn";
        case SyntaxNodeKind::KeywordShort:
            return "KeywordShort";
        case SyntaxNodeKind::KeywordSigned:
            return "KeywordSigned";
        case SyntaxNodeKind::KeywordSizeof:
            return "KeywordSizeof";
        case SyntaxNodeKind::KeywordStatic:
            return "KeywordStatic";
        case SyntaxNodeKind::KeywordStaticAssert:
            return "KeywordStaticAssert";
        case SyntaxNodeKind::KeywordStaticCast:
            return "KeywordStaticCast";
        case SyntaxNodeKind::KeywordStruct:
            return "KeywordStruct";
        case SyntaxNodeKind::KeywordSwitch:
            return "KeywordSwitch";
        case SyntaxNodeKind::KeywordTemplate:
            return "KeywordTemplate";
        case SyntaxNodeKind::KeywordThis:
            return "KeywordThis";
        case SyntaxNodeKind::KeywordThreadLocal:
            return "KeywordThreadLocal";
        case SyntaxNodeKind::KeywordThrow:
            return "KeywordThrow";
        case SyntaxNodeKind::KeywordTrue:
            return "KeywordTrue";
        case SyntaxNodeKind::KeywordTry:
            return "KeywordTry";
        case SyntaxNodeKind::KeywordTypedef:
            return "KeywordTypedef";
        case SyntaxNodeKind::KeywordTypeid:
            return "KeywordTypeid";
        case SyntaxNodeKind::KeywordTypename:
            return "KeywordTypename";
        case SyntaxNodeKind::KeywordUnion:
            return "KeywordUnion";
        case SyntaxNodeKind::KeywordUnsigned:
            return "KeywordUnsigned";
        case SyntaxNodeKind::KeywordUsing:
            return "KeywordUsing";
        case SyntaxNodeKind::KeywordVirtual:
            return "KeywordVirtual";
        case SyntaxNodeKind::KeywordVoid:
            return "KeywordVoid";
        case SyntaxNodeKind::KeywordVolatile:
            return "KeywordVolatile";
        case SyntaxNodeKind::KeywordWcharT:
            return "KeywordWcharT";
        case SyntaxNodeKind::KeywordWhile:
            return "KeywordWhile";
        case SyntaxNodeKind::KeywordCdecl:
            return "KeywordCdecl";
        case SyntaxNodeKind::KeywordDeclspec:
            return "KeywordDeclspec";
        case SyntaxNodeKind::KeywordCoAwait:
            return "KeywordCoAwait";
        case SyntaxNodeKind::KeywordCoReturn:
            return "KeywordCoReturn";
        case SyntaxNodeKind::KeywordCoYield:
            return "KeywordCoYield";
    }
    return "Unknown";
}

std::uint64_t SyntaxNodeKindClasses(SyntaxNodeKind kind) {
    const size_t index = KindIndex(kind);
    if (index >= kSyntaxKindInfoByKind.size()) {
        return 0;
    }
    return kSyntaxKindInfoByKind[index].classes;
}

bool SyntaxNodeKindHasClass(SyntaxNodeKind kind, SyntaxNodeClass syntaxNodeClass) {
    return (SyntaxNodeKindClasses(kind) & Bit(syntaxNodeClass)) != 0;
}

bool CallableBodyAllowsCompactSingleStatementForm(const SyntaxNode& node, SyntaxNodeKind parentKind) {
    const bool callableOwner =
        parentKind == SyntaxNodeKind::FunctionDefinition || parentKind == SyntaxNodeKind::LambdaExpression;
    if (node.kind != SyntaxNodeKind::CompoundStatement || !callableOwner) {
        return false;
    }
    if (node.compactCallableBodyCache != 0) {
        return node.compactCallableBodyCache == 2;
    }
    const SyntaxNode* statement = OnlyContentChild(node);
    // Compact callable spacing and body-header choices must agree. A lone statement that owns a
    // compound block, such as if/switch/compound, needs normal block indentation for that subtree.
    // These are the same three existential queries as the former separate recursive walks: comments are searched
    // under the body, preprocessing under the callable parent, and compound blocks under the lone statement.
    const SyntaxNode& searchRoot = node.parent == nullptr ? node : *node.parent;
    const bool result = statement != nullptr && !CallableBodyHasDisqualifier(searchRoot, node, statement);
    node.compactCallableBodyCache = result ? 2 : 1;
    return result;
}
