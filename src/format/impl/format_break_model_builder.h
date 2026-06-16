#pragma once

#include <span>

#include "format/impl/format_break_model.h"

FormatBreakModel BuildFormatBreakModel(std::span<const PrintToken> tokens);
FormatBreakModel BuildFormatBreakModel(std::span<const PrintToken> tokens, const FormatBreakModelContext& context);
