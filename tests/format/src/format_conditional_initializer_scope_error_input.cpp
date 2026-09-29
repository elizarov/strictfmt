int ConditionalInitializerScope(bool active) {
int value=0;
if(active)
value=
#if FIRST
1;
value+=10;
#else
2;
#endif
return value;
}
