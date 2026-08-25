#pragma once

#include <vector>

#include "format/impl/format_break_model.h"
#include "format/impl/format_config.h"

struct FormatBreakSolution {
    std::vector<FormatBreakChoice> choices;
    // Selected delimiter choices record their render base; other entries remain -1.
    std::vector<int> indentLevels;
    // Declaration owner/value nodes record the selected number of continuation lines in their value.
    std::vector<int> declarationValueContinuationLines;
};

FormatBreakSolution SolveFormatBreaks(
    const FormatterConfig& config,
    const FormatBreakModel& model,
    int startColumn,
    int indentLevel,
    int indentWidth,
    int breakLineSuffixWidth
);
