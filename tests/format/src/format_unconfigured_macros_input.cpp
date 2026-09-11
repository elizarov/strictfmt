// Parenthesized fragments reuse call arguments at every nesting depth.
struct Interface {
DECLARE_METHOD((Map<int, Value>), Find, ((const Map<int, Value>&)), (const, override));
DECLARE_METHOD(void, Visit, (Value&), (const, ref(&), override));
DECLARE_METHOD(void, Replace, (), (const, ref(&&), override));
};
void NestedFragments(){
TOKENS((,), ((const, override)), (((int, field))));
}

// Standalone expansion items coexist with ordinary declarations.
BEGIN_NAMESPACE
namespace nested {
API_EXPORT int value;
API_EXPORT API_EXPORT int Read(){return value;}
struct Record {
GENERATED_MEMBERS
API_EXPORT void Update();
GENERATED_MEMBERS
};
}
END_NAMESPACE

// Unknown modifiers remain parseable throughout class headers.
class [[nodiscard]] API_EXPORT Annotated : public Base {
class API_EXPORT(tag(Nested(value))) NestedType final : public Base {};
};
struct API_EXPORT(tag) AnnotatedRecord {int field;};
union API_EXPORT AnnotatedValue {int number;char byte;};
template<class T> class API_EXPORT Template : public Base<T> {};
class API_EXPORT Forward;
class Ordinary* pointer;
class Ordinary function();
auto RecordFunctionPointer() -> struct Ordinary (*)(int) {return nullptr;}

// Complete statements share their ordinary recursive grammar inside arguments.
void StatementArguments(){
CHECK_STATEMENT(;, Error);
CHECK_STATEMENT(CHECK_STATEMENT(;, Error), Error);
CHECK_STATEMENT(;, Error)<<message;
CHECK_STATEMENT(return;, Error);
CHECK_STATEMENT(do Work(); while(ready);, Error);
CHECK_STATEMENT(for(auto item:items) Consume(item);, Error);
CHECK_STATEMENT(if(ready) do Work(); while(More()); else return;, Error);
CHECK_STATEMENT(while(ready){if(done) break;continue;}, Error);
CHECK_STATEMENT(try{Work();}catch(const Error& error){throw;}, Error);
}
void StatementArgumentBoundaries(){
Inspect(first,second,Consume(value););
Inspect(return;,value,);
}
template<class... Values> void FoldArguments(Values... values){
(Consume(values),...);
}

// Function annotations reuse the class-header modifier grammar.
struct AnnotatedMethods {
AnnotatedMethods() ANNOTATION;
~AnnotatedMethods() noexcept ANNOTATION {}
Value Get() const & noexcept ANNOTATION;
Value Read() ANNOTATION(lock(Nested(value)));
virtual void Update() ANNOTATION(lock) =0;
virtual void Refresh() ANNOTATION final;
auto Compute() ANNOTATION -> Value;
};
void Function() ANNOTATION {}
typedef void (*AnnotatedCallback)() ANNOTATION;
using AnnotatedFunction=void(*)() ANNOTATION;
void (*callback)() ANNOTATION;

// These ambiguous shapes can use declaration annotations until roles are configured.
struct AnnotationFragments {
static Result ANNOTATION Callback(Window window);
Value value ANNOTATION;
};
void AdjacentCallFragments(){
TOKENS(call() token);
TOKENS((token) suffix);
CHECK_STATEMENT(ns::Value variable("name", Nested(input)));
CHECK_STATEMENT(ns::Value variable(std::move(input)));
STEP(first)
STEP(second)
STEP(third)
return;
}
void MixedCallTerminators(){
STEP(first)
STEP(second);
STEP(third)
STEP(fourth);
}

// Explicit roles also disambiguate newly structured replacement fragments.
#define DECLARE_ACCESSOR(Name) Value Name() ANNOTATION
#define FORWARD_ARGUMENTS(args) TOKENS(dummy STEP(ARG,,args))
#define EXPAND_ELEMENT(i,element) STEP(i) element

// Alias annotations use the same unknown-modifier fallback as declarations.
template<class T> using OldValue ANNOTATION = T;
namespace aliases {using OldPointer ANNOTATION = void*;}
template<class T> using OlderValue ANNOTATION(reason(Nested(value))) = T;

