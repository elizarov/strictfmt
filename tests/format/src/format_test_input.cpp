#pragma once
#pragma strictfmt_fixture

#include <windows.h>
#include <module/ITEM_ONE.hpp>
#include <chrono> // formatter spacing regression include
#include "zeta/thing.h"
#include "Zebra/thing.h"
#include <vector>
#include <module/items.hpp>
#include <string_view>
#include "format_test_fixture.h"
#include "vendor/library.h"
#include <module/ITEMS.hpp>
#include <winsock2.h>
#include <string>
#include "alpha/thing.h"
#include "Alpha/thing.h"
#include <algorithm>
#include <ws2tcpip.h>

#define FORMAT_FIXTURE_SUM(firstValue, secondValue, thirdValue) \
    ((firstValue) + \
        (secondValue) + \
        (thirdValue) + \
        (firstValue) + \
        (secondValue) + \
        (thirdValue))
#define FORMAT_FIXTURE_SHORT_MACRO(value) (value)
#define FORMAT_FIXTURE_MUCH_LONGER_MACRO(value) (value)
#define FORMAT_FIXTURE_STATEMENT_ARGUMENT(MethodName, ParamType, ParamName, ...) \
auto& MethodName(ParamType ParamName) { __VA_ARGS__ return *this; }
#define FORMAT_FIXTURE_STRUCTURED_LAMBDA [](){a();b();}
#define FORMAT_FIXTURE_DECLARE_OPTION(name,type) void name(type)
#define FORMAT_FIXTURE_LOAD_OPTIONAL(function,name) \
function=reinterpret_cast<decltype(function)>(GetProcAddress(module_,name))
#define FORMAT_FIXTURE_ITEMS(X) \
X(Alpha,"alpha") X(Beta,"beta") X(Gamma,"gamma")
#define FORMAT_FIXTURE_ENUM_ITEMS(X) \
X(First,"first") X(Second,"second")
#define FORMAT_FIXTURE_COMMENT_CONTINUATION(callback) \
    callback(); \
    /* cold testing path: */ \
    callback();
#define FORMAT_FIXTURE_TOKEN_PASTE(prefix,suffix) \
prefix ## suffix
#define FORMAT_PASTED_FN(name) inline int Get##name##Value(){return 0;}
#define FORMAT_PASTED_INIT(name) {(name),Get##name##Value()}
#define FORMAT_PASTED_NUMBER(suffix) 10 ## suffix
#define FORMAT_FIXTURE_STRINGIZE(value) \
#value
#define FORMAT_FIXTURE_FILEPATH FORMAT_NAMESPACE::logging::impl::CutFilePath(__builtin_FILE())
#define FORMAT_FIXTURE_REGISTER_TYPE(Type,Index) \
constexpr std::size_t TypeToId(FormatFixtureIdentity<Type>) noexcept{return Index;} \
constexpr Type IdToType(FormatFixtureSize<Index>) noexcept{return FormatFixtureConstruct<Type>();}
#define ENUM_STRING_DECLARE(EnumType, ItemsMacro) \
    enum class EnumType{ItemsMacro( \
        ENUM_STRING_DECLARE_ENUMERATOR \
    )}; template <> struct EnumStringTraits<EnumType>{static constexpr auto names = std::to_array<std::string_view>({ItemsMacro(ENUM_STRING_DECLARE_NAME)}); static_assert(enum_string_detail::ValidateCanonicalNames(names)); }

ENUM_STRING_DECLARE(FormatFixtureEnum, FORMAT_FIXTURE_ENUM_ITEMS);
FORMAT_FIXTURE_REGISTER_TYPE(unsigned char,1)
FORMAT_FIXTURE_CREATE_METRIC(
Metrics,
Tag,
"path",
(first,"First")  //
(second,"Second")  //
(third,"Third")  //
)
#undef FORMAT_FIXTURE_ENUM_ITEMS
#undef FORMAT_FIXTURE_ENUM_ITEMS_AUX
#define FORMAT_FIXTURE_TEMP_MACRO(value) (value)
#undef FORMAT_FIXTURE_TEMP_MACRO

namespace format_fixture {

class LayoutEditWidgetIdentity {};

namespace std_fixture {

template <typename T>
class vector {};

class string {};

}

constexpr auto kGeneratedFixtureValue =
#include "format_fixture_value.inc"
;

constexpr wchar_t kFilterCueText[]=L"Filter settings";

constexpr auto kFixtureSyntaxKindMappings =
    std::to_array<SyntaxKindMapping>({Kind(SyntaxNodeKind::Tree, Bit(SyntaxNodeClass::Tree)),

Tree(SyntaxNodeKind::TranslationUnit, "translation_unit"), Tree(SyntaxNodeKind::IncludeRun, "include_run"), Tree(
    SyntaxNodeKind::MacroReplacementList,
    "macro_replacement_list"
), Tree(SyntaxNodeKind::Declaration, "declaration", Bit(SyntaxNodeClass::MacroDeclarationFragment)), Tree(
    SyntaxNodeKind::FieldDeclaration,
    "field_declaration",
    Bit(SyntaxNodeClass::MacroDeclarationFragment)
)});

constexpr auto kFixtureCommentedSyntaxKindMappings =
    std::to_array<SyntaxKindMapping>({Kind(SyntaxNodeKind::Tree, Bit(SyntaxNodeClass::Tree)),

// tree nodes

Tree(SyntaxNodeKind::TranslationUnit, "translation_unit"), Tree(SyntaxNodeKind::IncludeRun, "include_run")});

class FormattingExample{
	public:
	int * pointer;

    int& reference;
    mutable ID2D1RenderTarget* cachedD2DBitmapTarget_ = nullptr;
    mutable RenderState& cachedRenderState_;

    FormattingExample(int* pointerValue,int& referenceValue):pointer(pointerValue),reference(referenceValue){}

private:
    int value;
};

class MacroSeparatedMethodHost{
#define FORMAT_FIXTURE_METHOD_MARKER(value) (value)

    void MethodAfterMacro() {}

    int fieldAfterMethod;
};

class MacroStatementArgumentHost{
    FORMAT_FIXTURE_STATEMENT_ARGUMENT(SetValue,int,value,
        value_=value;
        if(value<0){value_=0;}
    );

    int value_;
};

class DashboardShellHost{
public:
virtual~DashboardShellHost()=default;
virtual std::optional<FilePath> PromptDiagnosticsSavePath(std::string_view defaultFileName,std::string_view filter,std::string_view defaultExtension) const=0;
virtual::Renderer& Renderer()=0;
virtual const::Renderer& Renderer() const=0;
virtual DashboardOverlayState & LayoutDashboardOverlayState()=0;
virtual void Draw(::Renderer & renderer,const WidgetAnimationState & state) const=0;
virtual void ResolveLayoutState(const WidgetHost& renderer,const RenderRect& rect);
virtual void Draw(WidgetHost& renderer,const struct WidgetLayout& widget,const MetricSource& metrics) const;
};

class DialogRedrawScope{
public:
DialogRedrawScope(const DialogRedrawScope &)=delete;
DialogRedrawScope& operator=(const DialogRedrawScope &)=delete;
};

__declspec(noinline) bool DashboardController::FinishConfigMutation(DashboardShellHost& shell,bool refreshThemedIcons){
return refreshThemedIcons;
}

struct FormatTableRow{
const char * name;
int labelControl;
int editControl;
int flags;
};

struct FormatBitFields{
unsigned shortBits : 1;
unsigned muchLongerBits : 2;
};

struct AlignedStorage{
alignas(void*)unsigned char storage_[sizeof(void*)] {};
};

// Defaulted operator fixture.
struct ColorMixExpression {
    std::string target;
    double amount = 0.0;

    bool operator==(const ColorMixExpression& other) const = default;
};

namespace forward_declaration_grouping {
class Client;
struct Request;
template<typename T>
class Box;
class Definition{};
class AfterDefinition;
struct ElaboratedObject* object;
}

struct FirstTopLevelDeclarationGroupingType{};
enum class SecondTopLevelDeclarationGroupingType{Only,};
int topLevelDeclarationGroupingObject;
void TopLevelDeclarationGroupingCallable();
template<class T>struct DeclarationWrapperOwner{template<class U>static void Twice(U);template<class U>struct Nested{template<class V>static void ThreeTimes(V);};};
int beforeOnce;
template<class T>void Once(T){}
int beforeTwice;
template<class T>template<class U>void DeclarationWrapperOwner<T>::Twice(U){}
int beforeThreeTimes;
template<class T>template<class U>template<class V>void DeclarationWrapperOwner<T>::Nested<U>::ThreeTimes(V){}
int afterThreeTimes;
class DeclarationGroupingRules{
int firstField;

int secondField;
void FirstMethod();

void SecondMethod();
int functionPointerFactoryNeighborBefore;
void (*FunctionPointerFactory())();
int functionPointerFactoryNeighborAfter;
friend void DeclarationGroupingFriend();
int fieldFollowingDeclarationGroupingFriend;
operator bool() const = delete;
bool operator==(const DeclarationGroupingRules&) const=delete;
int fieldFollowingDeletedOperators;
struct FirstNestedType{int nestedField;};
struct FirstNestedType* nestedTypePointerField;
int fieldFollowingNestedTypePointer;
struct SecondNestedType{void NestedMethod();};
VeryLongDeclarationGroupingValueTypeName wrappedOnlyMovesInitializerToContinuation=declarationGroupingInitializerValueWithLongName;
int fieldFollowingWrappedInitializer;
int fieldBeforeLargeOperatorInitializer;
int largeOperatorInitializer=firstVeryLongDeclarationGroupingOperand+secondVeryLongDeclarationGroupingOperand+thirdVeryLongDeclarationGroupingOperand+fourthVeryLongDeclarationGroupingOperand+fifthVeryLongDeclarationGroupingOperand+sixthVeryLongDeclarationGroupingOperand;
int fieldAfterLargeOperatorInitializer;
std::array<DeclarationGroupingValue,5> isolatedValues={firstDeclarationGroupingValue,secondDeclarationGroupingValue,thirdDeclarationGroupingValue,fourthDeclarationGroupingValue,fifthDeclarationGroupingValue};
int fieldFollowingIsolatedValues;
using CompactDeclarationGroupingAlias=int;
using IsolatedDeclarationGroupingAlias=std::variant<FirstDeclarationGroupingAlternative,SecondDeclarationGroupingAlternative,ThirdDeclarationGroupingAlternative>;
using FollowingDeclarationGroupingAlias=int;
template<typename T> using IsolatedTemplatedDeclarationGroupingAlias=std::variant<T,FirstDeclarationGroupingAlternative,SecondDeclarationGroupingAlternative,ThirdDeclarationGroupingAlternative>;
using AliasFollowingIsolatedTemplate=int;
};

using BoolFunctionSignature=std::function<bool(const Value&,const Event&)>;
using ShortFunctionSignature=std::function<short(const Value&)>;

void UseFundamentalFunctionalCasts(int value){
Use(signed(value),unsigned(value),short(value),long(value));
Use(signed{value},unsigned{value},short{value},long{value});
}

void FormatAlphaNibble(char* text,unsigned int alpha){
constexpr char kHex[]="0123456789ABCDEF";
text[2]=kHex[(alpha >> 4)&0x0Fu];
text[3]=kHex[alpha&0x0Fu];
}

void IncrementSnapshotVersion(FrameState& frame){
frame.versions.snapshotVersion=++snapshotVersion_;
frame.versions.previousVersion=--snapshotVersion_;
}

template<std::size_t I,typename T>
consteval decltype(auto) ReferenceByIndex(T & value)noexcept{
return value.[:
nonstatic_data_members_of(
^^T,
std::meta::access_context::current()
).at(I)
:];
}

template<class T>
constexpr decltype(auto) StructuredBindingPackAt(T&& value){
auto&& [... members]=std::forward<T>(value);
return members...[0];
}

void WriteTraceStringFragments(TraceLog& trace) {
    trace.WriteFmt(
        TracePrefix::Diagnostics,
        RES_STR("layout_guide_sheet stats selected_cards=%zu callouts=%zu"),
        stats.selectedCards,
        stats.callouts
    );
}

void ExpectJoinedHexEscapeFragment() {
    EXPECT_THAT(
        output,
        testing::HasSubstr(
            "gpu.temp = 100,\xC2\xB0"
                "C,Core Temp\r\n"
        )
    );
}

void JoinAdjacentStringLiterals(){
ThrowError(
"The operation could not be completed because "
"the selected calculation is missing"
);
Use("first " "second " "third");
Use(L"wide " "text");
Use("view " "text"sv);
Use("\x41" "B");
Use("\x41" "G");
Use("\1" "7");
Use("\123" "7");
Use(R"(raw)" "tail");
}

void WriteLongTraceStringFragments(TraceLog& trace, const char* adapterName) {
    trace.WriteFmt(
        TracePrefix::GpuVendor,
        RES_STR(
            "adapter_candidate index=%u vendor_id=0x%04X device_id=0x%04X subsystem_id=0x%08X "
                "luid=0x%08x:0x%08x pci=%04X:%02X:%02X.%u vendor=%s match_rank=%d dedicated_gb=%.2f "
                "name=\"%s\""
        ),
        adapterIndex,
        adapterName
    );
}

std::string FormatLayoutEditTraceText(const LayoutEditTraceState& state, const char* captureText) {
    std::string trace = FormatText(
        "layout=\"%s\" editing=%s moving=%s modal_depth=%d tooltip_visible=%s tooltip_suppressed=%s "
            "tooltip_rect_valid=%s mouse_tracking=%s drag_active=%s capture=\"%s\"",
        state.config.display.layout.c_str(),
        Trace::BoolText(state.isEditingLayout),
        Trace::BoolText(state.isMoving),
        layoutEditModalUiDepth_,
        Trace::BoolText(dashboardTooltipOwner_ == DashboardTooltipOwner::LayoutEdit && dashboardTooltip_.Visible()),
        Trace::BoolText(layoutEditTooltipRefreshSuppressed_),
        Trace::BoolText(
            dashboardTooltipOwner_ == DashboardTooltipOwner::LayoutEdit && dashboardTooltip_.TargetRectValid()
        ),
        Trace::BoolText(layoutEditMouseTracking_),
        Trace::BoolText(layoutEditController_.HasActiveDrag()),
        captureText
    );
    return trace;
}

void ReportUnknownBenchmark(const std::string& firstArg,std::istream& input){
std::cerr<<"unknown benchmark \""<<firstArg<<"\"; supported benchmarks: "<<SupportedBenchmarkNames()<<"\n";
input>>firstBenchmarkName>>secondBenchmarkNameWithLongName>>thirdBenchmarkNameWithLongName>>fourthBenchmarkNameWithLongName;
std::cout<<std::left<<std::setw(18)<<name<<" total_ms="<<std::fixed<<std::setprecision(2)<<result.total.count()<<" per_iter_ms="<<result.perIteration.count()<<"\n";
}

void ReportTariffRetrieval(){
LOG_INFO()<<"Start retrieving cargo_tariffs for "<<zone_name<<' '<<tariff<<" and corp_client_id="<<corp_client_id<<" using experiment="<<pricing_experiment_name;
LOG_INFO()<<"Detailed cargo tariff retrieval payload="<<cargoTariffRetrievalResultWithAllFallbackAttributesAndMetadataIncludingExperimentOverrides;
}

void ExpectRectNoOverlap(RECT* rects){
if(check){
if(ready){
EXPECT_FALSE(IntersectRect(&intersection,&rects[i],&rects[j]))<<"rect "<<i<<" overlapped rect "<<j;
}
}
}

void ReserveGroupedRegionCount(LayoutEditActiveRegions& regions){
regions.Reserve(layoutResolver_->resolvedLayout_.cards.size()*4+layoutResolver_->layoutEditGuides_.size()+containerChildTargetCount+layoutResolver_->gapEditAnchors_.size()+layoutResolver_->widgetEditGuides_.size()+(layoutResolver_->staticEditableAnchorRegions_.size()+layoutResolver_->dynamicEditableAnchorRegions_.size())*2+layoutResolver_->staticColorEditRegions_.size()+layoutResolver_->dynamicColorEditRegions_.size());
}

void WriteMetricConfig() {
    const FilePath path = WriteTestConfig(
        "[metrics]\n"
            "nothing = 7,ignored,Overridden Placeholder\n"
            "cpu.load = *,%,Processor Load\n"
    );
}

void WriteInitialConfigText() {
    const std::string initialText = "[display]\r\n"
        "monitor_name = TL160ADMP03-0\r\n"
        "position = 258,117\r\n"
        "scale = 2\r\n"
        "\r\n"
        "[network]\r\n"
        "adapter_name = Wi-Fi\r\n"
        "\r\n"
        "[storage]\r\n"
        "drives = C\r\n";
}

struct OklabColor{
double l;
double a;
double b;
};

OklabColor MixOklab(OklabColor from,OklabColor to,double amount){
return OklabColor{
from.l + (to.l - from.l)* amount,
from.a + (to.a - from.a)* amount,
from.b + (to.b - from.b)* amount
};
}

OklchColor NormalizeOklch(const double* lightnessOverrideWithLongName,const double* chromaOverrideWithLongName,const double* hueOverrideWithLongName){
return OklchColor{
std::clamp(*lightnessOverrideWithLongName,0.0,1.0),
std::max(0.0,*chromaOverrideWithLongName),
std::clamp(*hueOverrideWithLongName,0.0,360.0),
};
}

enum class RuntimeConfigFieldValueKind{HexColor,Integer};

enum class ValueFormat : std::uint8_t {
    String, Integer, FloatingPoint, ColorHex, FontSpec, // text values
    FontSmall,
    FontFooter,
    FontClockTime,
    FontClockDate,

