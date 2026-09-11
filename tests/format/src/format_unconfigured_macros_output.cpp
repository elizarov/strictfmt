// Parenthesized fragments reuse call arguments at every nesting depth.
struct Interface {
    DECLARE_METHOD((Map<int, Value>),
        Find,
        ((const Map<int, Value>&)),
        (const, override));
    DECLARE_METHOD(void, Visit, (Value&), (const, ref(&), override));
    DECLARE_METHOD(void, Replace, (), (const, ref(&&), override));
};

void NestedFragments() { TOKENS((, ), ((const, override)), (((int, field)))); }

// Standalone expansion items coexist with ordinary declarations.
BEGIN_NAMESPACE
namespace nested {

API_EXPORT int value;

API_EXPORT API_EXPORT int Read() { return value; }

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

struct API_EXPORT(tag) AnnotatedRecord {
    int field;
};

union API_EXPORT AnnotatedValue {
    int number;
    char byte;
};

template <class T>
class API_EXPORT Template : public Base<T> {};

class API_EXPORT Forward;

class Ordinary* pointer;

class Ordinary function();
auto RecordFunctionPointer() -> struct Ordinary (*)(int) { return nullptr; }
