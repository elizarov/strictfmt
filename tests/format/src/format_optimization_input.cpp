// overflow
void f1(){if(a){throw (((((value)))));}}

// delimiter
void f2(){if(a){throw ((value));}}

// signature
Vector<int> func(int a);

// ternary
void f4(){auto x=std::optional<int>{}?F(a,b):Vector<int>{};}

// list item
void f5(){auto x=Call((((value))),b);}

// flat leaf
void f6(){auto x=((a+b));}

// exact runs
void f7(){if(a){if(b){throw ((((((((((((((((((((value))))))))))))))))))));}}}
