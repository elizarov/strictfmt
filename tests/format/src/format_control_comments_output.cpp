#include <algorithm>
#include <path with  spaces/header.h>  // trailing include comment
#include <path/"quote//header.h>  // trailing include comment
#include <path/'quote//header.h>  // trailing include comment
#include <path/* clang-format off */header>
#include <path//header.h>  // trailing include comment
#include <vector>

#include "a.h"
#include "path//header.h"  // trailing include comment
#include "z.h"

// Ordinary include boundary.
#include "second_a.h"
#include "second_z.h"

#ifndef CONTROL_COMMENTS_H
#define CONTROL_COMMENTS_H
#include "guard_a.h"
#include "guard_z.h"
#endif

void RemovedMarkers() {
    int value = 1;
    if (value) {
        Run();
    }
    Call(first, second);
    return;
}

int ConditionalMarkers() {
#if FEATURE
    return 1;
#else
    return 2;
#endif
}

#define STRUCTURED_MARKER(x) ((x) + 1)
#define RAW_MARKER(name) prefix##name + +suffix
#define RAW_SEPARATION(name) name other
#define RAW_CONTINUED(name) prefix##name + suffix
#define RAW_LINE_MARKER(name) prefix##name
#pragma marker   value
#undef STRUCTURED_MARKER

const char* ordinary = "// clang-format off";
const char* block = "/* clang-format on */";

const char* raw = R"tag("
// clang-format off
/* clang-format on */
)tag";

#define RAW_FRAGMENT(name) ) name other
#define RAW_FRAGMENT_LINES(name) \
    )                            \
    name
#define RAW_FRAGMENT_COMMENT(name) \
    )
#define RAW_NUMBERS(name) ) 1'000 name
#define RAW_STRINGS(name) name##suffix "/* clang-format off */" R"(// clang-format on)"

#if FEATURE
#include <guard//header.h>  // guarded include comment
#endif

void LocalHeaders() {
#include <local//header.h>  // local include comment
}

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
