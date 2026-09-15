#include "format/impl/format_layout_projection.h"

#include <algorithm>
#include <array>
#include "format/impl/format_syntax_map.h"

#include "format/impl/format_break_model_inline_helpers.h"
#include "format/impl/format_chain_continuation.h"

namespace {

class Projection {
public:
    Projection(
        std::span<const PrintToken> tokens, const FormatLayoutRegionContext& context, FormatBreakWorkspace* workspace
    ) : context_(context), selected_(workspace) {
        model_.nodes = std::make_unique<std::deque<FormatBreakNode>>();
        selected_.Reserve(tokens.size());
        for (const auto& token : tokens) {
            selected_.InsertOrAssign(token.node, FormatBreakToken{&token});
        }
    }

    FormatBreakModel Build(const FormatBreakModel& complete) {
        root_ = complete.root;
        intersections_.resize(complete.nodes == nullptr ? 1 : complete.nodes->size() + 1, 0);
        model_.root = complete.root == nullptr ? nullptr : Project(*complete.root);
        if (model_.root == nullptr) {
            model_.root = New();
        }
        ApplyChainContinuation(*model_.root);
        return std::move(model_);
    }

private:
    const FormatLayoutRegionContext& context_;
    FormatSyntaxMap<FormatBreakToken> selected_;
    FormatBreakModel model_;
    const FormatBreakNode* root_ = nullptr;
    mutable std::vector<std::uint8_t> intersections_;

    bool RequiresChainBreak(const FormatBreakToken& token) const {
        const auto layout = context_.chainPlacements == nullptr ? std::nullopt :
            context_.chainPlacements->Lookup(FormatBreakTokenValue(token).node);
        return layout && layout->requiredBreak;
    }

    void ApplyChainContinuation(FormatBreakNode& node) {
        if (
            context_.leadingSeparator &&
            node.kind == FormatBreakNodeKind::Chain &&
            node.operators.size() == 1 &&
            node.operators.front().token == nullptr &&
            !node.operands.empty()
        ) {
            const FormatBreakToken* prefix = FormatBreakNodeToken(node.operands.front());
            if (prefix != nullptr && FormatBreakTokenValue(*prefix).node == context_.leadingSeparator->token) {
                node.kind = FormatBreakNodeKind::Sequence;
                node.children = node.operands;
                node.operands = {};
                node.operators = {};
            }
        }
        if (node.kind == FormatBreakNodeKind::Chain) {
            if (std::any_of(node.operators.begin(), node.operators.end(), [this](const FormatBreakToken& token) {
                return RequiresChainBreak(token);
            })) {
                if (node.chainKind == FormatBreakChainKind::Ternary) {
                    node.ternaryRequiresColonBreaks = true;
                } else {
                    node.forceSplit = true;
                }
            }
            if (context_.chainPlacements != nullptr) {
                for (const FormatBreakToken& token : node.operators) {
                    const SyntaxNode* operatorNode = FormatBreakTokenValue(token).node;
                    const auto layout = context_.chainPlacements->Lookup(operatorNode);
                    if (layout) {
                        node.requiredChainBreakBaseIndent = layout->baseIndent;
                        node.flatSplitIndent = layout->flatSplitIndent;
                        break;
                    }
                }
            }
            for (size_t index = 0; index < node.operators.size(); ++index) {
                FormatBreakToken& op = node.operators[index];
                if (
                    !context_.leadingSeparator ||
                    FormatBreakTokenValue(op).node != context_.leadingSeparator->token ||
                    op.contextOnly
                ) {
                    continue;
                }
                node.requiredChainBreakBaseIndent = context_.leadingSeparator->indent - (node.flatSplitIndent ? 0 : 1);
                if (
                    node.chainKind == FormatBreakChainKind::AfterOperator ||
                    node.chainKind == FormatBreakChainKind::Ternary
                ) {
                    node.operands[index + 1] =
                        Sequence({TokenNode(op, node.rawDepth + 1), node.operands[index + 1]}, node.rawDepth + 1);
                    op.contextOnly = true;
                }
                if (node.chainKind == FormatBreakChainKind::Ternary && node.operators.size() == 2) {
                    if (FormatBreakTokenSyntaxKind(op) == SyntaxNodeKind::Question) {
                        node.ternaryRequiresQuestionBreak = true;
                    } else {
                        node.ternaryRequiresColonBreaks = true;
                    }
                } else {
                    node.forceSplit = true;
                }
            }
        }
        for (FormatBreakNode* child : node.children) {
            if (child != nullptr) {
                ApplyChainContinuation(*child);
            }
        }
        for (FormatBreakListItem& item : node.items) {
            if (item.node != nullptr) {
                ApplyChainContinuation(*item.node);
            }
        }
        for (FormatBreakNode* operand : node.operands) {
            if (operand != nullptr) {
                ApplyChainContinuation(*operand);
            }
        }
    }

