#pragma once

#include <cstddef>
#include <cstdio>
#include <span>

#include "format/impl/format_break_model.h"
#include "format/impl/format_break_solver.h"

class FormatBreakModelDumpWriter {
public:
    explicit FormatBreakModelDumpWriter(FILE* output) : output_(output) {}

    void WriteSegment(
        std::span<const PrintToken> tokens,
        const FormatBreakModel& model,
        const FormatBreakSolution& solution,
        int startColumn,
        int baseIndent,
        int breakLineSuffixWidth
    );

private:
    FILE* output_ = nullptr;
    size_t segmentCount_ = 0;
};
