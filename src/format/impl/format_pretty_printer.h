#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include "format/impl/format_config.h"
#include "format/impl/format_model.h"
#include "format/impl/format_model_text_stats.h"

std::string FormatModelText(const FormatterConfig& config, const FormatModel& model, std::string_view sourcePath);
std::string FormatModelText(
    const FormatterConfig& config, const FormatModel& model, std::string_view sourcePath, FormatModelTextStats& stats
);
void DumpFormatBreakTrees(
    const FormatterConfig& config, const FormatModel& model, std::string_view sourcePath, FILE* output
);