    FormatBreakNode* New() {
        auto& node = model_.nodes->emplace_back();
        node.id = static_cast<int>(model_.nodes->size());
        return &node;
    }

    FormatBreakNode* Copy(const FormatBreakNode& source) {
        auto* node = New();
        const int id = node->id;
        static_cast<FormatBreakNodeData&>(*node) = source;
        node->id = id;
        node->origin = &source;
        node->compactStringTexts = source.compactStringTexts;
        return node;
    }

    FormatBreakToken Token(const FormatBreakToken& token) const {
        if (token.token == nullptr) {
            return {};
        }
        const auto* found = selected_.Find(token.token->node);
        return found == nullptr ? FormatBreakToken{} : FormatBreakToken{
            found->token,
            token.spaceBefore != token.token->spaceBefore || !found->token->spaceBeforeKnown ? token.spaceBefore :
                found->token->spaceBefore,
            token.contextOnly,
        };
    }

    std::span<FormatBreakNode*> Store(const std::vector<FormatBreakNode*>& nodes) {
        return model_.nodePointers.Append(nodes);
    }

    FormatBreakNode* TokenNode(const FormatBreakToken& token, int depth) {
        if (token.token == nullptr) {
            return nullptr;
        }
        auto* node = New();
        node->kind = FormatBreakNodeKind::Token;
        node->token = token;
        node->syntaxOwner = token.token->node;
        node->rawDepth = node->structuralDepth = node->breakCost = depth;
        return node;
    }

    static void Shift(FormatBreakNode& node, int amount) {
        node.rawDepth -= amount;
        node.structuralDepth = std::max(0, node.structuralDepth - amount);
        node.breakCost = std::max(0, node.breakCost - amount);
        for (auto* child : node.children) {
            Shift(*child, amount);
        }
        for (auto* operand : node.operands) {
            Shift(*operand, amount);
        }
        for (auto& item : node.items) {
            if (item.node != nullptr) {
                Shift(*item.node, amount);
            }
        }
    }

    FormatBreakNode* Sequence(std::vector<FormatBreakNode*> children, int depth) {
        if (children.empty()) {
            return nullptr;
        }
        if (children.size() == 1) {
            return children.front();
        }
        auto* result = New();
        result->rawDepth = result->structuralDepth = result->breakCost = depth;
        result->children = Store(children);
        result->syntaxOwner = children.front()->syntaxOwner;
        return result;
    }

    bool Intersects(const FormatBreakNode& node) const {
        auto& cached = intersections_[static_cast<size_t>(node.id)];
        if (cached == 0) {
            cached = ComputeIntersection(node) ? 2 : 1;
        }
        return cached == 2;
    }

    bool ComputeIntersection(const FormatBreakNode& node) const {
        const auto selected = [&](const FormatBreakToken& token) {
            return token.token != nullptr && selected_.Contains(token.token->node);
        };
        if (selected(node.token) || selected(node.leadingTrailingComment) || selected(node.sourceTrailingComma)) {
            return true;
        }
        for (const auto& op : node.operators) {
            if (selected(op)) {
                return true;
            }
        }
        for (const auto& comments : node.commentsBeforeOperators) {
            for (const auto& comment : comments) {
                if (selected(comment)) {
                    return true;
                }
            }
        }
        for (const auto* child : node.children) {
            if (child != nullptr && Intersects(*child)) {
                return true;
            }
        }
        for (const auto* operand : node.operands) {
            if (operand != nullptr && Intersects(*operand)) {
                return true;
            }
        }
        for (const auto& item : node.items) {
            if (
                selected(item.separator) ||
                selected(item.trailingComment) ||
                (item.node != nullptr && Intersects(*item.node))
            ) {
                return true;
            }
        }
        return false;
    }

