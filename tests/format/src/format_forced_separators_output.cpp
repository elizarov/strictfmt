namespace BinaryOperators {

auto sum = first
#define MARKER 1
    + second;

auto difference = first
#define MARKER 1
    - second;

auto product = first
#define MARKER 1
    * second;

auto quotient = first
#define MARKER 1
    / second;

auto remainder = first
#define MARKER 1
    % second;

auto bits_and = first
#define MARKER 1
    & second;

auto bits_or = first
#define MARKER 1
    | second;

auto bits_xor = first
#define MARKER 1
    ^ second;

auto logical_and = first
#define MARKER 1
    && second;

auto logical_or = first
#define MARKER 1
    || second;

auto equal = first
#define MARKER 1
    == second;

auto unequal = first
#define MARKER 1
    != second;

auto less = first
#define MARKER 1
    < second;

auto greater = first
#define MARKER 1
    > second;

auto less_equal = first
#define MARKER 1
    <= second;

auto greater_equal = first
#define MARKER 1
    >= second;

auto ordering = first
#define MARKER 1
    <=> second;

auto shift_left = first
#define MARKER 1
    << second;

auto shift_right = first
#define MARKER 1
    >> second;

auto member_pointer = first
#define MARKER 1
    .*second;

auto pointer_member = first
#define MARKER 1
    ->*second;

auto alternative_and = first
#define MARKER 1
    and second;

auto alternative_or = first
#define MARKER 1
    or second;

auto alternative_bitand = first
#define MARKER 1
    bitand second;

auto alternative_bitor = first
#define MARKER 1
    bitor second;

auto alternative_xor = first
#define MARKER 1
    xor second;

auto alternative_unequal = first
#define MARKER 1
    not_eq second;

}

namespace AssignmentOperators {

void Assign() {
    value
#define MARKER 1
        = second;
}

void AddAssign() {
    value
#define MARKER 1
        += second;
}

void SubtractAssign() {
    value
#define MARKER 1
        -= second;
}

void MultiplyAssign() {
    value
#define MARKER 1
        *= second;
}

void DivideAssign() {
    value
#define MARKER 1
        /= second;
}

void RemainderAssign() {
    value
#define MARKER 1
        %= second;
}

void AndAssign() {
    value
#define MARKER 1
        &= second;
}

void OrAssign() {
    value
#define MARKER 1
        |= second;
}

void XorAssign() {
    value
#define MARKER 1
        ^= second;
}

void ShiftLeftAssign() {
    value
#define MARKER 1
        <<= second;
}

void ShiftRightAssign() {
    value
#define MARKER 1
        >>= second;
}

void AlternativeAndAssign() {
    value
#define MARKER 1
        and_eq second;
}

void AlternativeOrAssign() {
    value
#define MARKER 1
        or_eq second;
}

void AlternativeXorAssign() {
    value
#define MARKER 1
        xor_eq second;
}

}

namespace angle_comma {

using Type = Box<
    First
#define MARKER 1
    , Second,
    Third
>;

}

namespace arrow {

auto value = first
#define MARKER 1
    ->Member()
    ->Other();

}

namespace bare_comma {

void Run() {
    first
#define MARKER 1
        , second, third;
}

}

namespace base_comma {

struct Derived :
    First
#define MARKER 1
    , Second,
    Third {};

}

namespace between {

auto value = first
#define MARKER 1
    +
#define SECOND_MARKER 2
    second;

}

namespace both_ternary {

auto value = first
#define MARKER 1
    ? second
#pragma marker
    : third;

}

namespace call {

void Run() {
    Call(
        first + second
#define MARKER 1
            + third +
            fourth
    );
}

}

namespace call_assignment {

void Run() {
    Call(
        value
#define MARKER 1
            = second
    );
}

}

namespace cast {

auto value =
#define MARKER 1
    static_cast<Type>(first);

}

namespace chain {

auto value = first + second
#define MARKER 1
    + third +
    fourth;

}

namespace chain_many {

auto value = first
#define MARKER 1
    + second
#define SECOND_MARKER 2
    + third;

}

namespace colon {

auto value = first ? second
#define MARKER 1
    : third;

}

namespace comma_between {

void Run() {
    Call(
        first
#define MARKER 1
        ,
#pragma marker
        second
    );
}

}

namespace comma_comment {

void Run() {
    Call(
        first
#define MARKER 1
        // next item
        , second,
        third
    );
}

}

namespace comment {

auto value = first
#define MARKER 1
    // value comment
    + second;

}

namespace comment_after {

auto value = first
#define MARKER 1
    +  // value comment
    second;

}

