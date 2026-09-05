#pragma once

#include "format/impl/format_break_solution.h"
#include "format/impl/format_config.h"

// Selects an exact layout for one immutable segment at the supplied incoming
// column/indentation, including any physical suffix on taken breaks. Returns owned
// choices and render bases; candidate state and caches live only for this call.
FormatBreakSolution SolveFormatBreaks(
    const FormatterConfig& config,
    const FormatBreakModel& model,
    int startColumn,
    int indentLevel,
    int indentWidth,
    int breakLineSuffixWidth
);
