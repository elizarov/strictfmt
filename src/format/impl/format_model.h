#pragma once

#include <cstdint>
#include <memory>
#include <memory_resource>
#include <string>
#include <string_view>
#include <tree_sitter/api.h>
#include <vector>

struct ParseResult {
    bool ok = false;
    std::string error;
};

struct PrintToken;
struct SyntaxNode;

using SyntaxChildList = std::pmr::vector<SyntaxNode*>;

enum class SyntaxNodeKind : std::uint16_t {
    // Structural nodes.
    Unknown,
    Tree,
    FreeToken,
    Comment,
    TrailingComment,
    BlankLine,
    Error,
    Missing,

    // Tree-sitter syntax nodes.
    TranslationUnit,
    IncludeRun,
    MacroReplacementList,
    Declaration,
    FieldDeclaration,
    AliasDeclaration,
    FunctionPointerAliasDeclaration,
    FunctionDefinition,
    CompoundStatement,
    FieldDeclarationList,
    EnumeratorList,
    InitializerList,
    FieldInitializerList,
    FieldInitializer,
    DeclarationList,
    NamespaceDefinition,
    LinkageSpecification,
    EnumSpecifier,
    ClassSpecifier,
    StructSpecifier,
    BaseClassClause,
    AccessSpecifier,
    IfStatement,
    ElseClause,
    ForStatement,
    WhileStatement,
    DoStatement,
    SwitchStatement,
    CaseStatement,
    ReturnStatement,
    CoReturnStatement,
    ConditionClause,
    InitStatement,
    PreprocAssignmentStatement,
    PreprocCall,
    MacroDefinition,
    PreprocInclude,
    PreprocIf,
    PreprocIfdef,
    PreprocElse,
    PreprocElif,
    PreprocUsing,
    PreprocParams,
    PreprocArg,
    RawMacroReplacement,
    BinaryExpression,
    UnaryExpression,
    ConditionalExpression,
    CommaExpression,
    AssignmentExpression,
    InitDeclarator,
    CastExpression,
    PointerDeclarator,
    AbstractPointerDeclarator,
    ReferenceDeclarator,
    AbstractReferenceDeclarator,
    HandleDeclarator,
    AbstractHandleDeclarator,
    MemberPointerDeclarator,
    FunctionDeclarator,
    AbstractFunctionDeclarator,
    ParenthesizedDeclarator,
    AbstractParenthesizedDeclarator,
    ParameterList,
    ArgumentList,
    SubscriptArgumentList,
    TemplateParameterList,
    TemplateArgumentList,
    TemplateDeclaration,
    RequiresClause,
    RequiresExpression,
    RequirementSeq,
    NestedRequirement,
    RefQualifier,
    LambdaExpression,
    LambdaCaptureSpecifier,
    StructuredBindingDeclarator,
    FieldDesignator,
    FieldExpression,
    TrailingReturnType,
    OperatorName,
    OperatorCast,
    LabeledStatement,
    AttributeSpecifier,
    AttributeDeclaration,
    Attribute,
    AttributedStatement,
    MacroCallItem,
    MacroStatementSequence,
    MsCallModifier,
    MsDeclspecModifier,
    FunctionSuffixMacro,
    ConcatenatedString,
    RawStringLiteral,
    StringLiteral,
    SystemLibString,
    CharacterLiteral,
    NumberLiteral,
    Identifier,

    // Preprocessor directive tokens.
    PreprocessorDirectiveInclude,
    PreprocessorDirectiveDefine,
    PreprocessorDirectiveIf,
    PreprocessorDirectiveIfdef,
    PreprocessorDirectiveIfndef,
    PreprocessorDirectiveElif,
    PreprocessorDirectiveElifdef,
    PreprocessorDirectiveElifndef,
    PreprocessorDirectiveElse,
    PreprocessorDirectiveEndif,
    PreprocessorDirectiveUndef,
    PreprocessorDirectivePragma,
    PreprocessorDirectiveError,
    PreprocessorDirectiveWarning,
    PreprocessorDirectiveLine,
    PreprocessorDirectiveUsing,

