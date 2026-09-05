#pragma once

#include <span>

#include "format/impl/format_break_model.h"

// Builds the selected syntax segment and fixes its break costs before returning.
// The result owns break nodes but borrows tokens and syntax, which must outlive it.
// Construction updates syntax scratch marks; it does not change syntax structure.
FormatBreakModel BuildFormatBreakModel(std::span<const PrintToken> tokens);
FormatBreakModel BuildFormatBreakModel(std::span<const PrintToken> tokens, const FormatBreakModelContext& context);
