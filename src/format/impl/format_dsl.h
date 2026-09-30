#pragma once

#include "format/impl/format_break_model.h"
#include "format/impl/format_config.h"

// Classify complete member-call chains before projection or break-cost normalization.
void ConfigureBuilderChains(FormatBreakModel& model, const FormatterConfig& config);