    // Known tokens.
    Hash,
    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    LeftBrace,
    RightBrace,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    EqualEqual,
    BangEqual,
    Spaceship,
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Caret,
    Ampersand,
    Pipe,
    Bang,
    Tilde,
    Equal,
    PlusEqual,
    MinusEqual,
    StarEqual,
    SlashEqual,
    PercentEqual,
    CaretEqual,
    AmpersandEqual,
    PipeEqual,
    LessLess,
    GreaterGreater,
    LessLessEqual,
    GreaterGreaterEqual,
    AmpersandAmpersand,
    PipePipe,
    PlusPlus,
    MinusMinus,
    Arrow,
    Dot,
    ArrowStar,
    DotStar,
    ColonColon,
    Question,
    Colon,
    Semicolon,
    Comma,
    Ellipsis,
    KeywordAlignas,
    KeywordAlignof,
    KeywordAsm,
    KeywordAuto,
    KeywordBool,
    KeywordBreak,
    KeywordCase,
    KeywordCatch,
    KeywordChar,
    KeywordChar16T,
    KeywordChar32T,
    KeywordClass,
    KeywordConcept,
    KeywordConst,
    KeywordConsteval,
    KeywordConstexpr,
    KeywordConstinit,
    KeywordConstCast,
    KeywordContinue,
    KeywordDecltype,
    KeywordDefault,
    KeywordDelete,
    KeywordDo,
    KeywordDouble,
    KeywordDynamicCast,
    KeywordElse,
    KeywordEnum,
    KeywordExplicit,
    KeywordExport,
    KeywordExtern,
    KeywordFalse,
    KeywordFinal,
    KeywordFinally,
    KeywordFloat,
    KeywordFor,
    KeywordFriend,
    KeywordGoto,
    KeywordIf,
    KeywordInline,
    KeywordInt,
    KeywordLong,
    KeywordMutable,
    KeywordNamespace,
    KeywordNew,
    KeywordNoexcept,
    KeywordNullptr,
    KeywordOperator,
    KeywordOverride,
    KeywordPrivate,
    KeywordProtected,
    KeywordPublic,
    KeywordRegister,
    KeywordReinterpretCast,
    KeywordRequires,
    KeywordReturn,
    KeywordShort,
    KeywordSigned,
    KeywordSizeof,
    KeywordStatic,
    KeywordStaticAssert,
    KeywordStaticCast,
    KeywordStruct,
    KeywordSwitch,
    KeywordTemplate,
    KeywordThis,
    KeywordThreadLocal,
    KeywordThrow,
    KeywordTrue,
    KeywordTry,
    KeywordTypedef,
    KeywordTypeid,
    KeywordTypename,
    KeywordUnion,
    KeywordUnsigned,
    KeywordUsing,
    KeywordVirtual,
    KeywordVoid,
    KeywordVolatile,
    KeywordWcharT,
    KeywordWhile,
    KeywordCdecl,
    KeywordDeclspec,
    KeywordCoAwait,
    KeywordCoReturn,
    KeywordCoYield,
};

