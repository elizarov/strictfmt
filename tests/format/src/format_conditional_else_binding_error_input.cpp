int ConditionalElseBinding(bool active, bool choice) {
int result=0;
if(active)
if(choice) {
result=1;
#if FIRST
} else {
result=2;
#endif
}
else result=3;
return result;
}
