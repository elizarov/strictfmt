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

// Complete statements share their ordinary recursive grammar inside arguments.
void StatementArguments() {
    CHECK_STATEMENT(;, Error);
    CHECK_STATEMENT(CHECK_STATEMENT(;, Error), Error);
    CHECK_STATEMENT(;, Error) << message;
    CHECK_STATEMENT(return;, Error);
    CHECK_STATEMENT(
        do {
            Work();
        } while (ready);,
        Error
    );
    CHECK_STATEMENT(
        for (auto item : items) {
            Consume(item);
        },
        Error
    );
    CHECK_STATEMENT(
        if (ready) {
            do {
                Work();
            } while (More());
        } else {
            return;
        },
        Error
    );
    CHECK_STATEMENT(
        while (ready) {
            if (done) {
                break;
            }
            continue;
        },
        Error
    );
    CHECK_STATEMENT(
        try {
            Work();
        } catch (const Error& error) {
            throw;
        },
        Error
    );
}
void StatementArgumentBoundaries() {
    Inspect(
        first,
        second,
        Consume(value);
    );
    Inspect(
        return;,
        value,
    );
}
template <class... Values>
void FoldArguments(Values... values) { (Consume(values), ...); }

// Function annotations reuse the class-header modifier grammar.
struct AnnotatedMethods {
    AnnotatedMethods() ANNOTATION;
    ~AnnotatedMethods() noexcept ANNOTATION {}
    Value Get() const & noexcept ANNOTATION;
    Value Read() ANNOTATION(lock(Nested(value)));
    virtual void Update() ANNOTATION(lock) = 0;
    virtual void Refresh() ANNOTATION final;
    auto Compute() ANNOTATION -> Value;
};

void Function() ANNOTATION {}

typedef void (*AnnotatedCallback)() ANNOTATION;
using AnnotatedFunction = void (*)() ANNOTATION;

void (*callback)() ANNOTATION;

// These ambiguous shapes can use declaration annotations until roles are configured.
struct AnnotationFragments {
    static Result ANNOTATION Callback(Window window);

    Value value ANNOTATION;
};

void AdjacentCallFragments() {
    TOKENS(call() token);
    TOKENS((token) suffix);
    CHECK_STATEMENT(ns::Value variable("name", Nested(input)));
    CHECK_STATEMENT(ns::Value variable(std::move(input)));
    STEP(first)
    STEP(second)
    STEP(third)
    return;
}
void MixedCallTerminators() {
    STEP(first)
    STEP(second);
    STEP(third)
    STEP(fourth);
}

// Explicit roles also disambiguate newly structured replacement fragments.
#define DECLARE_ACCESSOR(Name) Value Name() ANNOTATION
#define FORWARD_ARGUMENTS(args) TOKENS(dummy STEP(ARG,, args))
#define EXPAND_ELEMENT(i, element) STEP(i) element