    FormatBreakNode* Project(const FormatBreakNode& source) {
        if (!Intersects(source)) {
            return nullptr;
        }
        if (source.kind == FormatBreakNodeKind::Token) {
            const auto token = Token(source.token);
            if (token.token == nullptr) {
                return nullptr;
            }
            auto* node = Copy(source);
            node->token = token;
            return node;
        }
        if (
            source.kind == FormatBreakNodeKind::Delimited ||
            source.kind == FormatBreakNodeKind::PrefixList ||
            source.kind == FormatBreakNodeKind::StatementSequence
        ) {
            return List(source);
        }
        if (source.kind == FormatBreakNodeKind::Chain) {
            return Chain(source);
        }
        auto* node = Copy(source);
        std::vector<FormatBreakNode*> children;
        for (const auto* child : source.children) {
            if (auto* projected = Project(*child)) {
                children.push_back(projected);
            }
        }
        for (const auto* operand : source.operands) {
            if (auto* projected = Project(*operand)) {
                children.push_back(projected);
            }
        }
        if (children.empty()) {
            return nullptr;
        }
        if (source.bodySyntax != nullptr && source.bodySyntax == context_.continuedBodyHeader) {
            node->continuedBodyHeaderOwnerIndent = context_.continuedBodyHeaderOwnerIndent;
        }
        if (source.kind == FormatBreakNodeKind::AdjacentStrings) {
            node->operands = Store(children);
            if (children.size() != source.operands.size()) {
                node->compactStringTexts.clear();
            }
        } else {
            node->children = Store(children);
            if (children.size() != source.children.size() && (
                source.kind == FormatBreakNodeKind::BodyHeader || source.kind == FormatBreakNodeKind::FunctionSignature
            )) {
                for (auto* child : children) {
                    Shift(*child, 1);
                }
                return Sequence(children, source.rawDepth);
            }
        }
        model_.hasLayoutChoice |= source.kind != FormatBreakNodeKind::Sequence;
        return node;
    }

    FormatBreakNode* List(const FormatBreakNode& source) {
        auto* node = Copy(source);
        node->items.reserve(source.items.size());
        std::vector<FormatBreakNode*> delimiters;
        bool openSelected = false;
        bool closeSelected = false;
        for (size_t index = 0; index < source.children.size(); ++index) {
            auto* child = Project(*source.children[index]);
            if (index == 0) {
                openSelected = child != nullptr;
            }
            if (index == 1) {
                closeSelected = child != nullptr;
            }
            if (child == nullptr && index == 1 && openSelected) {
                const auto* open = FormatBreakNodeToken(source.children.front());
                for (const auto& boundary : context_.listBoundaries) {
                    if (open != nullptr && open->token != nullptr && boundary.owner == open->token->node->parent) {
                        child = Copy(*source.children[index]);
                        child->token.contextOnly = true;
                        node->forceSplit |= boundary.forceSplit;
                        closeSelected = true;
                    }
                }
            }
            delimiters.push_back(child);
        }
        for (size_t index = 0; index < source.items.size(); ++index) {
            const auto& item = source.items[index];
            auto* value = item.node == nullptr ? nullptr : Project(*item.node);
            auto separator = Token(item.separator);
            if (source.splitTrailingCommaItem == index && (!closeSelected || !openSelected)) {
                separator = Token(source.sourceTrailingComma);
            }
            auto comment = Token(item.trailingComment);
            if (value == nullptr && separator.token == nullptr && comment.token == nullptr) {
                continue;
            }
            auto projected = item;
            projected.node = value == nullptr ? New() : value;
            projected.separator = separator;
            projected.trailingComment = comment;
            node->items.push_back(projected);
        }
        node->leadingTrailingComment = Token(source.leadingTrailingComment);
        const bool completeDelimiters = source.kind == FormatBreakNodeKind::StatementSequence ||
            (openSelected && (source.kind == FormatBreakNodeKind::PrefixList || closeSelected));
        if (completeDelimiters) {
            node->children = Store(delimiters);
            if (
                node->items.size() != source.items.size() ||
                !closeSelected ||
                (delimiters.size() > 1 && delimiters[1]->token.contextOnly)
            ) {
                node->splitTrailingCommaItem.reset();
                node->blankLineBeforeClose = false;
            }
            model_.hasLayoutChoice = true;
            return node;
        }
        std::vector<FormatBreakNode*> children;
        if (openSelected) {
            children.push_back(delimiters.front());
        }
        if (auto* comment = TokenNode(node->leadingTrailingComment, source.rawDepth + 1)) {
            children.push_back(comment);
        }
        for (const auto& item : node->items) {
            children.push_back(item.node);
            auto* separator = TokenNode(item.separator, source.rawDepth + 1);
            auto* comment = TokenNode(item.trailingComment, source.rawDepth + 1);
            if (separator != nullptr) {
                children.push_back(separator);
            }
            if (comment != nullptr) {
                children.push_back(comment);
            }
        }
        if (closeSelected) {
            children.push_back(delimiters.back());
        }
        for (auto* child : children) {
            Shift(*child, 1);
        }
        return Sequence(children, source.rawDepth);
    }

