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
