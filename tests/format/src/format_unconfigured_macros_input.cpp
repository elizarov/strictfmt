// Parenthesized fragments reuse call arguments at every nesting depth.
struct Interface {
DECLARE_METHOD((Map<int, Value>), Find, ((const Map<int, Value>&)), (const, override));
DECLARE_METHOD(void, Visit, (Value&), (const, ref(&), override));
DECLARE_METHOD(void, Replace, (), (const, ref(&&), override));
};
void NestedFragments(){
TOKENS((,), ((const, override)), (((int, field))));
}