    bool Complete(const FormatBreakNode& node) const {
        if (node.hasIndependentBodyItems) {
            return false;
        }
        if (node.kind == FormatBreakNodeKind::Token) {
            return node.token.token == nullptr ||
                node.token.token->node == nullptr ||
                node.token.token->node->text.empty() ||
                selected_.Contains(node.token.token->node);
        }
        for (const auto* child : node.children) {
            if (!Complete(*child)) {
                return false;
            }
        }
        for (const auto* operand : node.operands) {
            if (!Complete(*operand)) {
                return false;
            }
        }
        for (const auto& item : node.items) {
            if (item.node != nullptr && !Complete(*item.node)) {
                return false;
            }
            if (item.separator.token != nullptr && !selected_.Contains(item.separator.token->node)) {
                return false;
            }
            if (item.trailingComment.token != nullptr && !selected_.Contains(item.trailingComment.token->node)) {
                return false;
            }
        }
        return true;
    }

    static bool HasCode(const FormatBreakNode& node) {
        if (node.kind == FormatBreakNodeKind::Token) {
            return !node.token.contextOnly &&
                node.token.token != nullptr &&
                !IsCommentToken(node.token.token->kind) &&
                node.token.token->kind != PrintTokenKind::BlankLine;
        }
        for (const auto* child : node.children) {
            if (HasCode(*child)) {
                return true;
            }
        }
        for (const auto* operand : node.operands) {
            if (HasCode(*operand)) {
                return true;
            }
        }
        return !node.items.empty();
    }

    void ExtractLeadingTrivia(
        FormatBreakNode& node, const PrintToken& op, bool before, std::vector<FormatBreakNode*>& moved
    ) {
        if (node.kind == FormatBreakNodeKind::Token && node.token.token != nullptr && !node.token.contextOnly) {
            const auto& token = *node.token.token;
            if (token.node == op.node) {
                node.token.contextOnly = true;
            } else if (IsCommentToken(token.kind) && (token.sourceIndex < op.sourceIndex) == before) {
                moved.push_back(TokenNode(node.token, node.rawDepth));
                node.token.contextOnly = true;
            }
        }
        for (auto* child : node.children) {
            ExtractLeadingTrivia(*child, op, before, moved);
        }
    }

