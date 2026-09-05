#include "format/impl/format_compact_layout.h"

#include <vector>

#include "format/impl/format_break_model_inline_helpers.h"

struct FormatCompactLayout::Impl {
    struct CachedLine : FormatCompactLine {
        bool computed = false;
    };

    mutable std::vector<CachedLine> compactLineShapes_;

    explicit Impl(const FormatBreakModel& model) :
        compactLineShapes_(model.nodes == nullptr ? 1 : model.nodes->size() + 1) {}

    static bool CompactTokenTextIsValid(const FormatBreakToken& token, std::string_view text) {
        if (token.contextOnly) {
            return true;
        }
        const PrintToken& printToken = FormatBreakTokenValue(token);
        return printToken.kind != PrintTokenKind::BlankLine &&
            !IsCommentToken(printToken.kind) &&
            text.find_first_of("\r\n") == std::string_view::npos;
    }

    static void AppendCompactTokenShape(FormatCompactLine& result, const FormatBreakToken& token, std::string_view text)
    {
        result.hasContextOnlyTokens = result.hasContextOnlyTokens || token.contextOnly;
        if (!result.valid || token.contextOnly) {
            return;
        }
        if (!CompactTokenTextIsValid(token, text)) {
            result.valid = false;
            return;
        }
        const int width = Utf8CharacterCount(text);
        const int spaceWithoutLeadingText = result.producesText && token.spaceBefore ? 1 : 0;
        const int spaceWithLeadingText = token.spaceBefore ? 1 : 0;
        result.widthWithoutLeadingText += spaceWithoutLeadingText + width;
        result.widthWithLeadingText += spaceWithLeadingText + width;
        result.producesText = result.producesText || width > 0;
    }

    void AppendCompactNodeShape(FormatCompactLine& result, const FormatBreakNode* child) const {
        if (!result.valid || child == nullptr) {
            return;
        }
        const FormatCompactLine& childShape = BuildCompactLineShape(*child);
        if (!childShape.valid) {
            result.valid = false;
            return;
        }
        result.widthWithoutLeadingText +=
            result.producesText ? childShape.widthWithLeadingText : childShape.widthWithoutLeadingText;
        result.widthWithLeadingText += childShape.widthWithLeadingText;
        result.producesText = result.producesText || childShape.producesText;
        result.hasContextOnlyTokens = result.hasContextOnlyTokens || childShape.hasContextOnlyTokens;
    }

    void AppendCompactListShape(FormatCompactLine& result, const FormatBreakNode& node) const {
        for (size_t index = 0; result.valid && index < node.items.size(); ++index) {
            const FormatBreakListItem& item = node.items[index];
            AppendCompactNodeShape(result, item.node);
            if (FormatBreakTokenKind(item.separator) == PrintTokenKind::Known) {
                AppendCompactTokenShape(result, item.separator, FormatTokenText(FormatBreakTokenValue(item.separator)));
            }
            if (FormatBreakHasTrailingComment(node, index)) {
                result.valid = false;
            }
        }
    }

    const FormatCompactLine& BuildCompactLineShape(const FormatBreakNode& node) const {
        CachedLine& cached = compactLineShapes_[static_cast<size_t>(node.id)];
        if (cached.computed) {
            return cached;
        }
        // Break nodes and token spacing are immutable during a solve. This summary performs the same legality and
        // width calculation as recursively appending the compact form, with line-start text as its only input.
        cached.computed = true;
        cached.valid = !node.forceSplit && !node.ternaryRequiresQuestionBreak && !node.ternaryRequiresColonBreaks;
        if (!cached.valid) {
            return cached;
        }
        switch (node.kind) {
            case FormatBreakNodeKind::Token:
                AppendCompactTokenShape(cached, node.token, FormatTokenText(FormatBreakTokenValue(node.token)));
                break;
            case FormatBreakNodeKind::Sequence:
            case FormatBreakNodeKind::FunctionSignature:
                for (const FormatBreakNode* child : node.children) {
                    AppendCompactNodeShape(cached, child);
                }
                break;
            case FormatBreakNodeKind::BodyHeader:
                if (node.bodyHeaderRequiresDetachedBody) {
                    cached.valid = false;
                    break;
                }
                for (const FormatBreakNode* child : node.children) {
                    AppendCompactNodeShape(cached, child);
                }
                break;
            case FormatBreakNodeKind::Delimited:
                if (node.children.size() < 2) {
                    cached.valid = false;
                    break;
                }
                AppendCompactNodeShape(cached, node.children[0]);
                if (FormatBreakHasLeadingTrailingComment(node)) {
                    cached.valid = false;
                    break;
                }
                AppendCompactListShape(cached, node);
                AppendCompactNodeShape(cached, node.children[1]);
                break;
            case FormatBreakNodeKind::PrefixList:
                if (node.children.empty()) {
                    cached.valid = false;
                    break;
                }
                AppendCompactNodeShape(cached, node.children[0]);
                if (FormatBreakHasLeadingTrailingComment(node)) {
                    cached.valid = false;
                    break;
                }
                AppendCompactListShape(cached, node);
                break;
            case FormatBreakNodeKind::StatementSequence:
                AppendCompactListShape(cached, node);
                break;
            case FormatBreakNodeKind::Chain:
                if (node.chainStartsWithOperator) {
                    cached.valid = false;
                    break;
                }
                for (size_t index = 0; cached.valid && index < node.operands.size(); ++index) {
                    AppendCompactNodeShape(cached, node.operands[index]);
                    if (index < node.commentsBeforeOperators.size() && !node.commentsBeforeOperators[index].empty()) {
                        cached.valid = false;
                        break;
                    }
                    if (index < node.operators.size()) {
                        AppendCompactTokenShape(
                            cached, node.operators[index], FormatTokenText(FormatBreakTokenValue(node.operators[index]))
                        );
                    }
                }
                break;
            case FormatBreakNodeKind::AdjacentStrings:
                if (node.compactStringTexts.size() != node.operands.size()) {
                    cached.valid = false;
                    break;
                }
                for (size_t index = 0; cached.valid && index < node.operands.size(); ++index) {
                    if (node.compactStringTexts[index].empty()) {
                        continue;
                    }
                    const FormatBreakNode* operand = node.operands[index];
                    if (operand == nullptr || operand->kind != FormatBreakNodeKind::Token) {
                        cached.valid = false;
                        break;
                    }
                    AppendCompactTokenShape(cached, operand->token, node.compactStringTexts[index]);
                }
                break;
        }
        return cached;
    }

};

FormatCompactLayout::FormatCompactLayout(const FormatBreakModel& model) : impl_(std::make_unique<Impl>(model)) {}
FormatCompactLayout::~FormatCompactLayout() = default;

const FormatCompactLine& FormatCompactLayout::Measure(const FormatBreakNode& node) const {
    return impl_->BuildCompactLineShape(node);
}

FormatCompactLine FormatCompactLayout::MeasureToken(const FormatBreakToken& token, std::string_view text) {
    FormatCompactLine result{.valid = true};
    Impl::AppendCompactTokenShape(result, token, text);
    return result;
}
