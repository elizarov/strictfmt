#pragma once

#include "format/impl/format_break_solution.h"
#include "format/impl/format_config.h"

FormatBreakSolution SolveFormatBreaks(
    const FormatterConfig& config,
    const FormatBreakModel& model,
    int startColumn,
    int indentLevel,
    int indentWidth,
    int breakLineSuffixWidth
);
