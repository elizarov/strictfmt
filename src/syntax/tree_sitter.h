#pragma once

#include <cstdint>
#include <string_view>
#include <tree_sitter/api.h>

// Source-backed views and field traversal shared by syntax consumers. No layout policy.
inline std::string_view NodeText(TSNode node, std::string_view source) {
    if (ts_node_is_null(node)) {
        return {};
    }
    const uint32_t start = ts_node_start_byte(node);
    const uint32_t end = ts_node_end_byte(node);
    return start <= end && end <= source.size() ? source.substr(start, end - start) : std::string_view{};
}
inline std::string_view NodeType(TSNode node) {
    return ts_node_is_null(node) ? std::string_view{} : ts_node_type(node);
}
inline TSNode NodeField(TSNode node, std::string_view field) {
    if (ts_node_is_null(node)) {
        return {};
    }
    return ts_node_child_by_field_name(node, field.data(), static_cast<uint32_t>(field.size()));
}
template <typename Visitor>
void VisitChildren(TSNode node, Visitor&& visit) {
    if (ts_node_is_null(node)) {
        return;
    }
    TSTreeCursor cursor = ts_tree_cursor_new(node);
    if (ts_tree_cursor_goto_first_child(&cursor)) {
        do {
            const char* field = ts_tree_cursor_current_field_name(&cursor);
            visit(
                ts_tree_cursor_current_node(&cursor), field == nullptr ? std::string_view{} : std::string_view(field)
            );
        } while (ts_tree_cursor_goto_next_sibling(&cursor));
    }
    ts_tree_cursor_delete(&cursor);
}
