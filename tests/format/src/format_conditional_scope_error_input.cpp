int OuterConditionalClose(bool outer, bool inner) {
int value=0;
if(outer)
if(inner) {
#if FIRST
value=1;
}
value+=10;
#else
value=2;
}
value+=20;
#endif
return value;
}
