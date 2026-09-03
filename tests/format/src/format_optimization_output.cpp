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
