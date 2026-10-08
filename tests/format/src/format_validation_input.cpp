// Conditional directive calls.
#if HAS(feature)
int a;
#elif CHECK(major, minor)
int b;
#else
int c;
#endif

// Identical comments in a directive header and body.
#ifdef /* guard */ FEATURE
/* guard */ int a;
#endif

// Leading macro replacement comments.
#define FIELD(data, elem) \
    /* annotation */ \
    decltype(data::elem) elem;

// Terminal macro comments before endif.
#if HAS(feature)
#define FLAG 1 // note
#endif
int n;

// Packed parameter comments.
void SomeQuiteLongFunctionName(LongType a, LongType /*b*/);

// Template header comments.
template <typename T> // element type
class Queue;

// Numbers before pack expansions.
template<int... I> void f() { use({ I ? I : 0 ... }); }

// Macro terminators and standalone comments.
void f(){
    ITEM(one);
    ITEM(two); // tail
    ITEM(three)
    // standalone
    int a;
}

// Partially guarded namespaces.
#if FEATURE
namespace {
int x;
#endif
}

// Conditional ends before a consequence.
void f() {
#if FEATURE
if (a) { g(); } else if (b)
#endif
h();
}

// Conditional ends before else.
void f() {
#if FEATURE
if (a) { g(); }
#endif
else { h(); }
}