    FormatBreakNode* Chain(const FormatBreakNode& source) {
        auto* node = Copy(source);
        std::vector<FormatBreakNode*> operands;
        std::vector<FormatBreakToken> operators;
        for (size_t index = 0; index < source.operands.size(); ++index) {
            auto* operand = Project(*source.operands[index]);
            auto op = index < source.operators.size() ? Token(source.operators[index]) : FormatBreakToken{};
            const bool implicitOperator = index < source.operators.size() && source.operators[index].token == nullptr;
            if (operand == nullptr && op.token == nullptr && !implicitOperator && operators.empty()) {
                continue;
            }
            operands.push_back(operand == nullptr ? New() : operand);
            if (index < source.operators.size()) {
                operators.push_back(op);
                std::vector<FormatBreakToken> comments;
                if (index < source.commentsBeforeOperators.size()) {
                    for (const auto& comment : source.commentsBeforeOperators[index]) {
                        if (auto selected = Token(comment); selected.token != nullptr) {
                            comments.push_back(selected);
                        }
                    }
                }
                if (!comments.empty()) {
                    // Missing suffix entries and empty comment vectors are equivalent.
                    node->commentsBeforeOperators.resize(operators.size());
                    node->commentsBeforeOperators.back() = std::move(comments);
                }
            }
        }
        if (operands.empty()) {
            return nullptr;
        }
        while (
            !operators.empty() &&
            operators.back().token == nullptr &&
            operands.back()->kind == FormatBreakNodeKind::Sequence &&
            operands.back()->children.empty()
        ) {
            operands.pop_back();
            operators.pop_back();
        }
        while (!operators.empty() && operands.size() > 1 && !HasCode(*operands.back())) {
            const auto op = operators.back();
            auto* tail = operands.back();
            operands.pop_back();
            operators.pop_back();
            std::vector<FormatBreakNode*> suffix{operands.back()};
            if (auto* token = TokenNode(op, source.rawDepth + 1)) {
                suffix.push_back(token);
            }
            suffix.push_back(tail);
            operands.back() = Sequence(suffix, source.rawDepth + 1);
        }
        if (context_.leadingSeparator && source.chainKind == FormatBreakChainKind::AfterOperator) {
            for (size_t index = 0; index < operators.size(); ++index) {
                auto& op = operators[index];
                if (op.token == nullptr || op.token->node != context_.leadingSeparator->token) {
                    continue;
                }
                std::vector<FormatBreakNode*> before, after;
                ExtractLeadingTrivia(*operands[index], *op.token, false, after);
                ExtractLeadingTrivia(*operands[index + 1], *op.token, true, before);
                before.insert(before.begin(), operands[index]);
                operands[index] = Sequence(before, source.rawDepth + 1);
                after.push_back(operands[index + 1]);
                operands[index + 1] = Sequence(after, source.rawDepth + 1);
                op.contextOnly = false;
            }
        }
        if (operators.empty() && source.chainKind != FormatBreakChainKind::CallApplication) {
            for (auto* operand : operands) {
                Shift(*operand, 1);
            }
            return Sequence(operands, source.rawDepth);
        }
        for (size_t index = 1; index < operands.size(); ++index) {
            auto* child = operands[index];
            while (child->kind == FormatBreakNodeKind::Sequence && child->children.size() == 1) {
                child = child->children.front();
            }
            if (
                child->kind != FormatBreakNodeKind::Chain ||
                child->chainKind != source.chainKind ||
                child->chainStartsWithOperator ||
                index > operators.size() ||
                child->operators.empty() ||
                source.chainKind != FormatBreakChainKind::StreamBeforeOperator
            ) {
                continue;
            }
            const auto kind = FormatBreakTokenSyntaxKind(operators[index - 1]);
            if (!std::all_of(child->operators.begin(), child->operators.end(), [&](const auto& op) {
                return FormatBreakTokenSyntaxKind(op) == kind;
            })) {
                continue;
            }
            operands[index] = child->operands.front();
            operands.insert(operands.begin() + index + 1, child->operands.begin() + 1, child->operands.end());
            operators.insert(operators.begin() + index, child->operators.begin(), child->operators.end());
            if (node->commentsBeforeOperators.size() < operators.size() - child->operators.size()) {
                node->commentsBeforeOperators.resize(operators.size() - child->operators.size());
            }
            std::vector<std::vector<FormatBreakToken>> comments(child->operators.size());
            for (size_t j = 0; j < std::min(comments.size(), child->commentsBeforeOperators.size()); ++j) {
                comments[j] = child->commentsBeforeOperators[j];
            }
            node
                ->commentsBeforeOperators
                .insert(node->commentsBeforeOperators.begin() + index, comments.begin(), comments.end());
            node->forceSplit |= child->forceSplit;
            for (auto* operand : child->operands) {
                Shift(*operand, child->rawDepth - source.rawDepth);
            }
        }
        if (source.chainKind == FormatBreakChainKind::StreamBeforeOperator) {
            node->chainStartsWithOperator = !HasCode(*operands.front());
            if (node->chainStartsWithOperator) {
                // A mandatory boundary may consume the receiver. Its remaining trivia precedes the first operator.
                std::vector<FormatBreakNode*> comments;
                const auto* first = operators.empty() ? nullptr : operators.front().token;
                if (first != nullptr) {
                    ExtractLeadingTrivia(*operands.front(), *first, true, comments);
                    ExtractLeadingTrivia(*operands.front(), *first, false, comments);
                    if (node->commentsBeforeOperators.empty()) {
                        node->commentsBeforeOperators.resize(operators.size());
                    }
                    for (const auto* comment : comments) {
                        node->commentsBeforeOperators.front().push_back(comment->token);
                    }
                    std::stable_sort(
                        node->commentsBeforeOperators.front().begin(),
                        node->commentsBeforeOperators.front().end(),
                        [](const auto& left, const auto& right) {
                            return left.token->sourceIndex < right.token->sourceIndex;
                        }
                    );
                }
            }
            node->forceSplit |= context_.forceSplitStreamChain && !node->chainStartsWithOperator && &source == root_;
        }
        if (source.chainCompactRequiresFitOnOneLine && !Complete(source)) {
            node->forceSplit = true;
        }
        node->operands = Store(operands);
        node->operators = model_.tokens.Append(operators);
        model_.hasLayoutChoice = true;
        return node;
    }
};

}  // namespace

FormatBreakModel ProjectFormatLayout(
    const FormatBreakModel& complete,
    std::span<const PrintToken> tokens,
    const FormatLayoutRegionContext& context,
    FormatBreakWorkspace* workspace
) {
    return Projection(tokens, context, workspace).Build(complete);
}
