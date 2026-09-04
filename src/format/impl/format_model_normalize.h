#pragma once

#include "format/impl/format_model.h"

// Normalizes formatter-owned syntax and materializes local semantic facts after
// a node's children have been built and normalized. May allocate or reparent nodes
// in model; preserves parser-selected syntax categories and source ownership.
void NormalizeSyntaxNode(FormatModel& model, SyntaxNode& node);
