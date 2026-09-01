#include <algorithm>

#include "format/impl/format_break_model_inline_helpers.h"

namespace {

bool IsLeadingNameToken(const FormatBreakToken& token) {
    return FormatBreakTokenKind(token) == PrintTokenKind::Text || (
        FormatBreakTokenKind(token) == PrintTokenKind::Known &&
        FormatBreakTokenSyntaxKind(token) == SyntaxNodeKind::ColonColon
    );
}

bool ConsumeLeadingNameToken(const FormatBreakToken& token, std::string_view candidate, size_t& position) {
    if (!IsLeadingNameToken(token)) {
        return true;
    }
    const std::string_view text = FormatTokenText(FormatBreakTokenValue(token));
    if (position + text.size() > candidate.size() || candidate.substr(position, text.size()) != text) {
        return false;
    }
    position += text.size();
    return true;
}

bool ConsumeLeadingName(const FormatBreakNode& node, std::string_view candidate, size_t& position) {
    switch (node.kind) {
        case FormatBreakNodeKind::Token:
            return ConsumeLeadingNameToken(node.token, candidate, position);
        case FormatBreakNodeKind::Chain:
            if (!IsFormatBreakQualifiedName(node)) {
                return true;
            }
            if (!ConsumeLeadingName(*node.operands.front(), candidate, position)) {
                return false;
            }
            return ConsumeLeadingNameToken(node.operators.front(), candidate, position) &&
                ConsumeLeadingName(*node.operands.back(), candidate, position);
        case FormatBreakNodeKind::Sequence:
        case FormatBreakNodeKind::FunctionSignature:
        case FormatBreakNodeKind::BodyHeader:
            for (const FormatBreakNode* child : node.children) {
                if (child->kind == FormatBreakNodeKind::Delimited) {
                    return true;
                }
                if (!ConsumeLeadingName(*child, candidate, position)) {
                    return false;
                }
            }
            return true;
        default:
            return true;
    }
}

}  // namespace

bool FormatBreakLeadingNameMatches(const FormatBreakNode& node, std::string_view candidate) {
    size_t position = 0;
    return ConsumeLeadingName(node, candidate, position) && position == candidate.size();
}

bool IsFormatBreakUniformChain(const FormatBreakNode& node) {
    if (node.kind != FormatBreakNodeKind::Chain) {
        return false;
    }
    if (node.chainKind != FormatBreakChainKind::AfterOperator) {
        return true;
    }
    return !node.operators.empty() && std::all_of(node.operators.begin(), node.operators.end(), [](const auto& token) {
        return FormatBreakTokenKind(token) == PrintTokenKind::Known &&
            SyntaxNodeKindHasClass(FormatBreakTokenSyntaxKind(token), SyntaxNodeClass::ChainOperator);
    });
}

bool IsFormatBreakQualifiedName(const FormatBreakNode& node) {
    return node.kind == FormatBreakNodeKind::Chain &&
        node.operands.size() == 2 &&
        node.operators.size() == 1 &&
        FormatBreakTokenKind(node.operators.front()) == PrintTokenKind::Known &&
        FormatBreakTokenSyntaxKind(node.operators.front()) == SyntaxNodeKind::ColonColon;
}

bool IsFormatBreakStreamLiteralOperand(const FormatBreakNode& node) {
    if (node.kind == FormatBreakNodeKind::Token) {
        return IsStringLike(FormatBreakTokenValue(node.token));
    }
    if (node.kind == FormatBreakNodeKind::Sequence) {
        return node.children.size() == 1 && IsFormatBreakStreamLiteralOperand(*node.children.front());
    }
    return node.kind == FormatBreakNodeKind::AdjacentStrings &&
        !node.operands.empty() &&
        std::all_of(node.operands.begin(), node.operands.end(), [](const FormatBreakNode* operand) {
            return operand != nullptr && IsFormatBreakStreamLiteralOperand(*operand);
        });
}

bool IsFormatBreakStreamConfigurationOperand(
    const FormatBreakNode& node, const std::vector<std::string>& configurationMethods
) {
    return std::any_of(configurationMethods.begin(), configurationMethods.end(), [&node](const std::string& method) {
        return FormatBreakLeadingNameMatches(node, method);
    });
}