    // Card style anchors
    CardRadius,
    CardBorder,
};

enum class NestedEnumCommaValue {
FromParen=MakePair(1,2).value,
FromBrace=MakePair{1,2}.value,
FromTemplate=Pick<int,double>::value,
};

enum RuntimeMode{Default};

struct RuntimeConfigFieldDescriptor{RuntimeConfigFieldValueKind kind;const char* key;int keyLength;};

bool RuntimeConfigFieldEquals(
    const RuntimeConfigFieldDescriptor& field,
    const void* owner,
    const void* compareOwner
);
// Implemented by generated file build/cmake/generated/config/config_meta.generated.cpp.
std::span<const RuntimeConfigSectionDescriptor> RuntimeConfigSectionDescriptors();

std::vector<std::string> ParseIndentedStringList(const std::vector<ConfigLine>& lines, size_t& index, int parentIndent) {
    return {};
}

std::vector<std::string> ParseIndentedStringListDeclaration(const std::vector<ConfigLine>& lines, size_t& index, int parentIndent);

VeryLongNamespace::VeryLongReturnTypeNameWithNoTemplateArgumentsAndExtraSuffixBeyondLimit ParseLongNonTemplateReturnTypeWithExtremelyLongFunctionName(const std::vector<ConfigLine>& linesWithLongNameForFormatterFixture, size_t& indexWithLongNameForFormatterFixture);

std::vector<VeryLongReturnTypeNameWithTemplateArgumentsAndExtraSuffixBeyondLimit> ParseLongTemplateReturnTypeWithExtremelyLongFunctionName(const std::vector<ConfigLine>& linesWithLongNameForFormatterFixture, size_t& indexWithLongNameForFormatterFixture);

std::vector<std::string> ParseIndentedStringListWithSplitParameters(const std::vector<ConfigLine>& linesWithLongNameForFormatterFixture, size_t& indexWithLongNameForFormatterFixture, int parentIndentWithLongNameForFormatterFixture) {
    return {};
}

struct IncludeGroup {
    int priority;
};

void SortIncludeGroups(std::vector<IncludeGroup>& groups) {
    std::sort(groups.begin(), groups.end(), [](const IncludeGroup& left, const IncludeGroup& right) {
        return left.priority < right.priority;
    });
}

std::set<std::string> RequireSuffixGroup(
    const std::map<std::string, std::set<std::string>>& suffixGroups,
    std::string_view configPath,
    std::string_view groupName
) {
    return {};
}

struct ColorConfig {};

template <typename UpdateKeyFn>
void SaveBoardSectionDifferences(
    const BoardConfig& board,
    const BoardConfig* compareBoard,
    const std::string& sectionName,
    UpdateKeyFn& updateKey
) {
    DynamicSectionSaveContext<UpdateKeyFn> context{&board, compareBoard, &updateKey};
    updateKey(board, compareBoard, sectionName);
    const auto saveBoardKey =
        [&](const std::string& key, const std::string& currentValue, const std::string& compareValue)
    { if (compareBoard == nullptr || currentValue != compareValue) {
        updateKey(sectionName, key, currentValue);
    }
    };
}

template <typename FirstTemplateParameter,typename SecondTemplateParameter,typename ThirdTemplateParameter,typename FourthTemplateParameter> struct LongTemplateParameterHost{};

template <typename Result, typename...Args>
class FunctionRef<Result(Args...)> {
public:
    template <typename Callable>
    requires(!std::is_same_v<std::remove_cvref_t<Callable>, FunctionRef>&& std::is_invocable_r_v < Result, Callable&&, Args... >) FunctionRef(Callable&& callable) : context_(const_cast<void*>(static_cast<const void*>(std::addressof(callable)))), invoke_([](void* context, Args...args) -> Result {
        return(*static_cast<std::remove_reference_t<Callable>*>(context))(std::forward<Args>(args)...);
    }) {}

    Result operator()(Args...args) const {
        return invoke_(context_, std::forward<Args>(args)...);
    }

private:
    void* context_ = nullptr;
    Result (*invoke_)(void*, Args...) = nullptr;
};

template<typename... Args> void Forward(Args&&...args);

struct InitializerGeneralityWidget {
    InitializerGeneralityWidget(int value, int other);
    InitializerGeneralityWidget(int value, int other, int third);
    void Touch();