namespace continued_lambda {

auto value = first
#define MARKER 1
    + [] {
        Prepare();
        return Next();
    }() +
    last;

}

namespace ctor_comma {

C::C() :
    first(1)
#define MARKER 1
    , second(2),
    third(3) {}

}

namespace decl_comma {

int first
#define MARKER 1
    , second,
    third;

}

namespace directive_between_operators {

auto value = first
#define MARKER 1
    +
#define SECOND_MARKER 2
    second +
    third;

}

namespace enum_comma {

enum Value {
    First
#define MARKER 1
        ,
    Second,
    Third
};

}

namespace expr_comma {

void Run() {
    (
        first
#define MARKER 1
        , second,
        third
    );
}

}

namespace global_qualified {

auto value =
#define MARKER 1
    ::Name::Member;

}

namespace if_condition {

void Run() {
    if (
        first && second
#define MARKER 1
        && third &&
        fourth
    ) {
        Pass();
        Done();
    }
}

}

namespace if_ordinary {

void Run() {
    if (
        first
#define MARKER 1
            == second
    ) {
        Pass();
        Done();
    }
}

}

namespace init_comma {

auto values = {
    first
#define MARKER 1
    , second,
    third
};

}

namespace list {

void Run() {
    Call(
        first
#define MARKER 1
        , second,
        third
    );
}

}

namespace long_atom {

auto value = first
#define MARKER 1
    + OneUnavoidablyLongAtomicIdentifierThatOverflows;

}

namespace long_colon {

auto value = first ? second
#define MARKER 1
    : ThirdVeryLongValue ? FourthVeryLongValue :
    FifthVeryLongValue;

}

namespace long_question {

auto value = first
#define MARKER 1
    ? FirstVeryLongValue : SecondVeryLongValue;

}

namespace long_rhs {

auto value = first
#define MARKER 1
    + Call(
        FirstLongArgument,
        SecondLongArgument,
        ThirdLongArgument
    );

}

namespace member {

auto value = first
#define MARKER 1
    .Member()
    .Other();

}

namespace nested_ternary {

auto value = first ? second
#define MARKER 1
    : third ? fourth :
    fifth;

}

namespace parameter_comma {

void Run(
    First first
#define MARKER 1
    , Second second,
    Third third
);

}

namespace paren {

auto value = (
    first
#define MARKER 1
    + second +
    third
);

}

namespace postfix_decrement {

void Run() {
    first
#define MARKER 1
        --;
}

}

namespace postfix_increment {

void Run() {
    first
#define MARKER 1
        ++;
}

}

namespace qualification_long {

auto value = Name
#define MARKER 1
    ::AnotherVeryLongName::
    FinalVeryLongName;

}

namespace qualifier {

auto value = Name
#define MARKER 1
    ::Member;

}

namespace question {

auto value = first
#define MARKER 1
    ? second : third;

}

namespace raw_directive {

auto value = first
#undef MARKER
    + second
#pragma marker
    - third;

}

namespace return_arrow {

auto Run()
#define MARKER 1
    -> SomeReturnType;

}

namespace return_long_arrow {

auto Run()
#define MARKER 1
    -> SomeVeryLongReturnType<
        First, Second, Third
    >;

}

namespace split_before_directive {

auto value = FirstVeryLongValue +
    SecondVeryLongValue +
    ThirdVeryLongValue
#define MARKER 1
    + FourthVeryLongValue +
    FifthVeryLongValue;

}

namespace unary {

auto value =
#define MARKER 1
    -first;

}

namespace unary_increment {

void Run() {
    value =
#define MARKER 1
        ++first;
}

}

namespace unary_lambda {

auto action = +[] { return 1; };

}

namespace unary_not {

auto value =
#define MARKER 1
    !first;

}

namespace long_comma_items {

void Run() {
    Call(
        first
#define LONG_ITEM_MARKER 1
        , VeryLongFunction(
            FirstLongArgument,
            SecondLongArgument
        ),
        third
    );
}

using Type = Box<
    First
#define LONG_TYPE_MARKER 1
    , SecondUnavoidablyLongAtomicTypeNameThatOverflows,
    Third
>;

}
namespace comma_lambda {

void Run() {
    Call(
        first
#define LAMBDA_ITEM_MARKER 1
        , [] {
            Prepare();
            return Value();
        },
        third
    );
}

}

namespace alternative_unary {

auto negative =
#define NEGATIVE_MARKER 1
    not first;

auto inverted =
#define INVERT_MARKER 1
    compl first;

auto mixed = first bitand second
#define MIXED_MARKER 1
    & third bitand
    fourth;

}