enum class SyntaxNodeClass : std::uint64_t {
    Keyword = 1ull << 0,
    ControlKeyword = 1ull << 1,
    AttachAfterBlockKeyword = 1ull << 2,
    AccessKeyword = 1ull << 3,
    MemberOperator = 1ull << 4,
    AssignmentOperator = 1ull << 5,
    BinaryOperator = 1ull << 6,
    UnaryOperator = 1ull << 7,
    DeclaratorReferenceToken = 1ull << 8,
    Known = 1ull << 9,
    Tree = 1ull << 10,
    Literal = 1ull << 11,
    StringLike = 1ull << 12,
    WholeNodeAsFreeToken = 1ull << 13,
    AtomicPreprocessor = 1ull << 14,
    MacroDefinition = 1ull << 15,
    MacroDeclarationFragment = 1ull << 16,
    DeclaratorReferenceParent = 1ull << 17,
    ParenthesizedDeclarator = 1ull << 18,
    CompoundBlock = 1ull << 19,
    ControlHeader = 1ull << 20,
    FlatLogicalHeader = 1ull << 21,
    ChainOperator = 1ull << 22,
    IncludeDirective = 1ull << 23,
    Comment = 1ull << 24,
    Trivia = 1ull << 25,
    DeclarationModifierPreprocessor = 1ull << 26,
    ConditionalRhsPreprocessor = 1ull << 27,
    AllowedPreprocessorContainer = 1ull << 28,
    AllowedListPreprocessorContainer = 1ull << 29,
    ConditionalPreprocessorTree = 1ull << 30,
    ConditionalPreprocessorOpen = 1ull << 31,
    PreserveBlankLineParent = 1ull << 32,
    ListForceSplitMarker = 1ull << 33,
    PrefixList = 1ull << 34,
    DeclarationNode = 1ull << 35,
    SemanticDelimitedParent = 1ull << 36,
    PreprocessorSplitList = 1ull << 37,
    OpeningDelimiter = 1ull << 38,
    PreprocessorDirective = 1ull << 39,
    ConditionalPreprocessorDirective = 1ull << 40,
    ConditionalOpeningDirective = 1ull << 41,
    ConditionalBranchSeparatorDirective = 1ull << 42,
    CheckedPreprocessorDirective = 1ull << 43,
    EndifDirective = 1ull << 44,
    SupportedPreprocessorPlacement = 1ull << 45,
};

enum class SyntaxWrapperRole : std::uint8_t {
    None,
    Flatten,
    WholeToken,
    WholeAtom,
    WholeFieldAtom,
    CompactEmptyDelimited,
};

struct SyntaxSymbolInfo {
    SyntaxNodeKind treeKind = SyntaxNodeKind::Unknown;
    SyntaxNodeKind tokenKind = SyntaxNodeKind::Unknown;
    std::uint64_t classes = 0;
    SyntaxWrapperRole wrapperRole = SyntaxWrapperRole::None;
};

SyntaxNodeKind SyntaxNodeKindFromTreeType(std::string_view type);
SyntaxNodeKind SyntaxNodeKindFromTokenText(std::string_view text);
SyntaxNodeKind SyntaxNodeKindFromPreprocessorDirectiveLine(std::string_view line);
SyntaxSymbolInfo SyntaxSymbolInfoForSymbol(TSSymbol symbol);
std::string_view SyntaxNodeKindName(SyntaxNodeKind kind);
std::string_view SyntaxNodeKindTokenText(SyntaxNodeKind kind);
bool SyntaxNodeKindHasClass(SyntaxNodeKind kind, SyntaxNodeClass syntaxNodeClass);
bool LambdaBodyAllowsCompactSingleStatementForm(const SyntaxNode& node, SyntaxNodeKind parentKind);

struct SyntaxNode {
    explicit SyntaxNode(std::pmr::memory_resource* childResource = std::pmr::get_default_resource());

    // Keep nodes maximally generic and space-efficient; avoid fields that only apply to one node kind.
    SyntaxNodeKind kind = SyntaxNodeKind::Unknown;
    std::uint64_t classes = 0;
    std::string_view text;
    const SyntaxNode* parent = nullptr;
    size_t depth = 0;
    SyntaxChildList children;

    // Break model scratch storage. These fields are valid only for the active formatting pass mark.
    mutable const PrintToken* formatPrintToken = nullptr;
    mutable std::uint32_t formatSelectionMark = 0;
    mutable std::uint32_t formatTokenMark = 0;
    mutable bool formatSpaceBefore = false;
};

struct FormatModel {
    FormatModel();
    FormatModel(const FormatModel&) = delete;
    FormatModel& operator=(const FormatModel&) = delete;
    FormatModel(FormatModel&&) noexcept = default;
    FormatModel& operator=(FormatModel&&) noexcept = default;

    ParseResult parse;
    std::unique_ptr<std::string> sourceText;
    std::unique_ptr<std::pmr::monotonic_buffer_resource> childStorage;
    std::vector<SyntaxNode> nodes;
    SyntaxNode* root = nullptr;
};
