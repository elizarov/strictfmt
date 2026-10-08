#pragma once

#include "format/impl/format_break_solution.h"
#include "format/impl/format_config.h"

// Selects an exact layout for one immutable segment at the supplied incoming
// column/indentation, including physical suffixes on taken breaks and the final line. Returns owned
// choices and render bases; candidate state and caches live only for this call.
// Disabling the cost bound retains the unbounded reference search.
FormatBreakSolution SolveFormatBreaks(
    const FormatterConfig& config,
    const FormatBreakModel& model,
    int startColumn,
    int indentLevel,
    int indentWidth,
    int breakLineSuffixWidth,
    int finalLineSuffixWidth = 0,
    bool useCostBound = true
);
