#define FORMAT_EOF_VALUE 1
#define FORMAT_EOF_PAIR \
 (sizeof("prefix") - 1, sizeof(">") - 1)
namespace DirectiveEof { int Read(){return FORMAT_EOF_VALUE;} }
#define FORMAT_EOF_BODY() \
 do {} while(false) \
