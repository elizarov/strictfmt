#pragma once

#include <vector>

#include "format/impl/format_print_token.h"

// Projects a normalized syntax model into source-ordered tokens, materializing
// ancestry, comment continuation, and adjacent-source spacing facts once. Tokens
// borrow nodes and text from model, which must outlive them and remain structurally unchanged.
// Inherited traversal context and token initialization remain private to this module.
std::vector<PrintToken> BuildPrintTokens(const FormatModel& model, int tabWidth);
