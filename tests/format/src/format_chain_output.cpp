// one or initializer
bool f() {
    return Check({
        first_long_value,
        second_long_value,
        third_long_value
    }) ||
        other;
}

// two ors first initializer
bool f() {
    return Check({
        first_long_value,
        second_long_value,
        third_long_value
    }) ||
        other ||
        third;
}

// two ors middle initializer
bool f() {
    return first ||
        Check({
            first_long_value,
            second_long_value,
            third_long_value
        }) ||
        third;
}

// two ors last initializer
bool f() {
    return first || other || Check({
        first_long_value,
        second_long_value,
        third_long_value
    });
}

// one or call
bool f() {
    return Check(
        first_long_value,
        second_long_value,
        third_long_value
    ) ||
        other;
}

// two ors first call
bool f() {
    return Check(
        first_long_value,
        second_long_value,
        third_long_value
    ) ||
        other ||
        third;
}

// two ors middle call
bool f() {
    return first ||
        Check(
            first_long_value,
            second_long_value,
            third_long_value
        ) ||
        third;
}

// one or lambda
bool f() {
    return Check([] {
        Prepare();
        return value;
    }) ||
        other;
}

// two ors first lambda
bool f() {
    return Check([] {
        Prepare();
        return value;
    }) ||
        other ||
        third;
}

// two ors middle lambda
bool f() {
    return first ||
        Check([] {
            Prepare();
            return value;
        }) ||
        third;
}

// ordinary equals
bool f() {
    return Check({
        first_long_value,
        second_long_value,
        third_long_value
    }) == other;
}

// parenthesized prefix
bool f() {
    return (Check(
        first_long_value,
        second_long_value,
        third_long_value
    )) ||
        other;
}

// two ors parenthesized prefix
bool f() {
    return (Check(
        first_long_value,
        second_long_value,
        third_long_value
    )) ||
        other ||
        third;
}

// two ors first single statement lambda
bool f() {
    return Check([] {
        return first_long_value +
            second_long_value +
            third_long_value;
    }) ||
        other ||
        third;
}

// two ors middle single statement lambda
bool f() {
    return first ||
        Check([] {
            return first_long_value +
                second_long_value +
                third_long_value;
        }) ||
        third;
}

// two ors final lambda
bool f() {
    return first || other || Check([] {
        Prepare();
        return value;
    });
}

// two ors condition
void f() {
    if (
        Check({
            first_long_value,
            second_long_value,
            third_long_value
        }) ||
        other ||
        third
    ) {
        Run();
    }
}

// one or condition
void f() {
    if (
        Check({
            first_long_value,
            second_long_value,
            third_long_value
        }) ||
        other
    ) {
        Run();
    }
}

// two plus first initializer
auto f() {
    return Check({
        first_long_value,
        second_long_value,
        third_long_value
    }) +
        other +
        third;
}

// member receiver
auto f() {
    return Check([] {
        Prepare();
        return value;
    }).Next().Done();
}

// call receiver
auto f() {
    return Check([] {
        Prepare();
        return value;
    })(x)(y);
}

// unrelated sibling chain
void f() {
    Use(
        [] {
            Prepare();
            Finish();
        },
        a || b || c
    );
}

// single or unary
bool f() {
    return !Check({
        first_long_value,
        second_long_value,
        third_long_value
    }) ||
        !other;
}

// single pipe lambda
auto f() {
    return Check([] { return value; }) |
        next;
}

// single and call
bool f() {
    return Check(
        first_long_value,
        second_long_value,
        third_long_value
    ) &&
        other;
}

// raw nonfinal with expanded tail
auto f() {
    return R"(a
b)" +
        Check(
            first_long_value,
            second_long_value,
            third_long_value
        );
}

// nested chain after block
void f() {
    Use(
        Check([] {
            Prepare();
            return value;
        }) ||
            first,
        second || third
    );
}

// first block in parentheses
bool f() {
    return (Check([] {
        Prepare();
        return value;
    })) ||
        other ||
        third;
}

// first requires body
template <class T>
concept C = requires(T x) {
    x.f();
    x.g();
} &&
    Ready<T>;

namespace CommentedChainIndentation {

void Check(
    bool firstCondition,
    bool secondCondition
) {
    if (
        firstCondition &&
        secondCondition /* tail */
    ) {
        return;
    }
    while (
        /* head */ firstCondition ||
        secondCondition /* one */ /* two */
    ) {
        break;
    }
    if (
        firstCondition and
        secondCondition /* tail */
    ) {
        return;
    }
    if (
        firstCondition ||
        secondCondition  // tail
    ) {
        return;
    }
    if (
        /* outer */ (
            /* inner */ firstCondition &&
            secondCondition /* inner tail */
        ) /* outer tail */
    ) {
        return;
    }
    for (
        ;
        firstCondition &&
            secondCondition /* tail */;
    ) {
        break;
    }
    auto consume = [](bool value) {
        return value;
    };
    consume(
        firstCondition &&
            secondCondition /* argument */
    );
    if (consume(
        firstCondition &&
            secondCondition
    )) {
        return;
    }
    if (
        consume(
            firstCondition &&
                secondCondition /* argument */
        ) /* condition */
    ) {
        return;
    }
    if (
        firstCondition = firstCondition &&
            secondCondition /* assignment */
    ) {
        return;
    }
    auto sum = (
        firstCondition +
        secondCondition /* sum */
    );
    (void)sum;
}

}

// A parenthesized callee starts its chain before the first applied argument list.
DEFINE_FIELDS(
    Record,
    (Integer, first_field)
        (String, second_field)
        (Integer, third_field)
        (String, fourth_field)
);
DEFINE_FIELDS(
    Record,
    (Vector<Pair<int, Value>>, items)
        (bool, valid)(String, name)
);
void ParenthesizedCallReceivers() {
    Consume((one)(two)(three));
    Consume(
        (first_value_with_long_name)(
            second_value_with_long_name
        )
    );
    Consume(
        (first_value)
            (second_value)
            (third_value)
            (fourth_value)
    );
    (callable)
        (first_argument)
        (second_argument)
        (third_argument);
    ((callable))
        (first_argument)
        (second_argument);
    (
        Choose(
            first_option, second_option
        )
    )(third_argument)(fourth_argument);
    (FirstFactory()(first_argument))
        (second_argument)
        (third_argument);
    (Choose([] {
        First();
        Second();
    }))
        (first_argument)
        (second_argument);
    callable(first_argument)
        (second_argument)
        (third_argument);
}
