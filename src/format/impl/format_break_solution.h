#pragma once

#include <cstdint>
#include <vector>

#include "format/impl/format_break_model.h"

// Materialized segment layout shared by solving, lowering and diagnostics. Node-indexed vectors retain default entries for decisions
// not explicitly selected; operator identities use original token source indexes.
struct FormatBreakSolution {
    std::vector<FormatBreakChoice> choices;
    // Selected structural choices record the render base used to solve their node.
    std::vector<int> indentLevels;
    // Operators whose adjacent operands form a selected literal-value pair.
    std::vector<std::uint32_t> attachedChainOperators;
    // Lists whose trailing comma was removed before selecting the final layout.
    std::vector<int> omittedTrailingCommaNodes;
};
