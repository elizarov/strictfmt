namespace ContinuedTypeHeaders {

namespace class_value_base {

struct Derived :
    FirstBase, SecondBase, Base<[] {
        Prepare();
        return 1;
    }()>
{
    void Run();

    int value;
};

}

namespace class_lambda_base {

struct Derived :
    FirstBase,
    SecondBase,
    Base<decltype([] {
        Prepare();
        return Value();
    })>
{
    void Run();

    int value;
};

}

namespace class_comment {

struct Derived :
    FirstBase,  // first
    SecondBase,
    ThirdBase
{
    void Run();

    int value;
};

}

}

namespace ContinuedControlChains {

namespace if_last_two {

void Run() {
    if (
        First() &&
        [] {
            Prepare();
            return Check();
        }() &&
        Last() &&
        Final()
    ) {
        Pass();
        Done();
    }
}

}

namespace call_lambda_chain {

void Run() {
    Consume(
        First() &&
            [] {
                Prepare();
                return Check();
            }() &&
            Last() &&
            Final()
    );
}

}

namespace array_lambda_chain {

void Run() {
    auto value = array[
        First() +
            [] {
                Prepare();
                return Get();
            }() +
            Last()
    ];
}

}

}

namespace DirectiveLists {

namespace initializer_preprocessor {

Record::Record() :
    first_(FirstValue()),
#if ENABLED
    second_(SecondValue()),
#endif
    last_(LastValue())
{
    Check();
    Done();
}

}

namespace ctor_directive {

Record::Record() :
    first_(FirstValue()),
#define TAG 1
    second_(SecondValue()),
    last_(LastValue())
{
    Check();
    Done();
}

}

namespace class_directive {

struct Derived :
    FirstBase,
#define TAG 1
    SecondBase,
    LastBase
{
    void Run();

    int value;
};

}

namespace list_preprocessor_nested {

void Run() {
    Consume(
        first,
        Other(
            second,
#if ENABLED
            third,
#endif
            fourth
        ),
        last
    );
}

}

namespace list_directive {

void Run() {
    Consume(
        first,
#define TAG 1
        second,
        last
    );
}

}

}

namespace DirectiveChains {

namespace chain_directive {

bool ready = FirstCheck() &&
#define TAG 1
    MiddleCheck() &&
    LastCheck() &&
    FinalCheck();

}

namespace chain_comment {

bool ready = FirstCheck() &&  // first
    MiddleCheck() &&
    LastCheck() &&
    FinalCheck();

}

}

namespace BoundaryControls {

namespace lambda_capture {

auto action = [
    value = [] {
        Prepare();
        return Value();
    }(),
    second = SecondValue(),
    third = ThirdValue()
]() -> ReturnType {
    Run();
    Done();
};

}

namespace lambda_parameter_comment {

auto action = [](
    FirstParameter first,
    SecondParameter second
) {  // body
    Run();
    Done();
};

}

namespace lambda_deep_body_comment {

void Run() {
    Consume(
        [](
            FirstParameter first,
            SecondParameter second,
            ThirdParameter third
        ) ->
            VeryLongReturnType
        {  // body
            Check();
            Done();
        },
        last
    );
}

}

namespace param_default_lambda {

void Run(
    Value v = [] {
        Prepare();
        return Get();
    }(),
    int count = 0
) {
    Check();
    Done();
}

}

}

namespace DirectiveEdges {

Record::Record() :
#if ENABLED
    first_(First()),
#endif
    last_(Last())
{
    Check();
    Done();
}

struct Nested {
    Nested() :
        first_(First())
#if ENABLED
        , second_(Second())
#endif
        , last_(Last())
    {
        Check();
        Done();
    }
};

void NestedLists() {
    Consume(
        first,
        Other(
            second,
#define NESTED_MARKER 1
            third
        ),
        last
    );
}

bool final_operand = First() &&
#undef UNUSED_MARKER
    Last();

bool several = First() &&
#define FIRST_MARKER 1
#define SECOND_MARKER 2
    Second() &&
#pragma marker
    Last();

void FlatDirective() {
    if (
        First() &&
#define CONDITION_MARKER 1
        Last()
    ) {
        Pass();
        Done();
    }
}

}
namespace TypeQueryClosers {

using Type = decltype([] {
    Prepare();
    return Value();
});

auto size = sizeof([] {
    Prepare();
    return Value();
});

auto safe = noexcept([] {
    Prepare();
    return Value();
});

}

namespace BodyBoundaries {

Empty::Empty() :
    first_(First())
#if ENABLED
    , second_(Second())
#endif
{}
Nonempty::Nonempty() :
    first_(First())
#if ENABLED
    , second_(Second())
#endif
{
    Check();
    Done();
}

struct Separated {
    void Run() {
        First();
        Second();
    }

};

void NestedOpeningDelimiter() {
    Consume(
        First(),
#define AFTER_CALL 1
        Second()
    );
}

}
