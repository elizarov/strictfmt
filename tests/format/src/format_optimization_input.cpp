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

// comment spacing at the column limit
void f8(){f1(/**/"");}

// operator and comment suffix width
void f9(){a //
+b //
+c;}

// comment moved before a stream operator
void f10(){Get(1,2) << /* x */
a << b;}

// literal pair fits with the terminating semicolon
void f11(){aa+""+b;}

// enclosing semicolon prevents a pair
void f12(){a+""+bb;}

// trailing chain operator prevents a pair
void f13(){a+""+b+c;}

// closing parentheses prevent a pair
void f14(){F(a+""+b);}

// pairs stay independent across the chain
void f15(){a+""+b+""+c;}

// an expanded operand does not form a pair
void f16(){a+""+F(aa,bb)+c;}

// default values use the assignment break, including non-final and unnamed parameters
void f17(int x=Default);
void f18(T& x=Get(),int y=Default);
void f19(T* =Default);

// defaults with calls, braced values, comments, and nested assignments
void f20(T x=Make(a,b));
void f21(T x={a,b,c});
void f22(int x=/*d*/y);
void f23(int x=F(a=b));

// template defaults share the same list-item assignment handling
template<int n=Default>struct A;
template<class T=A::B>struct B;
template<template<class T=A::B>class C=Box>struct C;

// typedefs share the qualified-type boundary for every declarator shape
typedef a::B C;
typedef a::B Name;
typedef a::B* Ptr;
typedef a::B& Ref;
typedef a::B** PP;
typedef a::B A,B;
typedef a::B A[2];
typedef a::B (*P)[2];
typedef a::B (P);
typedef a::B F();
typedef a::B (*P)();
typedef a::B (C::*M)();
typedef a::B C::*M;
typedef a::B A,B[2],*P;
typedef a::T<B> Name;
typedef a::B F(),G();

// the same boundary applies to array and multiple-object declarations
a::B Object[20];
a::LongType A,B;
void f24(a::LongType a[2]);

// initializer-record siblings: compact, bare, typed, mixed, and empty records
P a={{1},{2}};
P b={{1},{first,second}};
P c={P{1},P{first,second}};
P d={{1},P{first,second}};
P e={P{}, {first,second}};
auto f=Call({1},{first,second});
auto g=Call(P{1},P{first,second});

// split/compact stays separate; split/split keeps its bridge; siblings need not be adjacent
P h={{first,second},{1}};
P i={{first,second},{third,fourth}};
P j={{1},0,{first,second}};

// ordinary trailing payloads and initializers nested inside calls are not sibling records
P k={0,{first,second}};
P l={{1},F(first,second)};
P m={F({1}),{first,second}};
P n={{first,second}};
P o={P{1},P{Q{first,second}}};

// direct declaration assignments do not need an initializer-declarator wrapper
template<class T>concept C=ns::C<T>;
template<class T>concept LongName=ns::C<T>;
template<class T>concept Both=ns::C<T>&&ns::D<T>;
namespace A=ns::inner;
namespace Name=aa::bb::cc;
namespace Global=::aa::bb;

// template arguments expand before their qualified name, including nested prefixes
using A=space::C<int>;
using B=a::b::C<int>;
using C=a::B<int>::C<long>;
using D=a::B<int>::C<long>::D<char>;
using E=::space::C<int>;
using F=space::C<ns::T>;
using G=LongNamespace::LongType<int>;
using H=space::C<>;
using I=typename T::template Rebind<int>::type;
void f25(){space::F<int>(x);}
void f26(){space::F<int>{x};}
void f27(){space::F<int,long>(x,y);}
void f28(){space::C<int> value;}
void f29(){if(space::C<int> value=Get()){Use(value);}}

// callable tail markers remain attached rather than acquiring assignment breaks
struct TailMarkers {TailMarkers()=default;TailMarkers(const TailMarkers&)=delete;virtual void Method()=0;};

// macro continuation suffix prevents a pair
#define JOIN a+""+b+c
