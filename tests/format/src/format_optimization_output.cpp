// overflow
void f1() {
    if (a) {
        throw (((((
            value
        )))));
    }
}

// delimiter
void f2() {
    if (a) {
        throw ((
            value
        ));
    }
}

// signature
Vector<int>
    func(
        int a
    );

// ternary
void f4() {
    auto x = std::
        optional<
            int
        >{} ?
        F(
            a,
            b
        ) :
        Vector<
            int
        >{};
}

// list item
void f5() {
    auto x = Call(
        (((value))),
        b
    );
}

// flat leaf
void f6() {
    auto x = (
        (a + b)
    );
}

// exact runs
void f7() {
    if (a) {
        if (
            b
        ) {
            throw (((((((((
                ((((((((((
                    (value)
                ))))))))))
            )))))))));
        }
    }
}

// comment spacing at the column limit
void f8() {
    f1(
        /**/ ""
    );
}

// operator and comment suffix width
void f9() {
    a +  //
        b +  //
        c;
}

// comment moved before a stream operator
void f10() {
    Get(
        1, 2
    ) /* x */
        << a
        << b;
}

// literal pair fits with the terminating semicolon
void f11() {
    aa +
        "" + b;
}

// enclosing semicolon prevents a pair
void f12() {
    a +
        "" +
        bb;
}

// trailing chain operator prevents a pair
void f13() {
    a +
        "" +
        b +
        c;
}

// closing parentheses prevent a pair
void f14() {
    F(
        a +
            "" +
            b
    );
}

// pairs stay independent across the chain
void f15() {
    a +
        "" +
        b +
        "" + c;
}

// an expanded operand does not form a pair
void f16() {
    a +
        "" +
        F(
            aa,
            bb
        ) +
        c;
}

// default values use the assignment break, including non-final and unnamed parameters
void f17(
    int x =
        Default
);
void f18(
    T& x =
        Get(),
    int y =
        Default
);
void f19(
    T* =
        Default
);

// defaults with calls, braced values, comments, and nested assignments
void f20(
    T x = Make(
        a, b
    )
);
void
    f21(T x = {
        a, b, c
    });
void f22(
    int x =
        /*d*/ y
);
void f23(
    int x = F(
        a = b
    )
);

// template defaults share the same list-item assignment handling
template <
    int n =
        Default
>
struct A;
template <
    class T =
        A::B
>
struct B;
template <
    template <
        class T =
            A::B
    > class C =
        Box
>
struct C;

// macro continuation suffix prevents a pair
#define JOIN \
    a + \
        "" + \
        b + \
        c
