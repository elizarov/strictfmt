void ConditionalTryInUnbracedBody(bool active) {
if(active)
#if FIRST
try {
#endif
First();
Second();
#if FIRST
} catch(...) {}
#endif
}