// Adjacent parenthesized fragments recurse through the common argument grammar.
void AdjacentParenthesizedFragments(){
TOKENS((int,field)(bool,other));
TOKENS(((int,field)(bool,other))((Map<Key,Value>,entries)));
TOKENS(prefix(int,field) middle(bool,other) suffix);
CHECK_STATEMENT(Value value(input),Error);
CHECK_STATEMENT(Value value(Convert(data.As<ns::Input>())),Error);
CHECK_STATEMENT(Value value({{"label",1,2,3}}),Error);
}

// Unconfigured calls may terminate statements without an explicit semicolon.
void StatementsWithoutTerminators(){
STEP(first)
Consume(value);
STEP(second);
if(ready) STEP(third) else STEP(fourth)
while(ready) STEP(next)
for(auto item:items) STEP(item)
do STEP(value) while(ready);
switch(value){case 0: STEP(value) break;default: STEP(fallback)}
STEP(done)
}
#define RUN_STEP(value) do {STEP(value)} while(false)

// A complete C++ call retains its callee when intermediate calls could end a statement.
template<class F,class T> void Apply(F&& function,T&& value){
ns::forward<F>(function)(ns::forward<T>(value));
}

// Namespace and block call alternatives keep equal preference through conditionals.
#ifndef ENABLE_OPTIONS
DECLARE_OPTION(option3);
DECLARE_OPTION(option4);
DECLARE_COUNT(option6);
DECLARE_OPTION(option7);
DECLARE_COUNT(option8);
DECLARE_COUNT(option9);
DECLARE_OPTION(option10);
DECLARE_OPTION(option11);
DECLARE_COUNT(option12);
DECLARE_OPTION(option13);
DECLARE_TEXT(option15);
#if ENABLE_EXTRA
DECLARE_TEXT(option17);
#endif
namespace call_types {
namespace detail {
class [[nodiscard]] Comparison {
  template <
      typename traits::EnableIf<!traits::IsIntegral<Left>::value ||
                              !traits::IsPointer<Right>::value>::type* = nullptr>
  static Result Compare(
      traits::Null , Value* right) {
    return CompareValues(left_text, right_text, static_cast<Value*>(nullptr),
                       right);
  }
};
Result Describe(const char* left_text, const char* right_text,
                                   const char* op) {
  return MakeResult()
         << " vs " << FormatValues(left, right);
}
class API_EXPORT [[nodiscard]] Helper {
  Helper(Event::Kind type, const char* file, int line,
               traits::String message);
  struct Data {
  };
};
}
Info* Register(const char* suite, const char* test,
                       const char* file, int line, Factory factory) {
  return detail::RegisterInfo(
      new FactoryAdapter{traits::Move(factory)});
}
}
#endif

// The call fallback also supplies whole enum and initializer-list fragments.
enum class ExpandedFields {First=0,STEP(Fields) Last,};
enum class AdjacentFields {STEP(Read) STEP(Write)};
auto expanded_values={STEP(Read) STEP(Write)};
auto mixed_values={1,STEP(Middle)2};
auto nested_values={STEP(Nested({1,2})) STEP(Other())};
enum class ConditionalFields {
#if ENABLE_FIELDS
STEP(Extra)
#else
STEP(Default)
#endif
Last,
};

// Adjacent macro items leave the following declaration's header intact.
STEP(Warnings)
ACTION(ReturnValue){return 5;}
STEP(EndWarnings)
TEST(Items,Declarations){Consume(value);}
API_EXPORT(annotation) ns::Result GetResult(){return {};}
namespace scoped_items {
STEP(Metric,(name,"description"))
ns::Result Read(){return {};}
STEP(Last)
}
struct GeneratedItems {STEP(First) STEP(Second) void Method();};
#define GENERATED_ITEMS(Name) struct Name {STEP(Fields) STEP(Methods)}
REGISTER_CASE(Record,1,2) ANNOTATION;

// Enum fragments can supply values or separators without configuration.
enum class PrefixedValues {First=ENUM_PREFIX Value,Second=ENUM_PREFIX Other,};
enum class PrefixedItems {ENUM_PREFIX First,ENUM_PREFIX Last};
struct EnumFragments {
enum Nested {First=ENUM_PREFIX (Compute(1)+2),Last,};
};
void LocalEnumFragments(){enum Local {First=ENUM_PREFIX Value,Last,};}
enum class ConditionalSeparators {
#if ENABLE_EXTRA
First
#else
Second
#endif
Last,
};
