#pragma once

class FormatLayoutTree;
class FormatLayoutWriter;
struct FormatterConfig;
struct FormatBreakModel;
struct FormatBreakSolution;

// Lowers one selected cost region into the persistent output program and records
// placements on its owners. Physical emission consumes only the completed program.
void LowerFormatLayout(
    const FormatterConfig& config,
    const FormatBreakModel& model,
    const FormatBreakSolution& solution,
    int baseIndent,
    FormatLayoutTree& tree,
    FormatLayoutWriter& output
);
