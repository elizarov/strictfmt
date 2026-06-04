#pragma once

#include <string_view>

#include "tools/impl/format_model.h"

struct FormatterConfig;

FormatModel ParseFormatModel(std::string_view text, const FormatterConfig& config);
