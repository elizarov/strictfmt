#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include "format/impl/format_break_solution.h"

// Immutable, arena-owned choice histories shared by solver candidates. Handles
// remain valid until this history is destroyed; append/concatenate preserve old
// handles. Lookup uses the last matching record, while materialization keeps the
// first choice/render base and last continuation count per node, and deduplicates
// attached operators. This module owns no candidate costs or layout search.
class FormatChoiceHistory {
public:
    struct Entry;

    using Handle = const Entry*;

    FormatChoiceHistory();
    ~FormatChoiceHistory();
    Handle Concat(Handle left, Handle right);
    Handle AddChoice(Handle history, int nodeId, FormatBreakChoice choice, int indentLevel = -1);
    Handle AddContinuationLines(Handle history, int nodeId, int lines);
    Handle AddAttachedOperator(Handle history, std::uint32_t sourceIndex);
    static std::optional<FormatBreakChoice> Find(Handle history, int nodeId);
    static FormatBreakSolution Materialize(Handle history, size_t choiceCount);

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};
