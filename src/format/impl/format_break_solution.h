#pragma once

#include <cstdint>
#include <vector>

#include "format/impl/format_break_model.h"

// Materialized segment layout shared by solving, emission, diagnostics, and
// declaration analysis. Node-indexed vectors retain default entries for decisions
// not explicitly selected; operator identities use original token source indexes.
struct FormatBreakSolution {
    std::vector<FormatBreakChoice> choices;
    // Selected structural choices record the render base used to solve their node.
    std::vector<int> indentLevels;
    // Declaration owner/value nodes record the selected number of continuation lines in their value.
    std::vector<int> declarationValueContinuationLines;
    // Operators whose adjacent operands form a selected literal-value pair.
    std::vector<std::uint32_t> attachedChainOperators;
};
