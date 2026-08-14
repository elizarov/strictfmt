#pragma once

#include <vector>

#include "format/impl/format_break_model.h"
#include "format/impl/format_config.h"

struct FormatBreakSolution {
    std::vector<FormatBreakChoice> choices;
};

FormatBreakSolution SolveFormatBreaks(
    const FormatterConfig& config,
    const FormatBreakModel& model,
    int startColumn,
    int indentLevel,
    int indentWidth,
    int breakLineSuffixWidth
);
