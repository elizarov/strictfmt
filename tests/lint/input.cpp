// Types, nested scopes, default access and declaration ownership.
namespace BadNamespace {
class bad_class {
    int missing_suffix;
    int MixedCase_;
protected:
    int protected_missing;
public:
    int BadPublic;
    void badMethod(int BadParameter);
    void begin();
    void end();
    void inherited_name() override;
    bad_class();
    ~bad_class();
    operator bool() const;
    static constexpr int bad_constant = 1;
    static int BadStatic;
};
struct bad_struct {
    int BadField;
private:
    int missing_suffix;
};
union bad_union { int BadUnionField; };
enum class bad_enum { bad_enumerator, kGood };
using bad_alias = int;
typedef int bad_typedef, another_bad_typedef;
typedef void (*bad_callback_type)(int BadNestedParameter);
template<class bad_type, int bad_value, template<class> class bad_template>
struct GoodTemplate {};
template<typename bad_default_type = SomeType, int bad_default_value = 0>
void GoodTemplateFunction();
template<class... bad_pack> struct GoodPack {};
template<class GoodType> concept bad_concept = true;
namespace good_namespace = BadNamespace;
}

// Each declarator owns its modifiers. Pointee constness is not object constness.
const int bad_global_constant = 0;
const int* BadPointer;
int* const bad_constant_pointer = nullptr;
constexpr int bad_constexpr = 1;
int BadGlobal, AnotherBadGlobal = 0;
void bad_function(int BadParameter = DefaultValue(), int (*BadCallback)(int BadNestedParameter));
void GoodFunction() {
    int BadLocal;
    static int BadStaticLocal;
    for (int BadElement : Range()) {}
    for (int BadIndex = 0; BadIndex < 2; ++BadIndex) {}
    try {} catch (const Error& BadError) {}
    auto GoodLambda = [](int BadLambdaParameter) { int BadLambdaLocal; };
    auto [BadBinding, another_binding] = Pair();
}
template<class GoodType> void GoodVariadic(GoodType... BadPack);

// Check all syntactic branches, and the name of a macro rather than its expansion.
#if PLATFORM
int BadFirstBranch;
#else
int BadSecondBranch;
#endif
#define bad_macro(BadMacroParameter) int BadGeneratedName;
#define GOOD_MACRO(BadMacroParameter) int BadGeneratedName;

// Qualified definitions reuse externally declared names.
void External::inherited_name(int BadDefinitionParameter) {}

// Suppressions apply to real comments, not strings.
int BadSuppressed; // NOLINT(readability-identifier-naming)
// NOLINTNEXTLINE(readability-identifier-naming)
int BadNextLine;
// NOLINTBEGIN(readability-identifier-naming)
int BadInBlock;
// NOLINTEND(readability-identifier-naming)
int BadAfterBlock;
int BadUnrelated; // NOLINT(performance-implicit-conversion-in-loop)
const char* good_text = "NOLINTBEGIN";
int BadAfterString;
int BadGenericSuppressed; // NOLINT

// Function/object ambiguity is resolved independently of naming policy.
Widget local_value(argument);
Widget LooksCallable(argument);
Widget bad_name_(argument);
Widget bitwise_value(a & b);
Widget GoodDefault(Other&& value = {});
Widget braced_value{argument};
Widget BadBraced{argument};
Widget bad_function_with_parameter(int argument);
Widget bad_function_definition(argument) {}
Widget NamedParameter(Other BadParameter);
Widget ambiguous_nested(a & Function(BadArgument));

// Local aliases retain function identity without resolving external headers.
using Callback = void(int BadAliasParameter);
Callback GoodAliasFunction;
Callback bad_alias_function;
Callback* BadAliasPointer;
using ChainedCallback = Callback;
ChainedCallback bad_chained_function;
typedef void FunctionType(int BadTypedefParameter);
FunctionType bad_typedef_function;
namespace nested {
using Callback = int;
Callback BadShadowedVariable;
}
Callback GoodRestoredFunction;

// Static members, references, access selectors, arrays and captures.
struct GoodMembers {
    static const int bad_static_constant = 0;
    static const int kGoodStatic = 0;
    const int* BadPointee;
    int* const BadConstPointer = nullptr;
    int BadArray[2];
};
void GoodCaptures() {
    int value = 1;
    auto good_lambda = [BadCapture = value] {};
    try {} catch (Error& BadCatch) {
        auto nested_lambda = [](int BadCatchLambdaParameter) {};
    }
}

