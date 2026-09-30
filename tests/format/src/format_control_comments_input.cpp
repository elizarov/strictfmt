// clang-format on
#include "z.h" // clang-format off
// clang-format off: preserve include order
#include <vector> /* clang-format on */
/* clang-format off */
#include <algorithm> // clang-format on: done
#include "a.h"
#include <path/* clang-format off */header> // clang-format on
// Ordinary include boundary.
#include "second_z.h" // clang-format off:
#include "second_a.h" /* clang-format off */

#ifndef CONTROL_COMMENTS_H // clang-format off
#define CONTROL_COMMENTS_H
// clang-format off
#include "guard_z.h"
/* clang-format on */
#include "guard_a.h" // clang-format on
#endif // clang-format on

// clang-format off
void RemovedMarkers(){
// clang-format off
int/* clang-format off */value=1; // clang-format on: reason
/* clang-format on */
if(value) /* clang-format off */ Run();
Call(/* clang-format off */first,second/* clang-format on */);
return; // clang-format off
}
// clang-format on

int ConditionalMarkers() {
#if FEATURE // clang-format off
return 1; /* clang-format on */
#else /* clang-format off */
return 2; // clang-format on
#endif // clang-format on: end
}

#define STRUCTURED_MARKER(x) ((x) /* clang-format off */ + 1)
#define RAW_MARKER(name) prefix##name /* clang-format off */ + + suffix
#define RAW_SEPARATION(name) name/* clang-format on */other
#define RAW_CONTINUED(name) prefix##name /* clang-format off */ \
    + suffix /* clang-format on */
#define RAW_LINE_MARKER(name) prefix##name // clang-format off: reason
#pragma marker /* clang-format off */ value // clang-format on
#undef STRUCTURED_MARKER // clang-format off

const char* ordinary = "// clang-format off";
const char* block = "/* clang-format on */";
const char* raw = R"tag("
// clang-format off
/* clang-format on */
)tag";
#define RAW_FRAGMENT(name) ) name/* clang-format off */other
#define RAW_FRAGMENT_LINES(name) \
    ) /* clang-format off */ \
    name /* clang-format on */
#define RAW_FRAGMENT_COMMENT(name) \
    ) // clang-format off: explanation \
    still part of the comment
#define RAW_NUMBERS(name) ) 1'000 /* clang-format off */ name
#define RAW_STRINGS(name) name##suffix "/* clang-format off */" R"(// clang-format on)"

//clang-format off
//  clang-format on
// Clang-format off
// clang-format OFF
// clang-format only
// clang-format offline
// clang-format off because this is ordinary prose
// clang-format on : a space before the colon is not a marker
/*clang-format off*/
/* clang-format on: block comments do not accept explanations */
/* clang-format off  */
/* A quoted marker: // clang-format off */
// An ordinary comment containing /* clang-format on */.
/* clang-format off
*/
int UnmatchedMarkers;
/* clang-format off */
// clang-format off 	