    int first_ = 0;
    int second_ = 0;
    int third_ = 0;
};

class FetcherWithGroupingImplementation {
public:
explicit FetcherWithGroupingImplementation(const two_phase::PrivateTicket& private_ticket, mem::SPtr<IRequester> shared_requester) : ExtremelyLongBaseInterfaceNameForFetcher{private_ticket}, shared_requester_with_grouping_and_caching_(std::move(shared_requester)) {}
};

struct DirectInitializedDeclarationGenerality {
ExtremelyLongDirectInitializerTypeNameForFormatterGeneralityAndMemberCoverage fieldWithBracedDirectInitializerName{value};
ResultType memberFunctionWithParenthesizedDeclarator(value);
};

void DirectInitializedDeclarationGeneralityLocals(){
ExtremelyLongDirectInitializerTypeNameForFormatterGeneralityAndMemberCoverage localWithBracedDirectInitializerName{value};
ExtremelyLongDirectInitializerTypeNameForFormatterGeneralityAndMemberCoverage localWithExtraParenDirectInitializerName((value));
}

void RepeatedCallApplications(){
call(init)(next)(more);
ConfigureWithModeratelyLongReceiverName(initialValue)(firstOptionWithModeratelyLongName)(secondOptionWithModeratelyLongName);
desc.add_options()("help,h","produce this help message")("print-config-schema","print config.yaml YAML Schema")("print-dynamic-config-defaults","print JSON object with dynamic config defaults")("config-vars,config_vars",po::value<std::string>(),"path to config_vars.yaml; if set, config_vars in config.yaml are ignored")("config-vars-override,config_vars_override",po::value<std::string>(),"path to an additional config_vars.yaml, which overrides vars of config_vars.yaml");
}

void PackedCommaSeparatedLists(){
const auto corp_clients_counter=std::unordered_map<std::optional<CorpClientId>,std::size_t>{{std::nullopt,1},{CorpClientId{"corp1"},1},{CorpClientId{"corp2"},2}};
const mem::SPtr<FetcherFromSlotUnusedOracle> fetcher=mem::MakeSPtr<impl::FetcherFromSlotUnusedOracle>(fetcher_from_slot,TwoPhaseFetcherCollector::CreateUnsafeNoneTwoPhase(fetcher_from_oracle));
auto called=MakeGenericResult<FirstTemplateParameterWithLongName,SecondTemplateParameterWithLongName>(firstArgument,secondArgument);
auto constructed=GenericResult<FirstTemplateParameterWithLongName,SecondTemplateParameterWithLongName>{firstArgument,secondArgument};
auto allocated=new GenericResult<FirstTemplateParameterWithLongName,SecondTemplateParameterWithLongName>(firstArgument,secondArgument);
MakeGenericCallable<FirstTemplateParameterWithLongName,SecondTemplateParameterWithLongName>(firstArgument)(secondArgument,thirdArgument);
Widget anExtremelyLongDirectInitializedVariableName(MakeFirstValueWithLongName(),MakeSecondValueWithLongName(),thirdValue);
Widget anExtremelyLongDirectInitializedVariableName{firstValueWithLongName,secondValueWithLongName,thirdValueWithLongName};
receiver.MakeGenericResult<FirstTemplateParameterWithLongName,SecondTemplateParameterWithLongName>(firstArgument,secondArgument).Use();
receiver->MakeGenericResult<FirstTemplateParameterWithLongName,SecondTemplateParameterWithLongName>(firstArgument,secondArgument)->Use();
Outer(MakeGenericResult<FirstTemplateParameterWithLongName,SecondTemplateParameterWithLongName>(firstArgument,secondArgument),other);
receiver.MakeGenericResult<FirstTemplateParameterWithLongName,SecondTemplateParameterWithLongName>(firstArgument,[](auto value){Prepare(value);return Finish(value);}).Use();
AnExceptionallyLongCallableNameWhoseFinalIdentifierHasExactlyTheWidthNeededToLeaveNoSpaceForAnEmptyArgumentListRightHere();
AnExceptionallyLongTypeNameWhoseFinalIdentifierHasExactlyTheWidthNeededToLeaveNoSpaceForAnEmptyBracedInitializerRightHere{};
}

auto PackedListReturnTailExpansion(){
return TwoPhaseFetcher::Create<impl::TwoPhaseFetcherImpl>(common::exp_utils::Experiments3{deps.experiments3},pricing_handle,std::move(slot_discounts),std::move(requester));
}

bool CallableNameWithLongParameterList(const FirstParameterType& firstParameter,const SecondParameterType& secondParameter){
Prepare();
return Finish();
}

class PackedConstructorInitializers{
PackedConstructorInitializers():GenericBase<FirstTemplateParameterWithLongName,SecondTemplateParameterWithLongName>(firstArgument,secondArgument),anExtremelyLongDirectInitializedMemberName{firstValueWithLongName,secondValueWithLongName,thirdValueWithLongName}{}
};

using PackedTemplateArguments=GenericContainerWithAnEspeciallyLongNameForTestingPackedTemplateArguments<FirstModeratelyLongTemplateArgument,SecondModeratelyLongTemplateArgument>;

void PackedListKindsAndBarriers(){
auto lambdaWithLongVariableName=[firstCapturedValueWithLongNameForPackedLayouts,secondCapturedValueWithLongName,thirdCapturedValueWithLongName]{return firstCapturedValueWithLongNameForPackedLayouts;};
auto longCommaExpressionVariableName=(firstCommaValueWithAnExtendedNameToPack,secondCommaValueWithAnExtendedName,thirdCommaValueWithAnExtendedName);
auto designatedValueWithLongVariableName=DesignatedValues{.first=firstValueWithLongName,.second=secondValueWithLongName,.third=thirdValueWithLongName};
CallWithCommentBarrier(firstArgument, // keep items separate
secondArgument,thirdArgument);
CallWithOpenerCommentBarrier( // keep items separate
firstArgument,
secondArgument,thirdArgument);
CallWithMultilineNonFinalItem([]{Prepare();Finish();},secondArgument,thirdArgument);
CallWithOperatorOnlyFinalItem(firstArgument,secondArgument,firstValueWithLongName+secondValueWithLongName+thirdValueWithLongName+fourthValueWithLongName);
CallWithPackedListTooWideToFit(firstArgumentWithExtremelyLongNameForThePackedListWidthTest,secondArgumentWithExtremelyLongNameForThePackedListWidthTest,thirdArgument);
CallWithUnavoidablyOverflowingItem(firstArgument,AnUnavoidablyLongAtomicArgumentWhoseSpellingAloneExceedsTheConfiguredColumnLimitAndMustNotBecomeAPackedSplitEvenWithAShortFirstItem);
}

void DeferredListCommaOwnership(){
Call([]{Prepare();Finish();},[](auto first,auto second){return first+second;},MakePair<First,Second>(first,second),Pair{first,second},(first,second),third);
}

auto DeferredListNestedLambdaBodies(){
return ExtraParams{
.first=[]{Prepare();Finish();},
.service_fee_percent=extra_params.service_fee().and_then([](const auto& service_fee){
return variant::Visit(service_fee,[](const tariff::ServiceFeePercent& percent)->std::optional<double>{return percent.fee_percent();},[](const auto&)->std::optional<double>{return std::nullopt;});
}),
.last=Call(first,second)
};
}

void DeferredCommaExpressionOwnership(){
([]{Prepare();Finish();}(),[]{return Call(first,second);}(),MakePair<First,Second>(first,second),Pair{first,second},(first,second),third);
}

void FinalBlockItemWithNestedCommaSuffix(){
Call(first,[]{Prepare();return Build();}(second,third));
}

void DesignatedInitializerAssignmentBreak(){
auto deps=Dependencies{{.delivery_corp_client_traits_fetcher=internal::delivery_corp_client_traits::MakeDeliveryCorpClientTraitsFetcher(dependencies)}};
Use(deps);
}

auto CompactDesignatedInitializerList(){
return DesignatedPair{.first=Convert(first),.second=Convert(second)};
}

auto DesignatedInitializerMultilineLiteral(){
return TextPair{.first="prefix",.second=R"q7(
line
)q7"};
}

auto SplitDesignatedInitializerListAtFieldBoundaries(){
return DesignatedPair{.first=Convert(first),.second=ConvertLongValue(secondDesignatedInitializerArgument,designatedInitializerConfiguration,designatedInitializerContext)};
}

auto SingleDesignatedInitializerTailExpansion(){
return SingleDesignatedValue{.value=ConvertLongValue(firstSingleDesignatedInitializerArgument,secondSingleDesignatedInitializerArgument,thirdSingleDesignatedInitializerArgument)};
}

auto PositionalInitializerTailExpansion(){
return PositionalPair{Convert(first),ConvertLongValue(firstPositionalInitializerArgument,secondPositionalInitializerArgument,thirdPositionalInitializerArgument)};
}

void SiblingInitializerRecordContexts(){
Use(Point{1,2},Point{firstInitializerRecordCoordinateWithLongName,secondInitializerRecordCoordinateWithLongName,thirdInitializerRecordCoordinateWithLongName});
Use(model::Point<int>{1,2},model::Point<int>{firstInitializerRecordCoordinateWithLongName,secondInitializerRecordCoordinateWithLongName,thirdInitializerRecordCoordinateWithLongName});
Use(context,Point{firstInitializerRecordCoordinateWithLongName,secondInitializerRecordCoordinateWithLongName,thirdInitializerRecordCoordinateWithLongName});
Use(MakeOptions({1,2}),Point{firstInitializerRecordCoordinateWithLongName,secondInitializerRecordCoordinateWithLongName,thirdInitializerRecordCoordinateWithLongName});
Use(Options{},[]{Prepare();Finish();});
Use([]{return 1;},Point{firstInitializerRecordCoordinateWithLongName,secondInitializerRecordCoordinateWithLongName,thirdInitializerRecordCoordinateWithLongName});
Use(Point{1,2},Wrap(Point{firstInitializerRecordCoordinateWithLongName,secondInitializerRecordCoordinateWithLongName,thirdInitializerRecordCoordinateWithLongName}));
Use(Point{1,2},Point{first,[]{Prepare();Finish();},last});
Use(Point{1,2},Point{{firstInitializerRecordCoordinateWithLongName,secondInitializerRecordCoordinateWithLongName,thirdInitializerRecordCoordinateWithLongName}});
}

void InitializerRecordDefaultArguments(Point first=Point{},Point second=Point{firstInitializerRecordCoordinateWithLongName,secondInitializerRecordCoordinateWithLongName,thirdInitializerRecordCoordinateWithLongName});

defs::internal::psp_pricer::BatchedOrderRoutePriceCorrectionRequirementNames ConvertRoutePriceCorrectionRequirementNames(const experiments3::cargo_pricing_batched_order_route_price_correction::BatchedOrderRoutePriceCorrectionRequirementNames& exp_value){
return {};
}

void UseGlobalQualifiedName(const ::first_global_qualification_namespace_with_long_name::second_global_qualification_namespace_with_long_name::FinalGlobalQualifiedTypeName& value);
void UseQualifiedTemplateName(const first_template_qualification_namespace_with_long_name::second_template_qualification_namespace_with_long_name::QualifiedTemplate<FirstTemplateArgument>& value);
void UseMultipleQualifiedBreaks(const first_qualification_namespace_with_extremely_long_name::second_qualification_namespace_with_extremely_long_name::third_qualification_namespace_with_extremely_long_name::FinalType& value);
using QualifiedMemberPointerAlias=int FirstMemberPointerNamespaceWithLongName::SecondMemberPointerClassWithLongName::*;

void UseSingleQualifiedName(const NamespaceWithAnIntentionallyLongNameForSingleQualificationCoverage::TypeWithAnIntentionallyLongNameForSingleQualificationCoverage& value);
using GlobalSingleQualifiedName=::NamespaceWithAnIntentionallyLongNameForSingleQualificationCoverage::TypeWithAnIntentionallyLongNameForSingleQualificationCoverage;
void UseLeadingGlobalScope(const ::GlobalTypeWithAnIntentionallyLongNameToVerifyThatLeadingScopeResolutionNeverCreatesAnIndependentBreakOpportunity& value);
void UseSingleQualifiedCall(){NamespaceWithAnIntentionallyLongNameForSingleQualificationCoverage::FunctionWithAnIntentionallyLongNameForSingleQualificationCoverage();}

int MemberPointerClassWithAnIntentionallyLongNameForSingleQualificationCoverage::*memberPointerWithAnIntentionallyExtendedName;
int FirstMemberPointerNamespaceWithLongName::SecondMemberPointerClassWithLongName::*memberPointerWithAnIntentionallyExtendedName;
int ::FirstMemberPointerNamespaceWithLongName::SecondMemberPointerClassWithLongName::*memberPointerWithAnIntentionallyExtendedName;
using LongScopedMemberPointerAlias=int MemberPointerNamespaceWithAnIntentionallyLongNameForQualification::MemberPointerClassWithAnIntentionallyLongNameForQualification::*;
using SingleMemberPointerTarget=int MemberPointerClassWithAnExtendedNameForSingleAbstractQualificationWhoseScopeFitsExactlyAtConfiguredColumnLimit::*;
int (FirstMemberPointerNamespaceWithLongName::SecondMemberPointerClassWithLongName::*memberFunctionPointerWithAnExtendedName)(int,int);
int MemberTemplate<FirstTemplateArgumentWithLongName,SecondTemplateArgumentWithLongName>::*memberPointerWithAnIntentionallyExtendedName;
using CompactMemberPointer=int Object::*;
int Object::*compactMember;

struct QualifiedTypeDeclaratorBreaks {
const ::loans::storages::CheckoutRemindersStorageComponent& loan_checkout_reminders_subscriptions_storage_component_;
virtual yango_wallet::agreements::AgreementSignEntry InsertAgreementWithAnIntentionallyLongNameThatRequiresBreakingAfterTheReturnType() const=0;
ProviderResult<model::providers::TransferDoc> ExecuteTransfer(const model::providers::ServiceTransferParams& service_transfer) const override;
void SetCommunications(communication_namespaces::with_a_long_name::CommunicationCollectionType communications_by_name_with_an_intentionally_long_name);
const ::first_global_qualification_namespace_with_long_name::second_global_qualification_namespace_with_long_name::FinalGlobalQualifiedTypeName& fallback_value;
};

typedef std::function<std::optional<double>(const TransformParams&, const double, const double)> ConfigurationTransformFunction;

struct QualifiedTypedefDeclaratorBreaks {
typedef const ::loans::storages::CheckoutRemindersStorageComponent* LoanCheckoutRemindersStorageComponentPointerAlias;
typedef ::loans::storages::CheckoutRemindersStorageComponent QualifiedCheckoutRemindersStorageComponentArrayAliasWithExtent[10];
typedef ::loans::storages::CheckoutRemindersStorageComponent FirstCheckoutRemindersStorageAlias,SecondCheckoutRemindersStorageAlias;
};

void QualifiedLocalDeclaratorBoundary() {
std::vector<application::configuration::ValueType> configured_values(configuration_values.size(),application::configuration::ValueType{});
Use(configured_values);
}

void DefaultQualifiedReference(int mode,const application::configuration::DefaultParameterValue& value=application::configuration::DefaultParameterValue::Default);
void DefaultQualifiedPointer(application::configuration::DefaultParameterValue* value=application::configuration::FindDefaultParameterValue());
void DefaultQualifiedUnnamed(const application::configuration::DefaultParameterValue& =application::configuration::DefaultParameterValue::Default);
void DefaultCallback(void(*callback)(int)=application::configuration::CreateCallback(first_configuration_value,second_configuration_value));

void NestedDefaultArguments() {
auto callback=[](int mode,const application::configuration::DefaultParameterValue& value=application::configuration::DefaultParameterValue::Default){return value;};
Use(callback);
}

template<typename Struct,typename Descriptor,typename Descriptor::Value(Struct::*member)>
struct MemberPointerTemplateParameter;

namespace NamespaceWithAnIntentionallyLongNameForSingleQualificationCoverage::NamespaceWithAnIntentionallyLongNameForNestedNamespaceCoverage {
void Function();
}

void TemplatedCallArgumentOverflow(){
mock.SetConfigValue<experiments3::CargoPricingBatchedOrderRoutePriceCorrectionWithSpecificExtendedExperimentConfiguration>({.requirement_names=experiments3::BatchedOrderRoutePriceCorrectionRequirementParamsWithSpecificExtendedConfiguration{}});
}

auto CommentAnnotatedAggregateItems(){
return {{.a=Make([]{First();Second();Third();}),.b=value},/*client_tariff_prices=*/client_tariff_prices,/*performer_tariff_prices=*/performer_tariff_prices,/*client_pricing_rules_exps=*/client_pricing_rules_exps};
}

struct Duration {
constexpr Duration(const std::chrono::seconds& seconds,const std::chrono::nanoseconds& nanos):seconds_(seconds),nanos_(nanos){}
};

struct CommentedDuration {
CommentedDuration(int seconds,int nanos)
// NOLINTNEXTLINE(test-check)
:seconds_(seconds),nanos_(nanos){}
};

struct SourceLocation {
constexpr SourceLocation(std::uint_least32_t line,std::string_view file_name,std::string_view function_name) noexcept:line_(line),line_digits_(DigitsBase10(line)),file_name_(file_name),function_name_(function_name){FillLineString();}
};

InitializerGeneralityWidget::InitializerGeneralityWidget(int value, int other) : first_(value), second_(other) {
    Touch();
}

InitializerGeneralityWidget::InitializerGeneralityWidget(int value, int other, int third) : first_(value), second_(other), third_(third) {
    Touch();
}

ConstructorBodyEconomyWidget::ConstructorBodyEconomyWidget(FirstExtremelyLongParameterType first,SecondExtremelyLongParameterType second) : first_(first),second_(second) {
    Use(first_,second_);
}

Widget::Widget(int first,int second,int third,int fourth):first_(first),second_(second),third_(third),fourth_(fourth){Use();}

PackedBracedInitializers::PackedBracedInitializers(int first,int second,int third) noexcept:first_{first},second_{second},third_{third}{Initialize();Verify();}

CommentedInitializers::CommentedInitializers(int first,int second):first_(first), // preserve split
second_(second){Use();}

PrefixCommentInitializers::PrefixCommentInitializers(int first,int second): // preserve split
first_(first),second_(second){Use();}

WideConstructorInitializers::WideConstructorInitializers():first_(firstInitializerValueWithAnExtremelyLongName),second_(secondInitializerValueWithAnExtremelyLongName),third_(thirdInitializerValueWithAnExtremelyLongName){}

PackedInitializerBodySuffix::PackedInitializerBodySuffix():firstValue_(firstInitializerValueWithAnExtremelyLongName),secondId_(secondInitializerValueWithAnExtremelyLongName){}

ExpandedFinalInitializer::ExpandedFinalInitializer(int first):first_(first),second_(BuildValue(firstArgumentWithAnExtremelyLongNameForTheConstructorFixture,secondArgumentWithAnExtremelyLongNameForTheConstructorFixture)){Use();}

CrossBlockInitializerList::CrossBlockInitializerList(const Dependencies& deps,const Account& account):first_initializer_with_long_name_(deps.first),second_initializer_with_long_name_(deps.second),account_([](const Account& value)->const Detail&{Validate(value);return GetDetail(value);}(account)),following_initializer_with_long_name_(deps.following),final_initializer_(deps.final){}

StringColumn::StringColumn(ColumnRef column)
    : ClickhouseColumn{impl::GetTypedColumn<StringColumn, NativeTyp>(column)}
{}

struct OverflowDeclaration {
    FunctionPtr destroy,writev,readv,setsockopt,get_base_stream,check_closed,poll,failed,timed_out,should_retry;
};

void FormatOverflowStream() {
    LOG()<<"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccccccccccccccccccc";
}

void FormatMultilineLiteralTail(){
const auto source=std::string{R"q4(
line one
line two
)q4"};
Use(prefix,std::string{R"q4(
final tail
)q4"});
Use(R"q4(
non-final
)q4",suffix);
auto nested=((R"q4(
nested
)q4"));
}

void FormatOverflowRawString() {
    EXPECT_EQ(formats::json::FromString(utils::statistics::ToSolomonFormat(GetStorage(),{})),formats::json::FromString(R"(
aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
)"));
}

enum class OverflowEnum {
    kAaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,//!< x
    /** @name Class 27 — Triggered Data Change Violation */
    kTriggeredDataChangeViolation=static_cast<std::int64_t>(SqlStateClass::kTriggeredDataChangeViolation),//!< 27000
    /** @name Class 28 — Invalid Authorization Specification */
    kInvalidAuthorizationSpecification=static_cast<std::int64_t>(SqlStateClass::kInvalidAuthorizationSpecification),//!< 28000
};

template <typename T>
requires(HasValue<T>) void UseShortRequires(T& value) {
    value.Use();
}

template <typename ExtremelyLongTemplateParameterNameForFormatterRequires>
requires(ExtremelyLongConceptNameWithoutLogicalOperatorsThatStillNeedsSubordinateLine<ExtremelyLongTemplateParameterNameForFormatterRequires>) void UseLongRequires(ExtremelyLongTemplateParameterNameForFormatterRequires& value) {
    value.Use();
}

template<typename RevertedEvent>
requires std::is_same_v<RevertedEvent,::handlers::PaymentAuthorizationRevertFullSucceededEvent>||std::is_same_v<RevertedEvent,::handlers::PaymentAuthorizationRevertPartialSucceededEvent>
FlowMetadata OnReverted(const FlowMetadata& flow_metadata,const Event& event){return {};}

template<typename T>
requires C<T>
void CompactRequiresPrefix();

template<typename T>
requires requires{typename T::type;}
struct ForcedRequiresPrefix;

template<typename T>
bool RequiresTernaryContinuation=requires{typename T::value_type;}?Enabled<T>:false;

template<typename T>
auto RequiresCallContinuation=requires{typename T::value_type;}(value);

template<typename T>
bool RequiresNestedContinuation=outer?requires{typename T::value_type;}?Enabled<T>:false:fallback;

template<typename T>
bool RequiresParenthesizedContinuation=(requires{typename T::value_type;});

template<typename T>
bool EmptyRequiresTernaryContinuation=requires{}?Enabled<T>:false;

void ConsumeMultilineRequiresItem() {
Consume(first,requires{first+second;typename Type::value_type;},third);
}

void InvokeEmptyAndNonemptyLambdas() {
[](){}();
[](){First();Second();}();
}

struct EmptyDeclaredType{}emptyDeclaredType;
enum class EmptyDeclaredEnum{}emptyDeclaredEnum;
struct NonemptyDeclaredType{int value;}nonemptyDeclaredType;

struct FirstDeclaredStruct{};
struct SecondDeclaredStruct{};
union FirstDeclaredUnion{};
union SecondDeclaredUnion{};
struct DeclaredStructValue{int number;}declaredStructValue;
union DeclaredUnionValue{int number;float decimal;}declaredUnionValue;

using BeforeGroupedConcept=int;
template<typename T>
concept GroupedConcept=true;
using AfterGroupedConcept=int;

template<typename T>
concept HasEnvironmentAndDependenciesConstructor=std::constructible_from<T,const EnvironmentHolder&,const handlers::Dependencies&>;
template<typename T>
concept HasQualifiedRequirements=std::same_as<typename T::template Rebind<Value>,ExpectedReboundValue>&&requires(T value){value.First();value.Second();};
template<typename T>
concept HasCommentedRequirements=/* constraint */std::constructible_from<T,const EnvironmentHolder&,const handlers::Dependencies&>;
namespace ExplicitNamespaceAssignmentBoundary=::first_namespace::second_namespace::third_namespace::fourth_namespace::last_namespace;

void QualifiedTemplatePrefixCohesion(){
Prepare();
AddFetcherHelpers<route_fetchers::ImpreciseTransportMotionModelFetcher<::clients::contractor_transport::TransportType::kPedestrian>>(fetcher_helpers);
Use<outer::Container<FirstArgument,SecondArgument>::template Rebind<AnotherArgument,YetAnotherArgument>::NestedType<LastArgument>>(first_argument,second_argument);
auto value=qualified_models::Container<FirstTemplateArgumentWithLongName,SecondTemplateArgumentWithLongName,ThirdTemplateArgumentWithLongName>{first_value,second_value};
}

using QualifiedTemplateAlias=qualified_models::Container<FirstTemplateArgumentWithLongName,SecondTemplateArgumentWithLongName,ThirdTemplateArgumentWithLongName>;
using DependentQualifiedTemplateAlias=typename outer::Container<FirstTemplateArgumentWithLongName,SecondTemplateArgumentWithLongName>::template Rebind<ThirdTemplateArgumentWithLongName,FourthTemplateArgumentWithLongName>::type;
void QualifiedTemplateParameter(const std::variant<first_namespace::FirstAlternativeTypeWithLongName,second_namespace::SecondAlternativeTypeWithLongName>& value);

void PreserveSiblingBlankLines(int value) {
First();

Second();
switch(value){case 1:First();

Second();break;}
}

template<typename T>
concept PreserveRequirementBlankLines=requires{typename T::first;

typename T::second;};

ExtremelyLongQualifiedNamespace::ExtremelyLongTemplate<FirstLongTemplateArgument,SecondLongTemplateArgument,ThirdLongTemplateArgument> storedSignatureComparisonValue;
ExtremelyLongQualifiedNamespace::ExtremelyLongTemplate<FirstLongTemplateArgument,SecondLongTemplateArgument,ThirdLongTemplateArgument> BuildSignatureComparisonValue();
static const auto kPricesTransformsToSkip=std::unordered_map<std::string,std::unordered_set<std::string>>{{"price1",{"transform1","transform2"}},{"price2",{}},{"price3",{"transform3","unexisting"}}};

using ConfigMetricAvailabilityResolver = bool (*)(std::string_view metricRef);
using RuntimeConfigDynamicItemVisitor = void (*)(void* context, std::string_view key, const void* item);
using RuntimeConfigEnsureDynamicItem = void* (*)(AppConfig& config, std::string_view key);
using RuntimeConfigFindDynamicItem = const void* (*)(const AppConfig& config, std::string_view key);
using MemberPointerAlias=ns::Value(ns::Owner::*);
using QualifiedFunctionTypeAlias=ns::Result(ns::Argument);
using TemplateFunctionTypeAlias=box::Result<int>(deep::ns::Argument);
using RuntimeConfigForEachDynamicItem =
    void (*)(const AppConfig& config, void* context, RuntimeConfigDynamicItemVisitor visitor);
using ZesDriver = void*;
using ZesInitFn = ZeResult(__cdecl*)(std::uint32_t);
using SlowPathCompilerCallModifierSpacingReproducer = VeryLongLevelZeroResultTypeName(__cdecl*)(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
typedef PDH_STATUS (*PdhAddEnglishCounterAFn)(PDH_HQUERY,LPCSTR,DWORD_PTR,PDH_HCOUNTER*);
typedef VeryLongReturnTypeNameForFunctionPointerGenerality (*VeryLongTypedefCallbackNameForFunctionPointerGenerality)(const Config& config, std::string_view name, RuntimeConfigDynamicItemVisitor visitor);
using DumpValues = std::vector<std::pair<std::string, std::string>>;
using LayoutEditParameter = ::LayoutEditParameter;
using TextLayoutResult = ::TextLayoutResult;
using LayoutEditActiveRegionPayload = std::variant<
    LayoutEditCardRegion,
    LayoutEditWidgetRegion,
    LayoutEditGuide,
    LayoutEditContainerChildReorderRegion,
    LayoutEditGapAnchor,
    LayoutEditWidgetGuide,
    LayoutEditAnchorRegion,
    LayoutEditColorRegion
>;

struct LayoutEditAnchorKey {
    LayoutEditWidgetIdentity widget;
    std::variant<
        LayoutEditParameter,
        LayoutMetricEditKey,
        LayoutCardTitleEditKey,
        LayoutNodeFieldEditKey,
        LayoutContainerChildOrderEditKey
    > subject = LayoutEditParameter::MetricListBarHeight;
    std::variant<
        LayoutEditParameter,
        LayoutMetricEditKey,
        LayoutCardTitleEditKey,
        LayoutNodeFieldEditKey,
        LayoutContainerChildOrderEditKey
    > fallbackSubject;
    int anchorId = 0;
};

std::variant<
    LayoutEditParameter,
    LayoutMetricEditKey,
    LayoutCardTitleEditKey,
    LayoutNodeFieldEditKey,
    LayoutContainerChildOrderEditKey
> DefaultLayoutEditSubject();
void UseLayoutEditSubject(
    std::variant<
        LayoutEditParameter,
        LayoutMetricEditKey,
        LayoutCardTitleEditKey,
        LayoutNodeFieldEditKey,
        LayoutContainerChildOrderEditKey
    > subject
);

struct LayoutEditAnchorBinding {
    LayoutEditAnchorKey key;
    int value = 0;
    AnchorShape shape = AnchorShape::Circle;
    std::optional<LayoutEditAnchorDragSpec> drag =
        LayoutEditAnchorDragSpec{AnchorDragAxis::Vertical, AnchorDragMode::AxisDelta, 1.0};
};

AppConfig LoadConfig(const FilePath& path, bool includeOverlay = true, const ConfigParseContext& context = {});

ColorConfig& MutableColorField(void* owner, const RuntimeConfigFieldDescriptor& field) {
    return *reinterpret_cast<ColorConfig*>(static_cast<char*>(owner) + field.offset);
}

void StartLenovoSnapshot(void* contextPtr) {
    // Parenthesized initializer ambiguity: this parser shape formats as a function declaration.
    ResultType functionDeclaration(FirstType* first, SecondType& second);
    // Use extra parentheses around expression operands when parenthesized initialization is intended.
    std::unique_ptr<LenovoServiceSnapshotThreadContext> context(
        static_cast<LenovoServiceSnapshotThreadContext*>(contextPtr)
    );
}

// Expression/template ambiguity: parenthesize value template arguments that could parse as type-like arguments.
using TemplateValueWorkaround = Box<(Size(A*B))>;

bool TemplateExpressionWorkaround() {
return (a < b) > (c);
}

bool RecursiveCallableTemplateDefault(X a,X b,X c,X d,X e,X f){
return (a+b)<Outer<Inner<c>>>(d);
return (a+b)<c>(d)<e>(f);
}

bool ParenthesizedCallableExpression(X a,X b,X c,X d){return ((a+b)<c)>(d);}

void DeclarationTemplateDefault(){
box<a,b> c;
(box<a),(b>c);
}

void QualifiedLiteralTemplateDeclarations(){
ns::Box<1,2,true,false> object;
ns::Box<1,2,true,false>* pointer=nullptr;
ns::Box<1,2,true,false>& reference=object;
ns::Outer<1,ns::Middle<2,ns::Inner<3,4>>> nested;
ns::Box<111111111111111111111111111111111111,222222222222222222222222222222222222,333333333333333333333333333333333333,444444444444444444444444444444444444> expanded;
}

HRESULT CreateWriteFactory(ComPtr<IDWriteFactory>& dwriteFactory_) {
    return DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(dwriteFactory_.ReleaseAndGetAddressOf())
    );
}

void GatewayAddress(const Gateway* gateway) {
    const sockaddr* address = gateway->Address.lpSockaddr;
    Use(address);
}

void ClearHistoryKeyIndexes(SystemSnapshot& snapshot) {
    for (uint16_t& encodedIndex : snapshot.retainedHistoryIndexByKey) {
        encodedIndex = 0;
    }
}

void test() {
    for (SpeexPreprocessState* state : preprocessStates) speex_preprocess_state_destroy(state);
}

void ManagedForEachLoop() {
    for each (SpeexPreprocessState* state in preprocessStates) speex_preprocess_state_destroy(state);
}

void ManagedReferenceSpacing(
    NativeType& nativeRef,
    NativeType&& nativeRvalueRef,
    NativeType*& pointerRef,
    NativeType*&& pointerRvalueRef,
    NativeType** pointerPointer,
    ManagedWidget^ managedHandle,
    ManagedWidget^% managedTrackingRef,
    ManagedWidget% managedReference
) {
    Use(
        nativeRef,
        nativeRvalueRef,
        pointerRef,
        pointerRvalueRef,
        pointerPointer,
        managedHandle,
        managedTrackingRef,
        managedReference
    );
}

ResultType NamespaceDeclaratorReferenceSpacing(FirstType* first, SecondType& second);

void LocalDeclaratorReferenceSpacing() {
    int directProduct((a*b), (c&d));
    ResultType local(FirstType* first, SecondType& second);
}

void RegisterStaticTextAnchor(
    const RenderRect&,
    const std::string&,
    TextStyleId,
    const TextLayoutOptions&,
    const LayoutEditAnchorBinding&,
    std::optional<LayoutEditParameter>,
    LayoutEditTargetOutline
) override {}

struct NetworkFooterWidgetConfig {
    int bottomGap{};  // config_meta: policy=non_negative_int

    bool operator==(const NetworkFooterWidgetConfig& other) const = default;
};

ColorConfig EmptyColor() {
    return {};
}

std::string_view LayoutNodeFieldEditTitle(const LayoutNodeFieldEditKey& key) {
    const LayoutNodeFieldEditDescriptor* descriptor = FindLayoutNodeFieldEditDescriptor(key);
    return descriptor != nullptr ? FindLocalizedText(descriptor->titleKey) : std::string_view{};
}

void TrackCoveredParameters() {
    std::array<bool, static_cast<size_t>(LayoutEditParameter::Count)> coveredColorParameters{};
    Use(coveredColorParameters);
}

void TrackFeatureLevels() {
    const std::array<D3D_FEATURE_LEVEL, 4> preferredFeatureLevels{
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    const std::array<D3D_FEATURE_LEVEL, 3>
        fallbackFeatureLevels{D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
    const std::array<VeryLongTemplateArgumentNameForDirectInitialization, 3>
        bracedFeatureLevels{D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
    std::vector<int> lengthInitializedValues((featureLevelCount));
    Use(preferredFeatureLevels, fallbackFeatureLevels, bracedFeatureLevels, lengthInitializedValues);
}

DashboardApp::DashboardApp(
    const DiagnosticsOptions& diagnosticsOptions,
    bool bringToFrontOnRun
) :
    renderer_(trace_),
    diagnosticsOptions_(diagnosticsOptions),
    layoutEditController_(*this),
    shellUi_(std::make_unique<DashboardShellUi>(*this)),
    bringToFrontOnRun_(bringToFrontOnRun)
{
    renderer_.SetLiveAnimationEnabled(true);
}

TraceTimingScope::TraceTimingScope(TraceTimingScope&& other) noexcept :
    collector_(std::exchange(other.collector_, nullptr)),
    trace_(std::exchange(other.trace_, nullptr)),
    operation_(std::exchange(other.operation_, nullptr)),
    startedAt_(std::exchange(other.startedAt_, 0)) {}

int CardChromeWidget::PreferredHeight(const WidgetHost&) const {
    return 0;
}

HANDLE OpenProbe(FilePath probePath) {
    HANDLE probe = CreateFileA(
        probePath.string().c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        CREATE_NEW,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
        nullptr
    );
    return probe;
}

HBITMAP CreateBitmap(BITMAPINFOHEADER header) {
    void* bits = nullptr;
    HBITMAP colorBitmap =
        CreateDIBSection(nullptr, reinterpret_cast<BITMAPINFO*>(&header), DIB_RGB_COLORS, &bits, nullptr, 0);
    return colorBitmap;
}

void AttachCalloutBubble(Callout& callout, LayoutGuideSheetExitSide side) {
    callout.bubbleAttachment = RenderPoint{
        side == LayoutGuideSheetExitSide::Left ? callout.bubbleRect.right : callout.bubbleRect.left,
        callout.bubbleRect.Center().y
    };
}

double MaxSegmentGap(double totalSweep, double minSegmentSweep, int segmentCount) {
    const double maxSegmentGap = (std::max)(
        0.0,
        (totalSweep - (minSegmentSweep * static_cast<double>(segmentCount))) / static_cast<double>(segmentCount - 1)
    );
    return maxSegmentGap;
}

bool HasIdentifier(std::string identifier) {
    return !identifier.empty();
}

unsigned int PackedRgb(ColorConfig color) {
    return (color.rgba >> 8) & 0xFFFFFFu;
}

void ReturnOnly() {
    return;
}

void BuildSearchContext() {
    struct SearchContext {
        const AppConfig* config = nullptr;
        const std::optional<TargetMonitorInfo>* configuredMonitor = nullptr;
        DisplayMenuOption* options = nullptr;
        size_t capacity = 0;
        size_t count = 0;
        bool hasConfiguredWallpaper = false;
        bool isConfiguredAtOrigin = false;
    } context{&config, &configuredMonitor, options, capacity, 0, hasConfiguredWallpaper, isConfiguredAtOrigin};
    Use(context);
}

void StandaloneLockBlock(LightweightMutex& mutex, TelemetryUpdate update, HWND hwnd) {
    {
        const LightweightMutexLock lock(mutex);
        pendingTelemetryUpdate_ = update;
        hasPendingTelemetryUpdate_ = true;
    }

    if (hwnd != nullptr) {
        PostMessageA(hwnd, kTelemetryUpdateMessage, 0, 0);
    }
}

int UnarySigns(int value) {
    int negative = -value;
    int positive = +value;
    return value - -negative + +positive + (-negative) + (+positive);
}

void DiscardServiceResult(Service& service) {
    (void)StopServiceIfRunning(service.Get());
}

void StructuredBindingLoop(const BoardSelections& resolvedSelections) {
    auto [parameterLine, descriptionLine] = SplitTooltipLines(tooltipText);
    for (const auto& [logicalName, sensorName] : resolvedSelections.boardFanSensorNames) {
        Use(logicalName, sensorName);
    }
}

void SelectFocusedMetric(const FocusKey* focusKey) {
    if (const auto* metricCandidate = std::get_if<LayoutMetricEditKey>(&*focusKey)) {
        metricKey = *metricCandidate;
    }
}

LayoutEditTreeLeaf* FindFocusedLeaf(LayoutEditTreeNode& node, const FocusKey& focusKey) {
    if (node.leaf.has_value() && MatchesLayoutEditFocusKey(node.leaf->focusKey, focusKey)) {
        return &(*node.leaf);
    }
    return nullptr;
}

double NormalizeAngleCandidate(double angleDegrees) {
    for (double candidate : {angleDegrees - 360.0, angleDegrees + 360.0}) {
        Use(candidate);
    }
    return angleDegrees;
}

void PreserveWin32BooleanMacros(){int falseValue=FALSE;int trueValue=TRUE;bool standardFalse=false;bool standardTrue=true;}

auto CompactEmptyBraceTernary(bool empty, std::string value){return empty?std::string{}:value;}

bool InitializerPaddingInSplitContext(const LayoutEditOverlayOwner& owner,const DragState& drag){return owner.childIndex==drag.currentIndex&&MatchesLayoutContainerEditKey(LayoutContainerEditKey{owner.key.editCardId,owner.key.nodePath},LayoutContainerEditKey{drag.key.editCardId,drag.key.nodePath});}

const auto noLookup = [](std::string_view) -> std::optional<ColorConfig> { return std::nullopt; };

const auto handled = [&result](INT_PTR value) {
    result = value;
    return true;
};

const auto appendTrialLeaders = [&](
    std::vector<TrialLeader>& leaders,
    const std::vector<size_t>& plannedIndexes,
    LayoutGuideSheetExitSide side,
    const LayoutGuideSheetCardPlacement& placement,
    const BlockLayout& block
) {
    Use(leaders, plannedIndexes, side, placement, block);
};

const auto appendEmbeddedRect = [&](
    const RenderRect& rect,
    const std::vector<LayoutEditOverlayOwner>* owners,
    LayoutEditOverlayAffordanceLayer artifactLayer
) {
    if (rect.IsEmpty()) {
        return;
    }
    Use(owners, artifactLayer);
};

const auto preserveLambdaSeparator = []() {
    FirstStep();

    SecondStep();
};

const auto findSectionIndex = [&lines](const std::string& sectionName) -> size_t {
    for (size_t i = 0; i < lines.size(); ++i) {
        if (Trim(lines[i]) == sectionName) {
            return i;
        }
    }
    return lines.size();
};

const auto ensureSection = [&lines, &findSectionIndex, shape](const std::string& sectionName) -> size_t {
    const size_t existingIndex = findSectionIndex(sectionName);
    if (existingIndex < lines.size()) {
        return existingIndex;
    }
    return lines.size();
};

const auto updateKey = [&lines, &ensureSection, &ensureSectionAfter, &findSectionEnd, shape]
    (const std::string& sectionName, const std::string& key, const std::string& value)
{
    Use(sectionName, key, value);
};

const auto ensureSectionAfter = [&lines, &findSectionIndex, shape]
    (const std::string& sectionName, const std::string& afterSectionName) -> size_t
{
    const size_t existingIndex = findSectionIndex(sectionName);
    if (existingIndex < lines.size()) {
        return existingIndex;
    }

    const size_t afterIndex = findSectionIndex(afterSectionName);
    return afterIndex;
};

const auto guideSheetLookup = [&config, activeTheme, &colorsSection]
    (std::string_view name) -> std::optional<ColorConfig>
{
    if (std::optional<ColorConfig> themeColor = FindThemeToken(*activeTheme, name); themeColor.has_value()) {
        return themeColor;
    }
    return FindColorFieldByKey(RuntimeConfigFields(colorsSection), &config.layout.colors, name);
};

void LambdaGeneralityCases(int left, int right) {
    auto twoParameterLambda = [](int left, int right) { return left + right; };
    auto twoCaptureLambda = [left, right](int value) { return left + right + value; };
    auto splitParameterSingleStatementLambda = [](
        int firstParameterNameThatForcesTheLambdaHeaderToSplit,
        int secondParameterNameThatForcesTheLambdaHeaderToSplit,
        int thirdParameterNameThatForcesTheLambdaHeaderToSplit
    ) { return 1; };
    Call(firstVeryLongArgumentName, [](int value) { Prepare(value); return value + 1; }, secondVeryLongArgumentName);
    std::array callbacks{firstVeryLongArgumentName, [](int value) { Prepare(value); return value + 1; }, secondVeryLongArgumentName};
    Use(twoParameterLambda, twoCaptureLambda, splitParameterSingleStatementLambda, callbacks);
}

void LambdaCapturePrefixDepthPreference(){
Visit([&](const VeryLongQualifiedCalculationTypeForLambdaCaptureBreakPreferenceWithAdditionalSuffixMoreId& calculation){Use(calculation);});
Visit([&context](const VeryLongQualifiedCalculationTypeForLambdaCaptureBreakPreferenceWithAdditionalSuffixMoreId& calculation){Use(calculation);});
}

void TrailingReturnTypeBreaks(){
Use([first_capture_with_long_name,second_capture_with_long_name,third_capture_with_long_name]() -> namespace_name::Result {Prepare();return {};});
Use([first_capture_with_long_name,second_capture_with_long_name,third_capture_with_long_name]() noexcept -> const namespace_name::Result& {Prepare();return result;});
auto callback=[first_capture_with_long_name,second_capture_with_long_name,third_capture_with_long_name]<class T>(T value) -> namespace_name::Result<T> {Prepare();return value;};
}

auto ComputeResultForCurrentEnvironmentAndConfiguration() noexcept -> const application::dispatch::DeliveryCalculationResult& {Prepare();return result;}
auto ComputeResultForCurrentEnvironmentAndConfiguration() noexcept -> const application::dispatch::DeliveryCalculationResult&;
struct TrailingReturnMembers {
virtual auto ComputeResultForCurrentEnvironmentAndConfiguration() const noexcept -> const application::dispatch::DeliveryCalculationResult& =0;
auto ComputeResultForCurrentEnvironmentAndConfiguration() & -> const application::dispatch::DeliveryCalculationResult& =delete;
};
template<class T>auto ComputeResultForCurrentEnvironmentAndConfiguration(T value) -> typename application::dispatch::Result<T> requires C<T> {Prepare();return value;}
template<class T>auto ComputeResultForCurrentEnvironmentAndConfiguration() noexcept -> typename application::dispatch::Result<T> requires C<T> {Prepare();return value;}

void LongLambdaCapturePrefixMaySplit(){
Visit([firstVeryLongLambdaCaptureName,secondVeryLongLambdaCaptureName,thirdVeryLongLambdaCaptureName,fourthVeryLongLambdaCaptureName](int value){Use(value);});
}

void LambdaHeaderCompactTailIsDepthIndependent(){
Visit(first,[](const IntentionallyLongParameterTypeForLambdaHeaderScoring& first,const IntentionallyLongParameterTypeForLambdaHeaderScoring& second){Use(first,second);});
Visit(first,Wrapper(([](const IntentionallyLongParameterTypeForLambdaHeaderScoring& first,const IntentionallyLongParameterTypeForLambdaHeaderScoring& second){Use(first,second);})));
}

auto FinalLambdaHeaderBreakSelectedByScore(const handlers::PriceFor& priceFor) -> const Sequence& {
return variant::Visit(plan, [](const Sequence& sequence) -> const Sequence& { return sequence; }, [&](const AgentPlan& agentPlan) -> const Sequence& { switch (priceFor) { case handlers::PriceFor::kPerformer: { return agentPlan.performerSequence; } case handlers::PriceFor::kClient: { return agentPlan.clientSequence; } } });
}

bool NestedLambdaContinuationIndent() {
return firstOperandWithAnExtremelyLongNameThatNearlyConsumesTheEntireConfiguredLineWidthAllByItselfAndKeepsGoing!=nullptr&&call(firstArgumentWithEnoughLength,secondArgumentWithEnoughLength,[&](const Node& node){return node.member!=nullptr;});
}

void PreferRootBreakBeforeNestedLambda() {
const bool found=model.nodes!=nullptr&&std::any_of(model.nodes->begin(),model.nodes->end(),[&](const FormatBreakNode& node){return node.declarationValueOwner!=nullptr&&DeclarationScopeItem(node.declarationValueOwner)==item;});
const bool matched=context.currentValue==expectedValue||std::any_of(context.values->begin(),context.values->end(),[&](const FormatBreakNode& node){return node.declarationValueOwner!=nullptr&&DeclarationScopeItem(node.declarationValueOwner)==item;});
}

bool StructuralLogicalBreak() {
return node.kind==SyntaxNodeKind::PreprocCall&&SyntaxNodeKindFromPreprocessorDirectiveLine(TrimLeadingWhitespace(node.text))==SyntaxNodeKind::PreprocessorDirectivePragma;
}

void StructuralAssignmentBreak() {
if(condition){chain->chainKind=(operatorKind==SyntaxNodeKind::LessLess||operatorKind==SyntaxNodeKind::GreaterGreater)?FormatBreakChainKind::StreamBeforeOperator:FormatBreakChainKind::AfterOperator;}
}

void StructuralMemberChainBreak() {
if(condition){bucket.events.erase(bucket.events.begin(),bucket.events.begin()+static_cast<std::ptrdiff_t>(bucket.firstEvent));}
}

auto CrossBlockMemberChain(auto source){
return source.and_then([&](auto kind){switch(kind){case 1:return 1;default:return 0;}}).value_or(0);
}

auto CrossBlockBinaryChain(auto first,auto last){
return first+Invoke([&]{switch(first){case 1:return 1;default:return 0;}})+last;
}

void CrossBlockStreamChain(auto& stream,auto tail){
stream<<Invoke([&]{switch(tail){case 1:return 1;default:return 0;}})<<tail;
}

auto CrossBlockNestedTernaryChain(bool firstCondition,bool secondCondition,auto second,auto last){
return firstCondition?Invoke([&]{switch(second){case 1:return 1;default:return 0;}}):secondCondition?second:last;
}

auto CrossBlockSingleTernary(bool condition,auto last){
return condition?Invoke([&]{switch(last){case 1:return 1;default:return 0;}}):last;
}

auto CrossBlockCommaChain(auto first,auto last){
return(first,Invoke([&]{switch(last){case 1:return 1;default:return 0;}}),last);
}

constexpr int kPrimaryFlag = 1;
constexpr int kSecondaryFlag = 2;
constexpr int kTertiaryFlag = 4;
// Shared telemetry update cadence and live animation duration.
inline constexpr auto kTelemetryRefreshInterval = std::chrono::milliseconds(250);
inline constexpr double kTelemetryRefreshIntervalSeconds =
    static_cast<double>(kTelemetryRefreshInterval.count()) / 1000.0;

constexpr std::string_view kRuntimePlaceholderMetricId = "nothing";

const MetricDefinitionConfig kRuntimePlaceholderMetricDefinition{
    std::string(kRuntimePlaceholderMetricId),
    MetricDisplayStyle::Scalar,
    false,
    1.0,
    "",
    "Nothing"
};

constexpr FormatTableRow kFormatRows[] = {
    {
        "alpha.metric.row.with.extra.detail.and.column.limit.coverage",
        100,
        200,
        kPrimaryFlag | kSecondaryFlag | kTertiaryFlag
    },
    {"beta.metric.row.with.extra.detail", 300, 400, kPrimaryFlag | kTertiaryFlag},
    {"gamma.metric.row", 500, 600, kSecondaryFlag}
};

constexpr FormatTableRow kInitializerChainRows[] = {{
    "chain.metric.row.with.extra.detail",
    100,
    200,
    firstInitializerFlagWithVeryLongName |
        secondInitializerFlagWithVeryLongName |
        thirdInitializerFlagWithVeryLongName |
        fourthInitializerFlagWithVeryLongName
}};

static constexpr OutputPath kOutputPaths[] = {
    {
        &DiagnosticsOptions::trace,
        &DiagnosticsOptions::tracePath,
        &DiagnosticsSession::tracePath_,
        kDefaultTraceFileName
    },
    {&DiagnosticsOptions::dump, &DiagnosticsOptions::dumpPath, &DiagnosticsSession::dumpPath_, kDefaultDumpFileName},
    {
        &DiagnosticsOptions::screenshot,
        &DiagnosticsOptions::screenshotPath,
        &DiagnosticsSession::screenshotPath_,
        kDefaultScreenshotFileName
    }
};

void DiagnosticsSession::ResolveOutputPathMember(const OutputPath& outputPath, const FilePath& workingDirectory) {
    this->*outputPath.resolvedPath =
        ResolveDiagnosticsOutputPath(workingDirectory, options_.*outputPath.configuredPath, outputPath.defaultFileName);
}

inline constexpr std::array<ColorDialogControls, 4> kColorDialogControls = {{
    {IDC_LAYOUT_EDIT_COLOR_RED_LABEL, IDC_LAYOUT_EDIT_COLOR_RED_EDIT, IDC_LAYOUT_EDIT_COLOR_RED_SLIDER, "red"},
    {IDC_LAYOUT_EDIT_COLOR_GREEN_LABEL, IDC_LAYOUT_EDIT_COLOR_GREEN_EDIT, IDC_LAYOUT_EDIT_COLOR_GREEN_SLIDER, "green"},
    {IDC_LAYOUT_EDIT_COLOR_BLUE_LABEL, IDC_LAYOUT_EDIT_COLOR_BLUE_EDIT, IDC_LAYOUT_EDIT_COLOR_BLUE_SLIDER, "blue"},
    {IDC_LAYOUT_EDIT_COLOR_ALPHA_LABEL, IDC_LAYOUT_EDIT_COLOR_ALPHA_EDIT, IDC_LAYOUT_EDIT_COLOR_ALPHA_SLIDER, "alpha"}
}};

int kAlignedAssignment = 1;
int kMuchLongerAlignedAssignment = 2;
int kTrailingComment = 1;// short
int kMuchLongerTrailingComment = 2;// long

using AlignedTrailingCommentTypes=TypeList<//
ShortType,//
MuchLongerType//
>;

auto kAlignedTrailingCommentRows=RowList{
{1,2},// first
{100,200}// second
};

int kSingleTrailingComment=0;// single

int kTrailingCommentOverflowAnchorWithAnIntentionallyLongDeclarationName=1;// x
int kTrailingCommentOverflowShort=2;// this explanation prevents alignment within the line limit

class BenchmarkLikeHost {
    bool ApplyMetricListOrder(
        const LayoutEditWidgetIdentity& widget,
        const std_fixture::vector<std_fixture::string>& metricRefs
    ) override {
        return true;
    }
};

int ShortNonEmpty() {
    return 1;
}

int CompactSingleStatementFunction(){return 1;}

struct CompactCallableBodyFixture{
CompactCallableBodyFixture(int value):value_(value){Initialize();}
~CompactCallableBodyFixture(){Cleanup();}
int Value() const{return value_;}
int value_;
};

int VeryLongSingleStatementFunctionNameThatCannotKeepItsCompleteCallableHeaderAndBodyOnOnePhysicalLine(int value){return value;}

void MultiStatementFunctionBody(){First();Second();}

void BlockBearingSingleStatementFunctionBody(bool ready){if(ready){Run();}}

void CommentedSingleStatementFunctionBody(){
// body comment
Run();
}

auto compactSingleStatementLambda=[](){return 1;};
auto blockBearingSingleStatementLambda=[](){if(ready){Run();}};

void EmptyFunction() {}
void EmptyFunctionPairA() {} void EmptyFunctionPairB() {}

std::string FormatNamedMenuLabel(std::string_view name, std::string_view description) {
    return description.empty() ? std::string(name) : FormatText(
        "%.*s - %.*s",
        static_cast<int>(name.size()),
        name.data(),
        static_cast<int>(description.size()),
        description.data()
    );
}

void PreferOuterCallBreakForSingleArgument() {
    UseAVeryLongCallNameThatPushesPastLimit(
        DecorateValueWithLongName(firstValueWithLongName && SingleArgumentCallWithLongName(secondValueWithLongName))
    );
}

void BuildLayoutEditTargetMenuLabel(const LayoutEditTarget* layoutEditTarget) {
    if (layoutEditTarget != nullptr) {
        if (layoutEditTarget->payload.kind == TooltipPayloadKind::LayoutEdit) {
            std::string label;
            const auto focusKey = TooltipPayloadFocusKey(layoutEditTarget->payload);
            if (label.empty() && focusKey.has_value() && std::holds_alternative<LayoutCardTitleEditKey>(*focusKey)) {
                label = BuildLayoutEditMenuLabel("card title");
            } else if (label.empty() && focusKey.has_value() && std::get_if<LayoutNodeFieldEditKey>(&*focusKey) != nullptr) {
                label = BuildLayoutEditMenuLabel("node field");
            } else if (label.empty() && focusKey.has_value() && std::holds_alternative<LayoutContainerEditKey>(*focusKey)) {
                label = BuildLayoutEditMenuLabel("layout container");
            }
        }
    }
}

void BuildTitlebarTooltipControls() {
    const struct {
        DashboardTitlebarTooltipControl control;
        const RECT& rect;
    } controls[] = {
        {DashboardTitlebarTooltipControl::Close, closeRect},
        {DashboardTitlebarTooltipControl::Display, displayRect},
        {DashboardTitlebarTooltipControl::EditLayout, editLayoutRect},
        {DashboardTitlebarTooltipControl::Layout, layoutComboRect},
        {DashboardTitlebarTooltipControl::Theme, themeComboRect},
        {DashboardTitlebarTooltipControl::AppMenu, appMenuRect}
    };
}

const char* SelectRevertLabel(bool isFontsSection, bool isThemeSection, bool isLayoutSection, bool isMetricsSection) {
    return isFontsSection ? "Revert Font Changes" :
        isThemeSection ? "Revert Theme" :
        isLayoutSection ? "Revert Layout" :
        isMetricsSection ? "Revert Metrics" :
        "Revert Field";
}

std::string SelectCurrentSensorName(const BoardConfig& board, const std::string& logicalName) {
    auto currentIt = board.temperatureSensorNames.find(logicalName);
    const std::string currentValue =
        currentIt != board.temperatureSensorNames.end() && !currentIt->second.empty() ? currentIt->second : logicalName;
    return currentValue;
}

const char* LayoutGuideAxisSizingKey(const LayoutGuide* guide) {
    return guide->axis == LayoutGuideAxis::Horizontal ?
        "overview_horizontal_sizing_guide" : "overview_vertical_sizing_guide";
}

std::optional<double> LayoutEditAnchorValue(const LayoutEditAnchor* anchor) {
    return LayoutEditAnchorParameter(anchor->key).has_value() ?
        std::optional<double>(static_cast<double>(anchor->value)) : std::nullopt;
}

size_t SelectConfigSectionStart(const std::string& sectionName) {
    size_t sectionStart = sectionName == "[gpu]" ? ensureSectionAfter(sectionName, "[display]") :
        sectionName == "[network]" ? ensureSectionAfter(sectionName, "[gpu]") :
        sectionName == "[storage]" ? ensureSectionAfter(sectionName, "[network]") :
        sectionName == "[board]" ? ensureSectionAfter(sectionName, "[storage]") :
        sectionName == "[metrics]" ? ensureSectionAfter(sectionName, "[board]") :
        ensureSection(sectionName);
    return sectionStart;
}

bool IsNamedColorField(const RuntimeConfigFieldDescriptor& field, std::string_view name) {
    if (
        field.kind == RuntimeConfigFieldValueKind::HexColor &&
        std::string_view(field.key, field.keyLength) == name &&
        field.keyLength > 0
    ) {
        return true;
    }
    return false;
}

void EnumerateUninstallChildren(HKEY uninstallKey) {
    while (
        RegEnumKeyExA(uninstallKey, index, childName, &childNameLength, nullptr, nullptr, nullptr, nullptr) ==
            ERROR_SUCCESS
    ) {
        ++index;
    }
}

bool HasMissingReflectionMembers(const ReflectionContext* context) {
    if (
        context->initializeMethod == nullptr ||
        context->getCurrentMethod == nullptr ||
        context->titleProperty == nullptr ||
        context->valueProperty == nullptr
    ) {
        return true;
    }
    return false;
}

void CollectLayoutEditHighlights(const DashboardOverlayState& overlayState, const LayoutEditAnchorRegion& region) {
    const auto* special = std::get_if<LayoutEditSelectionHighlightSpecial>(&*overlayState.selectedTreeHighlight);
    if (MatchesLayoutEditSelectionHighlight(*overlayState.selectedTreeHighlight, region.key) || (
        special != nullptr &&
        *special == LayoutEditSelectionHighlightSpecial::AllTexts &&
        LayoutEditAnchorParameter(region.key).has_value() &&
        IsFontEditParameter(*LayoutEditAnchorParameter(region.key))
    )) {
        appendHighlight(region, true);
    }
}

void SplitOperatorChainPartsLineByLine() {
    const int metricValue = firstValue + builder.WithSource(sourceValue).BuildMetricValue(
        firstArgumentWithLongName,
        secondArgumentWithLongName,
        thirdArgumentWithLongName,
        fourthArgumentWithLongName
    );
    const int chainedMetricValue = firstValue +
        builder.WithSource(sourceValue).BuildMetricValue(
            firstArgumentWithLongName,
            secondArgumentWithLongName,
            thirdArgumentWithLongName,
            fourthArgumentWithLongName
        ) +
        finalValueWithLongName;
    const bool found = currentKey == layoutLookupTable[ComputeLayoutKeyIndex(
        firstKeyPartWithLongName,
        secondKeyPartWithLongName,
        thirdKeyPartWithLongName,
        fourthKeyPartWithLongName,
        fifthKeyPartWithLongName
    )];
    const RenderRect bounds = baseBounds | RenderRect{
        leftValueWithLongName,
        topValueWithLongName,
        rightValueWithLongName,
        bottomValueWithLongName,
        extraValueWithLongName,
        finalValueWithLongName
    };
    const bool loaded = !LoadString(values, DumpKey(historyPrefix, ".series_ref"), history.seriesRef, error) ||
        !LoadDoubleArrayField(values, DumpKey(historyPrefix, ".samples"), history.samples, error) ||
        !LoadDoubleArrayField(
            values,
            DumpKey(historyPrefix, ".throughput_live_samples"),
            history.throughputLiveSamples,
            error
        ) ||
        !LoadDouble(values, DumpKey(historyPrefix, ".throughput_bucket_total"), history.throughputBucketTotal, error) ||
        !LoadUnsigned(
            values,
            DumpKey(historyPrefix, ".throughput_bucket_sample_count"),
            history.throughputBucketSampleCount,
            error
    );
}

void GoogletestMemberCallChains() {
value_pos=thread_local_values.insert(std::make_pair(thread_local_instance,std::shared_ptr<ThreadLocalValueHolderBase>(thread_local_instance->NewValueForCurrentThread()))).first;
::testing::UnitTest::GetInstance()->parameterized_test_registry().GetTestSuitePatternHolder<test_suite_name>(GTEST_STRINGIFY_(test_suite_name),::testing::internal::CodeLocation(__FILE__,__LINE__))->AddTestPattern(GTEST_STRINGIFY_(test_suite_name),GTEST_STRINGIFY_(test_name),new ::testing::internal::TestMetaFactory<GTEST_TEST_CLASS_NAME_(test_suite_name,test_name)>(),::testing::internal::CodeLocation(__FILE__,__LINE__));
}

bool TernaryBranchCommentIndent(bool a,bool b,bool c){
bool value=a?
// true case
b:c;
return value;
}

bool TernaryFalseBranchCommentIndent(bool a,bool b,bool c){
bool value=a?b:
// false case
c;
return value;
}

void UniversalBreakSelectionCases() {
    const int singleBinaryValue=firstValue+BuildValue(firstArgumentWithLongName,secondArgumentWithLongName,thirdArgumentWithLongName,fourthArgumentWithLongName);
    const int tailCallChainValue=firstValue+secondValue+BuildValue(firstArgumentWithLongName,secondArgumentWithLongName,thirdArgumentWithLongName,fourthArgumentWithLongName);
    const int sameOperatorChainValue = firstValue +
        BuildValue(
            firstArgumentWithLongName,
            secondArgumentWithLongName,
            thirdArgumentWithLongName,
            fourthArgumentWithLongName
        ) +
        finalValueWithLongName;
    const int nestedTieValue = OuterValue(
        FirstValue(firstArgumentWithLongName, secondArgumentWithLongName),
        SecondValue(thirdArgumentWithLongName, fourthArgumentWithLongName),
        finalArgumentWithLongName
    );
    const int singleTernaryValue = conditionWithLongName ?
        BuildValue(firstArgumentWithLongName, secondArgumentWithLongName, thirdArgumentWithLongName) :
        fallbackValueWithLongName;
    const int ternaryChainValue = firstConditionWithLongName ? firstValueWithLongName :
        secondConditionWithLongName ? BuildValue(
            firstArgumentWithLongName,
            secondArgumentWithLongName,
            thirdArgumentWithLongName
        ) :
        fallbackValueWithLongName;
    const int tailTernaryChainValue = firstConditionWithLongName ? firstValueWithLongName : BuildValue(firstArgumentWithLongName,secondArgumentWithLongName,thirdArgumentWithLongName,fourthArgumentWithLongName);
    const int ternaryTrueBranchChainValue = firstConditionWithLongName ? secondConditionWithLongName ? firstValueWithLongName : secondValueWithLongName : fallbackValueWithLongName;
    UseTemplate<
        FirstTemplateArgumentWithLongName,
        SecondTemplateArgumentWithLongName,
        ThirdTemplateArgumentWithLongName
    >();
    AppConfig config = extraTemplate.empty() ? LoadConfig(
        GetRuntimeConfigPath(),
        !options.defaultConfig,
        context
    ) : LoadConfigWithExtraTemplate(GetRuntimeConfigPath(), !options.defaultConfig, context, extraTemplate);
}

const char* PickParenthesizedTernary(const State& state) {
    return (
        !state.first_value.has_value() ? "First value" : (
        !state.second_value.has_value() ? "Second value" : (
        !state.third_value.has_value() ? "Third value" : (
        !state.fourth_value.has_value() ? "Fourth value" : throw Error())))
    );
}

int ParenthesizedOperatorPieces() {
return firstLongOuterOperandForParenthesizedOperatorPieces + (secondExtraLongInnerOperandForParenthesizedOperatorPieces + thirdExtraLongInnerOperandForParenthesizedOperatorPieces) + fourthLongOuterOperandForParenthesizedOperatorPieces;
}

void TrailingListExpansionCases() {
    UseTrailingListExpansion(firstValue,secondValue,BuildValue(firstArgumentWithLongName,secondArgumentWithLongName,thirdArgumentWithLongName,fourthArgumentWithLongName));
    UseTrailingListExpansion(firstValue,secondValue,FormatTableRow{"tail.list.row.with.extra.detail",100,200,firstInitializerFlagWithVeryLongName | secondInitializerFlagWithVeryLongName | thirdInitializerFlagWithVeryLongName | fourthInitializerFlagWithVeryLongName});
    UseTrailingListExpansion(firstValue,secondValue,conditionWithLongName ? firstValueWithLongName : BuildValue(firstArgumentWithLongName,secondArgumentWithLongName,thirdArgumentWithLongName,fourthArgumentWithLongName));
    UseTrailingListExpansion(firstValue,secondValue,firstReallyLongEqualityOperandForTrailingListExpansion == secondReallyLongEqualityOperandForTrailingListExpansion);
    const int compactMaxTailExpansion = std::max(0, ComputeTrailingMaximumCandidate(firstArgumentWithLongName, secondArgumentWithLongName, thirdArgumentWithLongName, fourthArgumentWithLongName));
}

ColorMixParseResult ParseColorMixParts(const std::vector<std::string>& parts) {
    ColorMixParseResult parsed{};
    if (parts.size() != 2) {
        return std::nullopt;
    } const double amount=ParseDoubleOrDefault(parts[0],std::numeric_limits<double>::quiet_NaN());

    if (!std::isfinite(amount)||amount<0.0||amount>1.0) {
        return std::nullopt;
    } parsed.mix=ColorMixExpression{parts[1],amount};
    return parsed;
}

bool EqualStringVectors(const void* address, const void* compareAddress) {
    return *reinterpret_cast<const std::vector<std::string>*>(address)==*reinterpret_cast<const std::vector<std::string>*>(compareAddress);
}

[[noreturn]] void FailWithAttribute(const char* message){throw Error(message);}

void FormatterSelfBreakCases(){
if(tokens[statementStart].text=="for"||tokens[statementStart].text=="while"||tokens[statementStart].text=="switch"){return;}
if(next<tokens.size()&&tokens[next].kind==TokenKind::Word&&(tokens[next].text=="else"||tokens[next].text=="catch"||tokens[next].text=="finally"||(tokens[next].text=="while"&&closedBlock.kind==BlockKind::DoStatement))){return;}
if(pendingTokens_.empty()&&pendingPrefix_.empty()&&IsTrailingCommentAfterEmittedClose(tokens,index)&&!outputLines_.empty()){return;}
}

void ApplyLayoutEditColorExpression(AppConfig& config,const LayoutEditParameter* parameter){
if(parameter==nullptr){return;}else if(const auto currentColor=FindLayoutEditParameterColorConfigValue(config,*parameter);currentColor.has_value() && *currentColor!=nullptr){colorExpressionValue=TooltipColorExpression(**currentColor);}
}

bool ConfigureDisplayGuard(DisplayState& state,DisplayOption option,DashboardShellHost& shell,UpdatedConfig updatedConfig){
if(!::ConfigureDisplay(updatedConfig,state.telemetryUpdate.dump,option.fittedScale,shell.TraceLog(),shell.WindowHandle())){return true;}
return false;
}

void GenericNestedCallDelimiterCombining(){
CallA(CallB(firstNestedCallArgumentWithLongName,secondNestedCallArgumentWithLongName,thirdNestedCallArgumentWithLongName,fourthNestedCallArgumentWithLongName,fifthNestedCallArgumentWithLongName));
}

int MeasureHexLabelWidth(HWND hwnd){
const int hexLabelWidth=MeasureTextWidthForControl(hwnd,IDC_LAYOUT_EDIT_COLOR_HEX_LABEL,ReadDialogControlText(hwnd,IDC_LAYOUT_EDIT_COLOR_HEX_LABEL))+8;
return hexLabelWidth;
}

int MeasureTextBlockRight(const RenderRect& measureRect,const std::wstring& wideText,const TextStyle& style){
const int width=std::max(0,static_cast<int>(MeasureTextBlockD2D(measureRect,wideText,style,TextLayoutOptions::SingleLine(TextHorizontalAlign::Leading,TextVerticalAlign::Center),nullptr).textRect.right));
return width;
}

size_t CountLeftCards(const std::vector<int>& cardPlanned,const std::vector<Callout>& plannedCalloutDetails,const std::vector<CardPlacement>& cardPlacements,size_t cardIndex){
const size_t leftCount=cardPlanned.size()==1?(plannedCalloutDetails[cardPlanned.front()].target.Center().x<cardPlacements[cardIndex].sourceRect.Center().x?1:0):cardPlanned.size()/2;
return leftCount;
}

RenderRect BuildGuideSheetTargetRect(const PlannedCallout& planned,const std::vector<CardPlacement>& cardPlacements,double dx,double dy){
const RenderRect targetRect=cardPlacements[planned.cardIndex].overview?TransformRect(planned.target,cardPlacements[planned.cardIndex].sourceRect,cardPlacements[planned.cardIndex].destRect):OffsetRenderRect(planned.target,dx,dy);
return targetRect;
}

double ComputeBackgroundWeight(const Geometry& geometry,double sampleX,double sampleY,double denom){
const double backgroundWeight=((geometry.topY - geometry.bottomY)*(sampleX - geometry.bottomX)+(geometry.bottomX - geometry.rightX)*(sampleY - geometry.bottomY))/denom;
return backgroundWeight;
}

double SampleSupersampledX(int x,int sx,int iconSize){
const double sampleX=(static_cast<double>(x)+(static_cast<double>(sx)+0.5)/kSupersample)*256.0/static_cast<double>(iconSize);
return sampleX;
}

void BuildTrianglePoints(const RECT& rect,const Geometry& geometry){
POINT points[]={{rect.left + static_cast<LONG>(std::lround(geometry.leftX)),rect.top + static_cast<LONG>(std::lround(geometry.topY))},{rect.left + static_cast<LONG>(std::lround(geometry.rightX)),rect.top + static_cast<LONG>(std::lround(geometry.topY))},{rect.left + static_cast<LONG>(std::lround(geometry.bottomX)),rect.top + static_cast<LONG>(std::lround(geometry.bottomY))}};
Use(points);
}

void AddWidgetAnimation(PresentationAnimation animation,TargetState targetState){
WidgetAnimationsForLayer(currentWidgetAnimationLayer_).push_back(DashboardPresentationAnimation{std::move(animation),std::move(targetState),currentWidgetAnimationTranslation_});
}

void AddMetricDefinition(MetricsConfig& metrics){
metrics.definitions.push_back(MetricDefinitionConfig{"gpu.load",MetricDisplayStyle::Percent,true,0.0,"%","Load"});
}

int BracedReceiverChain(int firstCoordinateWithLongName,int secondCoordinateWithLongName,int thirdCoordinateWithLongName,int y,int deltaX,int deltaY){
return RenderPoint{firstCoordinateWithLongName+secondCoordinateWithLongName+thirdCoordinateWithLongName,y}.OffsetBy(deltaX,deltaY).x;
}

void DrawGuideDot(RenderHost& renderer,int x,int y,int dotLength,int right,int strokeWidth){
renderer.Renderer().FillSolidRect(RenderRect{x,y,std::min(x+dotLength,right),y+strokeWidth},RenderColorId::LayoutGuide);
}

void DrawGuideDotFromAdapter(RenderHost& renderer,RenderState& state,int x,int y,int dotLength,int right,int strokeWidth){
RenderHostAdapter{renderer,state}.Renderer().FillSolidRect(RenderRect{x,y,std::min(x+dotLength,right),y+strokeWidth},RenderColorId::LayoutGuide);
}

void RegisterStaticEditAnchor(RenderHost& renderer,Widget& widget,const RenderRect& barRect,const RenderRect& anchorRect,const Config& config,int rowIndex,int anchorCenterX,int anchorCenterY){
renderer.EditArtifacts().RegisterStaticEditAnchor(LayoutEditAnchorRegistration{.key=LayoutEditAnchorKey{LayoutEditWidgetIdentity{widget.cardId,widget.editCardId,widget.nodePath},WidgetHost::LayoutEditParameter::MetricListBarHeight,rowIndex},.targetRect=barRect,.anchorRect=anchorRect,.shape=AnchorShape::Circle,.value=config.barHeight,.drag=LayoutEditAnchorDrag::AxisDelta(AnchorDragAxis::Horizontal,RenderPoint{anchorCenterX,anchorCenterY})});
}

void AddThemeColorLeaf(Theme* theme,std::string token,LayoutEditTreeNode& leafNode,const LayoutEditTreeNode& sectionNode){
leafNode.leaf.emplace(LayoutEditTreeLeaf{ThemeColorEditKey{theme->name,token},sectionNode.label,token,leafNode.descriptionKey,configschema::ValueFormat::ColorHex,});
}

void AssignCompactBracedConstructor(LayoutEditGapAnchor& outerMarginAnchor){
outerMarginAnchor.key.widget=LayoutEditWidgetIdentity{"","",{},LayoutEditWidgetIdentity::Kind::DashboardChrome};
}

void PlaceEmptyLambdaCallout(){
const LayoutGuideSheetPlacementResult result=PlaceLayoutGuideSheetCallouts(cardPlacements,callouts,LayoutGuideSheetPlacementStyle{10,12,4,20,0,1},[](LayoutGuideSheetPlacementCallout&,int){
},nullptr);
}

void AssignedSingleStatementLambdaContext(){
const auto shortAssignedLambda=[](int value){return value+1;};
const auto extremelyLongAssignedLambdaNameThatConsumesEnoughColumnsToForceTheAssignmentPrefixAwayFromTheLambdaHeaderBeforeTheSingleStatementBody = [](int value){return value+1;};
}

auto ReturnedLambdaWithSplitOwner() {
return [](const ExtremelyLongEventNameForLambdaReturnGeneralization& event, const ExtremelyLongContextNameForLambdaReturnGeneralization& context) {
Handle(event, context);
Finish(event, context);
};
}

const char* ReturnForcedAdjacentString() {
return "first line\n" "second line";
}

const char* ActiveAdjacentNewlineEscape="first\n" "second";
const char* ActiveAdjacentCrLfEscape="first\r\n" "second";
const char* EscapedAdjacentNewlineText="first\\n" "second";
const char* EscapedAdjacentCrLfText="first\\r\\n" "second";

Task CoReturnForcedAdjacentString() {
co_return "first line\n" "second line";
}

void ThrowForcedAdjacentString() {
throw "first line\n" "second line";
}

Generator CoYieldForcedAdjacentString() {
co_yield "first line\n" "second line";
}

Task KeywordOwnedCoReturnSpacing() {
co_return(value);
co_return[](){return value;};
co_return::qualifiedValue;
}

void KeywordOwnedThrowSpacing() {
throw(failure);
throw::qualifiedFailure;
}

Generator KeywordOwnedCoYieldSpacing() {
co_yield(item);
co_yield[]{return item;}();
co_yield::qualifiedItem;
}

void CallForcedAdjacentString() {
Log("first line\n" "second line");
}

void SnapGaugeWidth(){
const bool snapped=layout_snap_solver::FindNearestSnapWeight(kCurrentGaugeWeight,kCombinedWeight,kThreshold,{layout_snap_solver::SnapCandidate{targetExtent,targetExtent-startExtent,0}},[](int firstWeight,int& extent)->bool{extent=ComputeCpuGaugeWidth(firstWeight);return true;},snappedWeight);
}

int CountPlusAnchors(RenderHost& renderer){
return static_cast<int>(std::count_if(renderer.editArtifacts.staticAnchors.begin(),renderer.editArtifacts.staticAnchors.end(),[](const LayoutEditAnchorRegion& region){return region.shape==AnchorShape::Plus;}));
}

void TraceCaptureChanged(HWND hwnd,LPARAM lParam,bool handled){
TraceLayoutEditUiEventFmt(TracePrefix::LayoutEditUi,"wm_capturechanged","new_owner=\"%s\" handled=\"%s\"",reinterpret_cast<HWND>(lParam)==nullptr?"none":(reinterpret_cast<HWND>(lParam)==hwnd?"dashboard":"other"),firstValueWithLongName+secondValueWithLongName+thirdValueWithLongName+fourthValueWithLongName+fifthValueWithLongName,handled?"true":"false");
}

int ManyParameters(int * firstPointerWithLongName,int & firstReferenceWithLongName,int secondValueWithLongName,int thirdValueWithLongName,int fourthValueWithLongName,int fifthValueWithLongName,int sixthValueWithLongName){
int localValueWithLongName=firstPointerWithLongName ? *firstPointerWithLongName:0;// trailing
bool combinedValue=firstReferenceWithLongName > 0 && secondValueWithLongName > 0 && thirdValueWithLongName > 0 && fourthValueWithLongName > 0 && fifthValueWithLongName > 0 && sixthValueWithLongName > 0;
if(localValueWithLongName)return firstReferenceWithLongName;
while(localValueWithLongName<secondValueWithLongName)++localValueWithLongName;
for(int index=0;index<thirdValueWithLongName;++index){localValueWithLongName+=index;}
switch(localValueWithLongName){case 1:{int scopedValue=localValueWithLongName+fourthValueWithLongName; localValueWithLongName=scopedValue; break;} case 2: return fourthValueWithLongName; default: break;}
return VeryLongFunctionCall(firstReferenceWithLongName,secondValueWithLongName,thirdValueWithLongName,fourthValueWithLongName,fifthValueWithLongName,sixthValueWithLongName,localValueWithLongName,123456789,987654321);
}

int NestedSwitchIndent(int message,int wParam){
switch(message){case WM_WTSSESSION_CHANGE:switch(wParam){case WTS_SESSION_LOCK:return 1;default:return 0;}case WM_ERASEBKGND:return 1;default:return 0;}
}

int CharacterCaseSpacing(char value){
switch(value){case '\n':return 1;case '\\':return 2;default:return 0;}
}

void LongForCondition(){
for(int rowIndex=0;rowIndex<layoutState_.visibleRows&&rowIndex<static_cast<int>(layoutState_.rowBarRects.size())&&rowIndex<static_cast<int>(layoutState_.rowBarAnchorRects.size());++rowIndex){Use(rowIndex);}
}

bool ReviewLogNumericLimits(long value){
return value<(std::numeric_limits<int>::min)()||value>(std::numeric_limits<int>::max)();
}

bool ReviewLogLayoutMove(int fromIndex,int toIndex,LayoutNodeConfig* node){
return fromIndex<0||toIndex<0||fromIndex>=static_cast<int>(node->children.size())||toIndex>=static_cast<int>(node->children.size());
}

bool ReviewLogChoiceFor(int nodeId,const FormatBreakSolution& solution){
if(nodeId<0||static_cast<size_t>(nodeId)>=solution.choices.size()){return false;}
return true;
}

void ReviewLogJsonDigit(){
while(position_<text_.size()&&text_[position_]>='0'&&text_[position_]<='9'){++position_;}
}

void ReviewLogMetricDrag(){
if(draggedIndex<0||draggedIndex>=static_cast<int>(metricRefs_.size())||draggedIndex>=static_cast<int>(layoutState_.rowRects.size())){return;}
}

std::optional<BoardVendorTelemetrySample> ReviewLogBoardSensorsResponse(){
if(!ReadString(cursor,remaining,payloadHeader.boardManufacturerBytes,sample.boardManufacturer)||!ReadStringVector(cursor,remaining,payloadHeader.requestedTemperatureCount,sample.requestedTemperatureNames)||!ReadStringVector(cursor,remaining,payloadHeader.availableTemperatureCount,sample.availableTemperatureNames)||remaining!=0){return std::nullopt;}
return sample;
}

void ControlFlowVariety(int * values,int count){
if(count>0){values[0]+=1;}else values[0]=0;
if(count==0)values[0]=0;else if(count==1)values[0]=1;else{if(count==2){values[0]=2;}}

    if (values != nullptr) {
        values[0] = count;
    }
    while (count > 0) {
        --count;
    }

    for (int outer = 0; outer < count; ++outer) {
        if (values[outer] % 2 == 0) {
            values[outer] += outer;
        } else {
            values[outer] -= outer;
        }
    }
    for (int simple = 0; simple < count; ++simple) {
        values[simple] += 1;
    }
    switch (count) {
        default:
            values[0] = count;
    }
    int index = 0;
    for (;;) {
        break;
    }
    while (index < count) {
        values[index] += index;
        ++index;
    }
    do {
        ++index;
    } while (index < count);
    do {
        --index;
    } while (index > 0);
}

void AttributedCompoundControlBodies(int count){
if(count<0) [[unlikely]] { Use(count); }
else [[likely]] { Use(-count); }
while(count>0) [[likely]] { --count; }
for(int index=0;index<count;++index) [[likely]] { Use(index); }
do [[unlikely]] { ++count; } while(count<0);
switch(count) [[likely]] { default: break; }
}

void AttributedElseIfCollapse(bool first,bool second,bool third){
if(first){Use(first);}else{[[likely]] if(second){Use(second);}}
if(first){Use(first);}else [[likely]] if(second){Use(second);}else{[[likely]] [[likely]] if(third){Use(third);}}
}

void CommentedControlBodyBoundaries(bool first,bool second,int count){
if(first)/* if body */{Use(first);}else/* else body */{Use(second);}
if(first){Use(first);}else/* braced else-if */{if(second){Use(second);}else/* nested else body */{Use(count);}}
if(first){Use(first);}else/* else-if */if(second){Use(second);}
if(first){Use(first);}else/* unbraced else */Use(second);
if(first){Use(first);}/* before else */else{Use(second);}
if(first){Use(first);}/* first before else *//* second before else */else{Use(second);}
if(first){}/* before empty else */else{Use(second);}
if(second){}/* first empty *//* second empty */else{Use(first);}
if(first){Use(first);}else/* first *//* second */{Use(second);}
while(count>0)/* while body */{--count;}
for(int i=0;i<count;++i)/* for body */{Use(i);}
do/* do body */{++count;}/* before while */while(count<0);
do{}/* before empty while */while(count<0);
switch(count)/* switch body */{default:break;}
}

void CommentedBlockAttachments(bool first,int count){
try{Use(first);}/* before catch */catch(...){Use(count);}
try{Use(first);}/* before finally */finally{Use(count);}
Consume([&]{Use(first);}/* before comma */,count);
Consume([&]{Use(first);}/* before close */);
auto action=[&]{Use(first);}/* before semicolon */;
auto result=[&]{return count;}/* before call */();
auto constrained=[]<typename T>/* before lambda requires */requires C<T>(T value){return value;};
}

template<typename T>/* before requires */requires C<T>
void CommentedRequiresAttachment();

struct CommentedLabels{
public/* before access colon */:
void Use();
};

void CommentedCaseLabels(int value){
switch(value){case 1:/* before case block */{Use(value);break;}default/* before default colon */:break;}
retry/* before label colon */:Use(value);
}

void EmptyElseIfSpacing(bool first,bool second,bool third){
if(first){}else if(second){}else if(third){Use(third);}
}

void EmptyDoWhile(bool running){
do{}while(running);
}

void TryFinallyCleanup(){
const auto originalDirectory=Environment::CurrentDirectory;
try{
RunWithTemporaryDirectory();
}finally{
Environment::CurrentDirectory=originalDirectory;
}
}

void TryCatchAfterComment(){
try{
Work();
}  // keep note
catch(...){
Recover();
}
}

namespace trailing_comment_fixture {

void UseNamespaceTrailingComment() {}

}  // namespace trailing_comment_fixture

namespace trailing_semicolon_fixture {

void UseNamespaceTrailingSemicolon();

};  // namespace trailing_semicolon_fixture

namespace {

void UseAnonymousNamespaceTrailingSemicolon();

};

extern "C" {

void UseLinkageTrailingSemicolon();

};  // extern "C"

int nullDeclarationTerminator=0;
;

int commentedNullDeclarationTerminator=0;
;  // optional terminator

void FunctionNullDeclarationTerminator(){
Run();
}
;

struct RepeatedDeclarationTerminator{
int value;
}
;
;

void NullStatementTerminators(bool ready){
Run();
;
if(ready){
Run();
}
;
{
Run();
}
;
switch(ready){
case true:{
Run();
}
;
case false:
Run();
;
}
}

void LeadingNullStatement(){
;
Run();
}

void OnlyNullStatement(){;}

namespace only_null_declaration_fixture {
;
}

void RequiredNullStatementBodies(bool ready){
if(ready);
while(ready);
}

void LongComment() {
    // This deliberately long comment should remain as one physical line because ReflowComments is false even though it is beyond the configured column limit for the fixture.
}

bool ParenthesizedEqualityOperator() {
return (firstReallyLongParenthesizedEqualityOperandForOrdinaryBinaryOperatorSplit == secondReallyLongParenthesizedEqualityOperandForOrdinaryBinaryOperatorSplit);
}

unsigned int ParenthesizedBitwiseAndChain() {
return (firstReallyLongParenthesizedBitwiseAndOperandForFormatterOwnedChainSplit & secondReallyLongParenthesizedBitwiseAndOperandForFormatterOwnedChainSplit);
}

unsigned int ParenthesizedXorOperator() {
return (firstReallyLongParenthesizedXorOperandForOrdinaryBinaryOperatorSplit ^ secondReallyLongParenthesizedXorOperandForOrdinaryBinaryOperatorSplit);
}

unsigned int SplitXorOperator() {
    return firstReallyLongXorLeftOperandForOrdinaryBinaryOperatorSplit ^ secondReallyLongXorRightOperandForOrdinaryBinaryOperatorSplit;
}

unsigned int SplitXorChainOperator() {
    return firstReallyLongXorFirstOperandForFormatterOwnedChainSplit ^ secondReallyLongXorSecondOperandForFormatterOwnedChainSplit ^ thirdReallyLongXorThirdOperandForFormatterOwnedChainSplit ^ fourthReallyLongXorFourthOperandForFormatterOwnedChainSplit;
}

int ParenthesizedDivisionOperator() {
return (firstReallyLongParenthesizedDivisionOperandForOrdinaryBinaryOperatorSplit / secondReallyLongParenthesizedDivisionOperandForOrdinaryBinaryOperatorSplit);
}

int ParenthesizedSubtractionOperator() {
return (firstReallyLongParenthesizedSubtractionOperandForOrdinaryBinaryOperatorSplit - secondReallyLongParenthesizedSubtractionOperandForOrdinaryBinaryOperatorSplit);
}

int ParenthesizedRemainderOperator() {
return (firstReallyLongParenthesizedRemainderOperandForOrdinaryBinaryOperatorSplit % secondReallyLongParenthesizedRemainderOperandForOrdinaryBinaryOperatorSplit);
}

int SplitDivisionOperator() {
    return firstReallyLongNumeratorValueForOrdinaryDivisionOperatorSplit / secondReallyLongDivisorValueForOrdinaryDivisionOperatorSplit;
}

int SplitRemainderOperator() {
    return firstReallyLongDividendValueForOrdinaryRemainderOperatorSplit % secondReallyLongDivisorValueForOrdinaryRemainderOperatorSplit;
}

int SplitSubtractionOperator() {
    return firstReallyLongMinuendValueForOrdinarySubtractionOperatorSplit - secondReallyLongSubtrahendValueForOrdinarySubtractionOperatorSplit;
}

void AllocateBitmapPixels(){
std::vector<DisplayPlacementMenuBitmapPixel> pixels((kBitmapSize * kBitmapSize));
}

struct CompactBaseList:First,Second{};

class PackedBaseListWithLongClassName final:public FirstInterfaceWithLongName,protected SecondInterfaceWithLongName,private ThirdBase{void Run();};

template<class... Bases> struct PackedTemplateBaseListWithLongClassName final:public GenericBase<FirstArgument,SecondArgument>,protected virtual Interface,Bases...{};

struct InheritanceListHost{struct PackedNestedBaseListWithLongClassName:FirstInterfaceWithLongName,SecondInterfaceWithLongName,ThirdInterfaceWithLongName{};};

struct WideBaseList:FirstInterfaceWithNameTooLongToShareALineWithTheOtherBaseClasses,SecondInterfaceWithNameTooLongToShareALineWithTheOtherBaseClasses,ThirdInterface{};

struct PrefixCommentBaseList: // preserve split
First,Second{};

template<class Base>
class CommentedBaseList // keep this declaration comment
:public First,public Base{public:void Run();};

class CommentedClassBrace /* public for compatibility */
{public:void Raise();};

struct ExpandedTemplateBaseList:SimpleBase,GenericBase<FirstTemplateArgumentWithAnExtremelyLongNameForTestingBaseListExpansion,SecondTemplateArgumentWithAnExtremelyLongNameForTestingBaseListExpansion>{};

class BaseClassListCommentDerived : public BaseClassListCommentRootA,  // primary
public BaseClassListCommentRootB, public BaseClassListCommentRootC {};

struct Derived final :
FormatterReviewExtremelyLongBaseClassNameThatForcesTheInheritanceClauseToRemainBrokenAcrossLines<Derived> {using Request=int;};

void AssemblyPrefixLists(){
asm volatile("op" : "=r"(firstOutput),"=r"(secondOutput) : "r"(firstInput),"r"(secondInput) : "memory","cc");
asm("instruction" : "=r"(firstOutputOperandWithLongDescriptiveName),"=r"(secondOutputOperandWithLongDescriptiveName));
asm("another longer instruction" : : "r"(firstInputOperandWithLongDescriptiveName),"r"(secondInputOperandWithLongDescriptiveName));
asm("instruction" : : : "first_clobbered_register_with_a_long_descriptive_name","second_clobbered_register_with_a_long_descriptive_name");
asm goto("jmp %l0; nop; nop; nop" : : : : firstDestinationLabelWithLongDescriptiveName,secondDestinationLabelWithLongDescriptiveName);
asm("instruction" : "=r"(firstOutputOperandWithAnExtremelyLongDescriptiveNameForTestingPrefixListWrapping),"=r"(secondOutputOperandWithAnExtremelyLongDescriptiveNameForTestingPrefixListWrapping));
asm volatile("instruction" : // preserve split
"=r"(firstOutput),"=r"(secondOutput));
asm volatile("instruction" : : "r"(firstInput), // preserve split
"r"(secondInput));
asm volatile("op" : : :);
firstDestinationLabelWithLongDescriptiveName:Use();
secondDestinationLabelWithLongDescriptiveName:Use();
}

void RegisterSubscriptListComment() {
value = matrix[firstReallyLongIndexForFormatterGenerality,  // selected row
secondReallyLongIndexForFormatterGenerality, thirdReallyLongIndexForFormatterGenerality];
}

bool AttachedOpenChainKeepsFollowingOperator(const PrintToken* previous, KnownToken prev, const PrintToken& current) {
    if (
        previous->kind == PrintTokenKind::Known && (
            prev == KnownToken::RightParen ||
            prev == KnownToken::RightBracket ||
            prev == KnownToken::RightBrace ||
            prev == KnownToken::Greater
        ) &&
        IsWordLike(current)
    ) {
        return true;
    }
    return false;
}

struct BracedMacroInitializerStressEntry {};

enum class BracedMacroInitializerStressKind {
    Value
};

constexpr BracedMacroInitializerStressEntry BracedMacroInitializerStressValues[] = {
    BRACED_MACRO_INITIALIZER_STRESS("field1", BracedMacroInitializerStressKind::Value, source.path1),
    BRACED_MACRO_INITIALIZER_STRESS("field2", BracedMacroInitializerStressKind::Value, source.path2),
    BRACED_MACRO_INITIALIZER_STRESS("field3", BracedMacroInitializerStressKind::Value, source.path3),
    BRACED_MACRO_INITIALIZER_STRESS("field4", BracedMacroInitializerStressKind::Value, source.path4),
    BRACED_MACRO_INITIALIZER_STRESS("field5", BracedMacroInitializerStressKind::Value, source.path5),
    BRACED_MACRO_INITIALIZER_STRESS("field6", BracedMacroInitializerStressKind::Value, source.path6),
    BRACED_MACRO_INITIALIZER_STRESS("field7", BracedMacroInitializerStressKind::Value, source.path7),
    BRACED_MACRO_INITIALIZER_STRESS("field8", BracedMacroInitializerStressKind::Value, source.path8),
    BRACED_MACRO_INITIALIZER_STRESS("field9", BracedMacroInitializerStressKind::Value, source.path9),
    BRACED_MACRO_INITIALIZER_STRESS("field10", BracedMacroInitializerStressKind::Value, source.path10),
    BRACED_MACRO_INITIALIZER_STRESS("field11", BracedMacroInitializerStressKind::Value, source.path11),
    BRACED_MACRO_INITIALIZER_STRESS("field12", BracedMacroInitializerStressKind::Value, source.path12),
    BRACED_MACRO_INITIALIZER_STRESS("field13", BracedMacroInitializerStressKind::Value, source.path13),
    BRACED_MACRO_INITIALIZER_STRESS("field14", BracedMacroInitializerStressKind::Value, source.path14),
    BRACED_MACRO_INITIALIZER_STRESS("field15", BracedMacroInitializerStressKind::Value, source.path15),
    BRACED_MACRO_INITIALIZER_STRESS("field16", BracedMacroInitializerStressKind::Value, source.path16),
    BRACED_MACRO_INITIALIZER_STRESS("field17", BracedMacroInitializerStressKind::Value, source.path17),
    BRACED_MACRO_INITIALIZER_STRESS("field18", BracedMacroInitializerStressKind::Value, source.path18)
};

int DelimiterStackThresholdGenerality(
    int firstReallyLongOperandName,
    int secondReallyLongOperandName,
    int thirdReallyLongOperandName,
    int fourthReallyLongOperandName
) {
    int seven = (((((((firstReallyLongOperandName + secondReallyLongOperandName + thirdReallyLongOperandName + fourthReallyLongOperandName)))))));
    int eight = ((((((((firstReallyLongOperandName + secondReallyLongOperandName + thirdReallyLongOperandName + fourthReallyLongOperandName))))))));
    return seven + eight;
}

int DeepDelimiterStressCase(int y) {
    int x = ((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((y))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))));
    return x;
}

// Delimiter-boundary coalescing generalizes only direct close-comma-open boundaries:
// bare-brace items coalesce at `}, {`, bare-paren items coalesce at `), (`, and
// prefixed template-id items keep separate item lines.
void DelimiterBoundaryCoalescingGenerality() {
Widget braceBoundaryRows[] = {{firstBraceElementValueForCoalescingGenerality, secondBraceElementValueForCoalescingGenerality, thirdBraceElementValueForCoalescing}, {fourthBraceElementValueForCoalescingGenerality, fifthBraceElementValueForCoalescingGenerality, sixthBraceElementValueForCoalescing}};
int parenBoundaryValues[] = {(firstParenElementValueForCoalescingGenerality + secondParenElementValueForCoalescingGenerality + thirdParenElementValueForCoalescing), (fourthParenElementValueForCoalescingGenerality + fifthParenElementValueForCoalescingGenerality + sixthParenElementValueForCoalescing)};
OuterAngleContainerForCoalescingGenerality<FirstAngleElementTemplateForCoalescingGenerality<firstAngleArgumentValueNameForCoalescing, secondAngleArgumentValueNameForCoalescing>, SecondAngleElementTemplateForCoalescingGenerality<thirdAngleArgumentValueNameForCoalescing, fourthAngleArgumentValueNameForCoalescing>> angleBoundaryValue;
}

struct FormatterOperatorSpacingRegression {
explicit operator bool() const;
operator std::shared_ptr<const T>();
void* operator new(std::size_t);
};

template<typename T>
struct FormatterConversionOperatorDefinitionSpacing {
constexpr operator T() const;
};

template<typename T>
constexpr FormatterConversionOperatorDefinitionSpacing<T>::operator T() const{return T{};}

struct FormatterPureVirtualRegression {
virtual ~FormatterPureVirtualRegression() = 0;
};

struct RefQualifiedNoexceptSpacing {
void Lvalue() & noexcept;
void Rvalue() && noexcept(false);
};

struct FormatterMacroTrailingCommentRegression {
FORMAT_FIXTURE_DECLARE_OPTION(set_ssh_key_function, void*); // TODO curl_sshkeycallback?
};

int DumpFormatModelText(std::string_view sourceText,const FormatterConfig& config,FILE* output,FILE* errorOutput,std::string_view commandName);

FormatBreakNode* BuildTemplatePrefix(const ConstSyntaxChildList& templateHeadChildren,const SyntaxNode* requiresNode,int depth){return nullptr;}

void IifeCallSplit(){auto value=[](){return 1;}();Use(value);}

auto CompactLambdaNestedInitializer(int x){return [x](){return T{x};};}

void TemplateAngleSpacing(){chaotic::Validate<Validators...>(arr,value);return std::get<0>(key_or_result_);}

template<typename... T>
void AmbiguousTemplateIdSpacing(Json& json,Stream& stream){
utils::SmallString<10> str;
auto decimal=Decimal<10>::FromBiased(1);
std::tuple<T...> tuple;
detail::StaticQueryParameters<sizeof...(T)> params;
StaticQueryParameters<3> directParams;
json.ParseStream<Flags::kOne|Flags::kTwo>(stream);
}

auto QualifiedCallableTemplateInNestedCall(const Item& item){return decimal64::ToStringTrailingZeros(decimal64::Decimal<3>(item.weight())/kGramInKgInt);}

bool AlternativeBinaryOperatorSpacing(bool first,bool second,bool third){return first and (!second or third);}

bool AlternativeBinaryOperatorBeforeUnary(bool first,bool second){return first and !second;}

bool AlternativeLogicalOperatorChain(){return FirstConditionWithAnIntentionallyLongNameForAlternativeOperatorCoverage() or SecondConditionWithAnIntentionallyLongNameForAlternativeOperatorCoverage() or ThirdConditionWithAnIntentionallyLongNameForAlternativeOperatorCoverage();}

using TemplateArgumentBinary=A<N + 1>;
template<class... T> using TemplateArgumentFold=A<(T::value && ...)>;
using TemplateArgumentPointer=A<T*>;
using TemplateArgumentReference=A<T&&>;
using TemplateArgumentUnary=A<-1>;
using TemplateArgumentQualifiedAddress=A<& ::T::value>;
using TemplateArgumentPointerDereference=A<* ::pointer>;

void FallthroughSpaceBeforeSemicolon(int value){switch(value){case 0:[[fallthrough]];case 1:break;}}

formats::json::Value SerializeWithUnnamedParameter(
const CallCenterTariffSettings& data,
const formats::serialize::To<formats::json::Value>&  /*to*/
){}

void ForEmptyInitSpacingOrBody(size_t count){size_t len=0;for(;(1UL << len) < count;++len);}

void StandaloneOrTrailingCommentsMoved(int command){switch(command){case 67: /* command complete */
 Complete(); break;}}

void StandaloneCommentBeforeSwitchLabel(int value){switch(value){case 1:Use();break;

// fallback
case 2:break;}}

void EnumDeclaratorDetached(){enum{kChar,kPercent,kKey}state=kChar;Use(state);}

struct FormatterEmptyBlockBreakRegression { FormatterEmptyBlockBreakRegression() {}int value; };

template <typename T>
void FormatterSuspiciousDiffRegressionCases() {
auto duration = 100ms;
auto negativeDuration = -50ms;
if constexpr (FormatterCondition<T>) { Use(duration); } else if constexpr (FormatterOtherCondition<T>) { Use(negativeDuration); }
Call(1 /*count*/, "x" /*name*/);
auto lambda = [] { // starts
Work();
};
auto binary = T{}+ i;
auto fold = (spans.size()+...);
void* allocated = ::operator new(4);
StartFormattingCallbacks(
    ready,
    [this](const std::string& consumer_tag) {
        Use(consumer_tag);
    },
    // message callback
    [this](const FormatterMessage& message, uint64_t delivery_tag, bool) {
        if (!stopped_) {
            OnMessage(message, delivery_tag);
        }
    },
    start_deadline
);
}

// Compact-list prefix pruning: every non-final nested call must remain on the opener line.
auto CompactListPrefixPruningRegression(){return make_tuple_of_references(workaround_cast<T,decltype(field01)>(field01),workaround_cast<T,decltype(field02)>(field02),workaround_cast<T,decltype(field03)>(field03),workaround_cast<T,decltype(field04)>(field04),workaround_cast<T,decltype(field05)>(field05),workaround_cast<T,decltype(field06)>(field06),workaround_cast<T,decltype(field07)>(field07),workaround_cast<T,decltype(field08)>(field08),workaround_cast<T,decltype(field09)>(field09),workaround_cast<T,decltype(field10)>(field10),workaround_cast<T,decltype(field11)>(field11),workaround_cast<T,decltype(field12)>(field12));}

void ControlInitializerDeclaratorBinding(){if(T* pointer=g()){}if(T& reference=g()){}if(T&& rvalueReference=g()){}}

void ControlInitializerTemplateType(){if(A<B> value=g()){}}

void ControlInitializerContinuationIndent(){
while(ready){if(FormatBreakNode* templated=BuildAdjacentTemplateDeclaration(children,index,end,depth+1,afterTemplate)){Use(templated);}}
if(veryLongAssignmentTargetNameThatMustStayWithItsOperator=BuildAdjacentTemplateDeclaration(children,index,end,depth+1,afterTemplate)){Use(veryLongAssignmentTargetNameThatMustStayWithItsOperator);}
}

}

template <typename T>
void operator==(T, T) = delete;
template <typename T>
void operator!=(T, T) = delete;

struct FriendOperators {
[[maybe_unused]] friend bool operator==(const char* lhs, FriendOperators) { return *lhs == '\0'; }[[maybe_unused]] friend bool operator!=(const char* lhs, FriendOperators) { return *lhs != '\0'; }
};

int CommentedValues[] = {
/** one */
1,
};

void TrailingBlockComment() {
if (Ready()) {  // keep
Run();
}
}

X CommentSeparatedChain() {
return X()
.A()
// next group
.B();
}

auto rawStringSuffix = R"(value)"sv;

struct DesignatedBraceInner { int value; };
struct DesignatedBraceOuter { int first; DesignatedBraceInner inner; };
DesignatedBraceOuter MakeDesignatedBraceOuter(){return {.first{1},.inner{.value{2}}};}

void PreferShallowBreakOverFewerLines() {
const auto same_point_pickup_coefficients = pickup_settings.same_point_pickup_coefficients | ranges::MapTo<std::vector>([](const auto& settings) { return psp_defs::PickupCoefficient{.coef = settings.ToDoubleInexact()}; });
}

void WeightedExpansionExamples() {
optional::Map(request.cargo_options(), [&](const auto& cargo_options) { builder[fields::kCargoOptions] = json::Serialize(cargo_options); });
auto result = BuildResult(request, TransformCargoOptions(request.cargo_options(), [&](const auto& cargo_options) { return json::Serialize(cargo_options); }));
DispatchCargoOptions([&](const auto& cargo_options) { PrepareCargoOptions(cargo_options); return SerializeCargoOptions(cargo_options); }, request);
auto& component_block = wb_utils::AddCollapsible(builder, component_text).SetValue<wb::BlockList>(/*orientation=*/ wb::Orientation::kVertical);
output << firstLabel << BuildDetailedCargoOptions(request, [](const auto& cargo_options) { return json::Serialize(cargo_options); });
}

auto ExpansionCostArgumentList(){return BuildResult(firstLongValue,secondLongValue,thirdLongValue,fourthLongValueWithSuffix,BuildFinalValue(firstArgument,secondArgument));}

auto ExpansionCostInitializerList(){return Result{firstLongValue,secondLongValue,thirdLongValue,fourthLongValueWithSuffix,BuildFinalValue(firstArgument,secondArgument)};}

auto ExpansionCostBinaryChain(){return firstLongValue+secondLongValue+thirdLongValue+fourthLongValue+BuildFinalValue(firstArgument,secondArgument);}

auto ExpansionCostCommaList(){return (firstLongValue,secondLongValue,thirdLongValue,fourthLongValueWithSuffix,BuildFinalValue(firstArgument,secondArgument));}

auto ExpansionCostMemberChain(){return CreateFirstValue().ApplyFirstOption().ApplySecondOptionWithSuffix().ApplyThirdOption().FinalizeResult(firstArgument,secondArgument);}

void ExpansionCostStreamChain(){output<<firstLongValue<<secondLongValue<<thirdLongValue<<fourthLongValue<<BuildFinalValue(firstArgument,secondArgument);}

auto ExpansionCostTernaryChain(){return firstCond?firstVal:secondCond?secondVal:thirdCond?thirdVal:fourthCond?fourthVal:BuildFinalValue(firstArgument,secondArgument);}

auto ExpansionCostStringChain(){return Log(firstLongValue,secondLongValue,"This first fragment is part of a longer message. " "This second fragment continues the same message. " "This final fragment completes the message.");}

auto SingleLambdaArgumentKeepsTemplateName() {
return abstract_future::MakeSharedFutureFromCallOnceFunc<mem::SPtr<ExperimentsMap>>([used_tariff, request, avalon_tags_fut, opt_edges_fut, deps = deps_] {
const auto avalon_tags = avalon_tags_fut.transform([](const auto& fut) { return fut.WaitAndGet(); });
const auto opt_edges = opt_edges_fut.transform([](const auto& fut) { return fut.WaitAndGet(); });
const auto opt_router_distance_meter = opt_edges.transform([](const auto& edges) {
const auto shortest_route = edges.at(routing::GetMinimalDistanceRouteIdx(edges));
return static_cast<std::int64_t>(routing::GetRouteDistanceMeter(shortest_route));
});
return FetchPriceModificationsExperimentMap(used_tariff, request, /*driver_tags*/ std::nullopt, avalon_tags, opt_router_distance_meter, deps);
});
}

auto SingleLambdaInitializerKeepsTemplateName() {
return DeferredPriceModificationsCallback<mem::SPtr<ExperimentsMap>>{[used_tariff, request, avalon_tags_fut, opt_edges_fut, deps = deps_] { Prepare(); return Fetch(); }};
}

auto NestedSingleLambdaArgumentGroups() {
return WrapCallback((abstract_future::MakeSharedFutureFromCallOnceFunc<mem::SPtr<ExperimentsMap>>([used_tariff, request, avalon_tags_fut, opt_edges_fut, deps = deps_] { Prepare(); return Fetch(); })));
}

auto RecursiveFinalLambdaDiscountKeepsTemplateName() {
if (!IsBatchSizePredictionRequestEnabled(exp3)) {
return subrequests | ranges::MapTo<std::unordered_map<SlotId, std::optional<mem::SPtr<BatchSizePrediction>>>>([](const auto& subrequest) { return std::make_pair(subrequest.slot_params.slot().slot_id, std::optional<mem::SPtr<BatchSizePrediction>>{}); });
}
}

void PreferCompactWrappersAroundExpandedFinalLambda(){Outer(Middle(Async(task_name,[first,second]{UpdateAnIntentionallyLongValue(first,second,third,fourth,fifth,sixth);})));}

void FinalLambdaDiscountExamples() {
optional::Map([&](const auto& cargo_options) { builder[fields::kCargoOptions] = json::Serialize(cargo_options); }, request.cargo_options());
CallbackConfig config{request.cargo_options(), [&](const auto& cargo_options) { builder[fields::kCargoOptions] = json::Serialize(cargo_options); }};
CallbackConfig commented{request.cargo_options(), [&](const auto& cargo_options) { builder[fields::kCargoOptions] = json::Serialize(cargo_options); },
/* callback */
};
optional::Map(request.cargo_options(), [&](const VeryLongCargoOptionsTypeName& cargo_options, const SerializationContextWithAdditionalCargoOptions& context) { return Serialize(cargo_options, context); });
optional::Map(request.cargo_options(), [&](const auto& cargo_options) { return SerializeCargoOptionsWithContext(cargo_options, serialization_context, additional_serialization_options, serialization_fallback_policy); });
auto callback = [&](const auto& cargo_options) { builder[fields::kCargoOptions] = json::Serialize(cargo_options); };
}

void BraceListTrailingCommaLayout(){
auto compact=BraceValues{first,second,};
auto fullSplit=BraceValues{firstValueWithAnExtremelyLongNameForBraceListTrailingComma,secondValueWithAnExtremelyLongNameForBraceListTrailingComma};
}

void PackedBraceListInRangeFor(){
for(auto value:RangeContainerWhoseTypeNameMakesOnlyTheInitializerDelimitersSplit{firstValue,secondValue,thirdValue}){Use(value);}
}

namespace post_namespace_first_fixture {
class First;
}  // namespace post_namespace_first_fixture
namespace post_namespace_second_fixture {
class Second;
}  // namespace post_namespace_second_fixture
// after adjacent namespaces

struct PureVirtualDeclarationTails {
    virtual void ExtendArgsWithRelativeTiming(LocalizerArgs& args, const std::string& key, int relative_minutes) const = 0;
    virtual Result* BuildActionParams(const State& state, const DataProviderStoragePtr& storage, const Options& options) const = 0;
    struct Nested {
        virtual Result& BuildActionParams(const State& state, const DataProviderStoragePtr& storage, int mode) const = 0;
    };
    virtual Result CopyActionParams(const State& state, const DataProviderStoragePtr& storage, int mode) const = delete;
    int initial_value = ComputeInitialValue(first_argument, second_argument, third_argument, fourth_argument, fifth_argument);
};

void QualifiedTemplateCallWrapping() {
    value = Record{
        .cancel_price = decimal64::Decimal<4>::FromStringPermissive(laundry_tariff_estimating_result.paid_cancel_in_driving.value().cancel_price),
    };
    auto nested = units::Scale<2>::Ratio<3>::ConvertWithOriginalPrecision(source.original_value_with_unit_metadata_and_conversion_options);
    auto mixed = container::Map<Key, units::Scale<4>>::template Rebind<Value>::Create(source.value);
    bool ordered = (first < second) > ::third;
}

void StreamLiteralBindingWithSuffixes() {
    Prepare();
    LOG_INFO() << "[DubaiLaundryOffers] Built request tariffs" << ", mappings_count=" << state_to_request_tariff_mapping.size();
    Consume(LOG_INFO() << "[DubaiLaundryOffers] Built request tariffs" << ", mappings_count=" << state_to_request_tariff_mapping.size());
    auto values = Values{LOG_INFO() << "[DubaiLaundryOffers] Built request tariffs" << ", mappings_count=" << state_to_request_tariff_mapping.size()};
}

void ChainedLambdaInDesignatedInitializer() {
    record = Record{
        .price = Find(input)
            .and_then([](const Doc& doc) -> std::optional<Price> {
                Prepare(doc);
                return Extract(doc);
            })
            .transform([](const Price& price) { return Format(price); }),
    };
}

auto BlankLinesBetweenMultilineListItems() {
    return visit(
        Overloaded{
            [](const A& a) {
                Check(a);
                return a.value;
            },

            [](const B& b) {
                Check(b);
                return b.value;
            },
        },
        value
    );
}

template <typename R>
auto GenericLambdaInTemplate() {
    return std::visit(
        []<typename T>(const T& value) -> std::optional<R> {
            if constexpr (std::is_same_v<T, R>) {
                return value;
            }
            return std::nullopt;
        },
        variant
    );
}

template <typename R>
auto ConstrainedGenericLambdaInTemplate() {
    return Visit([]<typename T>(T value) requires(Accepts<T>) { Prepare(value); return value; }, value);
}

template <template <typename> class Container, typename Value>
struct TemplateTemplateParameterOwner;

void RangeForExpressionSpacing() {
    for (const auto& item : (*items)->values) { Use(item); }
    for (const auto& item : [](auto source) { return source; }(items)) { Use(item); }
    for (int item : {1, 2, 3}) { Use(item); }
    for (const auto& item : ::Items()) { Use(item); }
    auto value = flag ? (left) : (right);
}

void PreserveListBlankLines() {
    Consume(first,


        second, third);
    auto values = Values{first,

        second};
    Consume([] { Prepare(); Finish(); },

        value);
}

void PreserveParameterBlankLines(int first,

    int second);

void PreserveFinalParameterBlankLine(
int first,
int second

);

struct PreserveBaseListBlankLines : First,

    Second {};

void StreamPairsRequireNonLiteralValues() {
    output << "message" << "continued " "message" << "number=" << 42 << "boolean=" << true << false << "empty=" << nullptr << "duration=" << 12_ms << "text=" << "hello"s << 'x' << ':' << "name=" << name << ' ' << total;
    output << "message" << "hex=" << std::hex << 42 << "value=" << std::hex << std::setw(8) << value << "literal=" << std::hex << "tail" << "unfinished=" << std::hex;
}

auto VisitWithCompetingLambdaHeaders() {
    return std::visit(Overloaded{
        [context](const RequestConfiguration& config, const ResponseParameters& response_parameters) -> std::optional<Result> {
            Prepare();
            return {};
        }}, value);
}

void InlineBlockCommentSpacing() {
Configure( /* mode = */"default",/* options = */{},/* enabled = */true);
Consume(/* number = */42,/* character = */'x',/* null = */nullptr);
Consume(/* wide = */L"text",/* raw = */R"(raw)",/* suffix = */"text"sv);
Consume(/* first */  /* second */"value");
Consume(value/* explanation */ ,other);
Consume( /* no arguments */ );
auto index = items[ /* index = */0/* last */ ];
auto grouped = ( /* first */value/* last */ );
auto values = Values{ /* first */1,/* second */2/* last */ };
auto empty = Values{ /* no values */ };
auto constructed = Type/* explanation */{};
auto initialized = Type/* explanation */{1};
auto lambda = [ /* capture */value/* last */ ] { return /* explanation */value; };
bool negated = !/* explanation */ready;
bool inversion = /* explanation */!ready;
auto dereferenced = * /* explanation */pointer;
auto indirect = /* explanation */*pointer;
auto address = /* explanation */&value;
auto negative = /* explanation */-value;
auto member = object/* explanation */.member;
auto pointerMember = pointer/* explanation */->member;
auto memberPointer = object/* explanation */.*member;
auto pointerMemberPointer = pointer/* explanation */->*member;
auto selected = object./* explanation */member;
auto pointed = pointer->/* explanation */member;
auto qualified = ns::/* explanation */value;
auto scope = ns/* explanation */::value;
auto invoked = Run/* explanation */(value);
auto sum = left/* explanation */+right;
auto difference = left-/* explanation */right;
auto lesser = left</* explanation */right;
auto greater = left/* explanation */>right;
value/* explanation */++;
value++/* explanation */ ;
output++=value;
++output=value;
for (int i = 0/* initial */ ;i < count/* condition */ ;++i) { Use(i); }
return /* explanation */result;
}

struct CommentContinuationLayout {
    int count;  // Number of active items
                // including pending ones.
    bool ready;

    int multiple;  // first line
                   // second line
                   // third line
    bool after_multiple;

    int independent;  // describes independent
    // Describes the next field, not independent.
    bool next;

    int   shifted;      // spacing-normalized anchor
                        // follows the formatted anchor
    bool after_shifted;

    int aMuchLongerName;  // first
    int x;  // anchor moved by trailing-comment alignment
            // follows the moved anchor
    bool after_alignment;
};

void TrailingCommentAlignmentGroups(bool a, bool b, bool c, bool d) {
    if (a || // first
        b || // second
        (c && d) // third
    ) { Run(); }
    Use(
        a + // first operand
        b + // second operand
        Nested(c) // third operand
    );
    Use(
        Nested(
            a // nested group
        ), // outer group
        b
    );
}

void UnnamedCommentedParameter(Type&/* unused */ );

template < /* parameter */typename T/* last */ >
struct CommentedTemplateArguments {
using Type = Container< /* argument */T/* last */ >;
using Compared = Container<(left/* before */</* after */right), (left/* before */>/* after */right)>;
bool operator</* name */(const T&);
bool operator/* name */>(const T&);
};

[[ /* attribute */maybe_unused/* last */ ]] int commentedAttribute;

#define COMMENTED_ARGUMENTS(value) Configure(/* mode = */"default",/* options = */{},/* value = */value)

void BlockCommentLineBoundaries() {
/* standalone */
Use();/* trailing */
Finish(); // trailing line comment
Call( /* arguments */
value);
auto values = Values{ /* items */
1};
Call(/* first */value, /* more */
other);
}

void CommentedBinaryOperators() {
const auto count = items.size() // items
+ separators.size() // separators
+ before + after;
const auto already = items.size() + // items
separators.size() + // separators
before + after;
auto blocks = first /* first */
+ second + third;
auto standalone = first
// next term
+ second + third;
auto mixedComments = first // first term
// next term
+ second + third;
auto bothSides = first // first term
+ // operator note
second + third;
auto product = first // first factor
* second * third;
auto bits = (first // mask
& second) | (third ^ fourth);
if (enabled // feature
&& ready // readiness
&& available) { Run(); }
if (enabled and // feature
ready or fallback) { Run(); }
auto nested = (first // inner term
+ second) * factor // outer term
+ third;
Consume(first // argument term
+ second, third);
auto inlineComment = first /* inline */ + second;
}

void CommentedStreamOperators() {
output << // first value
first << // second value
second << third;
output // first value
<< first // second value
<< second << third;
input >> /* first value */
first >> second;
output <<
// next insertion
first << second;
output
// next insertion
<< first << second;
output // receiver
<< // first insertion
first << second;
output << "name=" << // name
name << "count=" << count;
output << std::hex << // hexadecimal value
value << other;
output << "value=" << std::hex << // hexadecimal value
value << "tail=" << tail;
Consume(output << // argument insertion
value, other);
output << (first // sum term
+ second) << third;
auto result = first + (output << // nested insertion
value);
output << /* inline */ value;
}

#define COMMENTED_SUM(first, second) \
    (first /* first term */ \
        + second)
#define COMMENTED_STREAM(out, value) \
    out << /* insertion */ \
        value

void CommentedOperatorsAcrossBlocks() {
auto sum = Make([]{First();Second();}) // term
+ value + other;
output << Make([]{First();Second();}) << // final
value << other;
}

void CommentedOperatorsWithBlankLines() {
auto sum = first // first

+ second + third;
output << // first

first << second;
}

void MultilineOperatorComments() {
auto sum = first /* first term
continued */
+ second + third;
output << /* first insertion
continued */
first << second;
}

auto JoinLabelAtoms() {
return first_attribute_label + "|" + second_attribute_label + "|" + third_attribute_label + "|" + fourth_attribute_label;
}

void AdditionLiteralPairs() {
auto compact = "name=" + value;
auto forward = first // first
+ "name=" + value + "count=" + count;
auto startsWithLiteral = "name=" + value + other // other
+ last;
auto literalFollowers = first // first
+ "one" + "two" + value + ':' + 'x' + next + "number=" + 42 + last;
auto otherLiterals = first // first
+ 42 + value + true + ready + nullptr + tail;
auto stringKinds = first // first
+ L"wide=" + wide + u8"utf8=" + utf8 + R"(raw=)" + raw + "owned="s + owned;
auto fragments = first // first
+ "longer " "label=" + value + '\n' + tail;
auto expressions = first // first
+ "call=" + Build(value) + "field=" + object.member + "nested=" + (left + right);
auto configuredName = first // first
+ "hex=" + std::hex + value;
auto mixed = first // first
+ "product=" + value * factor + "difference=" + (left - right);
auto otherOperators = first // first
* "factor=" * value * "next=" * other;
auto logical = first // first
|| "value=" || value || "next=" || other;
}

void AdditionPairsWithComments() {
auto trailing = first + "label=" // label
+ value + "tail=" + tail;
auto followerComment = first // first
+ "label=" + value // value
+ "tail=" + tail;
auto standalone = first + "label=" +
// value
value + "tail=" + tail;
auto blocks = first // first
+ "label=" /* label */ + value + "tail=" + tail;
}

void NestedAdditionPairs() {
Use(first // first
+ "label=" + value + "tail=" + tail, other);
auto nested = outer + (first // first
+ "label=" + value + "tail=" + tail);
output << "joined=" << (first // first
+ "label=" + value + "tail=" + tail);
auto callback = first // first
+ "value=" + Make([] { First(); Second(); }) + "tail=" + tail;
}

void LiteralPairsWithBodiesAndNewlines() {
auto empty = first // first
+ "value=" + Make([] {}) + "tail=" + tail;
auto single = first // first
+ "value=" + Make([] { return value; }) + "tail=" + tail;
output << "value=" << Make([] { First(); Second(); }) << "tail=" << tail;
auto multiline = first // first
+ R"(first
second)" + value + "tail=" + tail;
output << R"(first
second)" << value << "tail=" << tail;
auto fragments = first // first
+ "line\n" "next" + value + "tail=" + tail;
}

void CompactBodiesStartingWithGlobalScope() {::RunGlobal();}
auto compactGlobalLambda = [] {::RunGlobal();};