// Foreign forward declarations and locally defined object-like macros.
namespace ForeignNamespace { class ForeignType; }
class forward_type;
#define OBJECT_NAME expanded_name
int OBJECT_NAME;
#define NAMESPACE_NAME expanded_namespace
namespace NAMESPACE_NAME { int good; }

// NOLINT tokens require word boundaries; block comments retain line locations.
int BadTokenPrefix; // ANOLINT
int BadTokenSuffix; // NOLINT_typo
/* NOLINTNEXTLINE(readability-identifier-naming) */
int BadBlockSuppressed;
/* ordinary text
   NOLINTNEXTLINE(readability-identifier-naming)
*/ int BadMultiLineSuppressed;
int BadAfterMultiline;

// Access flows independently through conditional branches and rejoins as alternatives.
class ConditionalAccess {
#if FIRST
public:
    int BadFirst;
#elif SECOND
protected:
    int missing_second_suffix;
#else
private:
    int missing_else_suffix;
#endif
    int public_name;
    int private_name_;
    int BadEveryAlternative;
private:
    int missing_final_suffix;
    friend void bad_buddy(int BadFriendParameter);
};

// Assignment initializers cannot be mistaken for function parameter lists.
auto BadAssigned = MakeValue();
auto BadCopy = other;
auto [BadGlobalBinding, good_global_binding] = Pair();
void GoodBindings() {
    auto& [BadRefBinding, good_ref] = pair;
    for (auto [BadRangeBinding, good_range] : pairs) {}
}

// final without virtual also inherits a callable's name from its base declaration.
class GoodFinal : public ExternalBase {
public:
    void inheritedName() final;
    virtual void bad_new_virtual() final;
};

// Each function/object interpretation retains its own qualifiers.
namespace ambiguous_qualifiers {
const Widget kGlobal(argument);
Widget const kEastGlobal(argument);
const Widget invalid_global(argument);
void CheckInitializers() {
    static const Widget kPattern(kArgument);
    static Widget const kEastConst(kArgument);
    static const Visitor<Type> kVisitor(Policy::kFirst);
    static Widget* const kPointer(pointer);
    static const Widget* pointer_value(pointer);
    static const Widget invalid_constant(argument);
    static const Visitor<Type> invalid_visitor(Policy::kFirst);
    static Widget kMutable(argument);
    static const Widget invalid_braced{argument};
    const Widget local_const(argument);
    const Widget kInvalidLocal{argument};
    static Widget* const invalid_pointer(pointer);
}
const Widget bad_const_return(int argument);
const Widget* bad_pointee_return(int argument);
}

// Check lists select lint checks, not source identifiers or configured rule names.
int BadShortCheck; // NOLINT(identifier-naming)
int BadPrefixedCheck; // NOLINT(strictfmt-identifier-naming)
int BadRuleCheck; // NOLINT(Objects)
int BadSourceName; // NOLINT(BadSourceName)
int BadEmptyCheckList; // NOLINT()
int BadOnlyOtherChecks; // NOLINT(bugprone-unused-return-value, performance-implicit-conversion-in-loop)
int AllChecksSuppressed; // NOLINT(*)
int MixedChecksSuppressed; // NOLINT( performance-implicit-conversion-in-loop, readability-identifier-naming )
// NOLINTNEXTLINE(readability-identifier-naming, performance-implicit-conversion-in-loop)
int NextMixedChecksSuppressed;
// NOLINTNEXTLINE(identifier-naming)
int BadShortNextLine;
// NOLINTBEGIN(strictfmt-identifier-naming)
int BadPrefixedRegion;
// NOLINTEND(strictfmt-identifier-naming)
// NOLINTBEGIN(*)
int AllRegionChecksSuppressed;
// NOLINTEND(*)
int BadAfterAllRegion;

// Case patterns require a nonempty ASCII name with the configured first character.
void GoodCaseNames() {
    int _;
    int __;
    int _2;
    int _name;
    int name_;
    int lower__case;
    int имя;
}
constexpr int k0 = 0;
constexpr int k3dsUrl = 0;
constexpr int k3DSUrl = 0;
constexpr int k_ = 0;
constexpr int k__ = 0;
struct GoodSuffixNames {
private:
    int _;
    int __;
};

// Regex alternatives match whole identifiers.
// The exception for end does not exempt longer names containing end.
void begin_extra();
void extra_end();
void extra_begin_extra();
void extra_end_extra();
void extra_hash_value();
void hash_value_extra();
void getSpendingCategoriesResponse();

// Mixed ASCII and Cyrillic names do not satisfy an ASCII case pattern.
int exс;
