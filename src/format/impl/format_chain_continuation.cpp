#include "format/impl/format_chain_continuation.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include "format/impl/format_break_emitter.h"
#include "format/impl/format_break_model_builder.h"
#include "format/impl/format_break_model_inline_helpers.h"

struct FormatChainContinuation::Impl {
    explicit Impl(std::span<const PrintToken> tokens) : tokens_(tokens) {}

    std::span<const PrintToken> tokens_;
    std::unordered_set<const SyntaxNode*> requiredChainBreakOperators_;
    std::unordered_map<const SyntaxNode*, const SyntaxNode*> requiredChainBreakGroups_;
    std::unordered_map<const SyntaxNode*, int> requiredChainBreakBaseIndents_;
    std::unordered_set<const SyntaxNode*> pendingCrossBlockChainGroups_;

    static const SyntaxNode* CrossBlockSourceItem(const SyntaxNode* block) {
        for (
            const SyntaxNode* cursor = block;
            cursor != nullptr && cursor->parent != nullptr;
            cursor = cursor->parent
        ) {
            if (SyntaxNodeHasClass(*cursor->parent, SyntaxNodeClass::SourceItemScope)) {
                return cursor;
            }
        }
        return nullptr;
    }

    static bool HasUniformSplitForm(const FormatBreakNode& node) {
        if (node.kind != FormatBreakNodeKind::Chain || node.operators.size() < 2) {
            return false;
        }
        if (
            node.chainKind == FormatBreakChainKind::MemberBeforeOperator ||
            node.chainKind == FormatBreakChainKind::StreamBeforeOperator
        ) {
            return true;
        }
        if (node.chainKind == FormatBreakChainKind::Ternary) {
            return node.operators.size() > 2;
        }
        return std::all_of(node.operators.begin(), node.operators.end(), [](const FormatBreakToken& token) {
            return FormatBreakTokenKind(token) == PrintTokenKind::Known &&
                SyntaxNodeKindHasClass(FormatBreakTokenSyntaxKind(token), SyntaxNodeClass::ChainOperator);
        });
    }

    size_t ActiveTokenIndex(const FormatBreakToken& token) const {
        if (token.token == nullptr) {
            return tokens_.size();
        }
        const PrintToken* begin = tokens_.data();
        const PrintToken* end = begin + tokens_.size();
        return token.token >= begin && token.token < end ? static_cast<size_t>(token.token - begin) : tokens_.size();
    }

    void CollectCrossBlockChainBreaks(const FormatBreakNode& node, size_t blockIndex) {
        if (HasUniformSplitForm(node)) {
            size_t firstOperator = tokens_.size();
            size_t lastOperator = 0;
            for (const FormatBreakToken& token : node.operators) {
                const size_t index = ActiveTokenIndex(token);
                firstOperator = std::min(firstOperator, index);
                lastOperator = std::max(lastOperator, index);
            }
            const bool crossesBlock = firstOperator < blockIndex && blockIndex < lastOperator;
            if (crossesBlock) {
                const SyntaxNode* group = FormatBreakTokenValue(node.operators.front()).node;
                pendingCrossBlockChainGroups_.insert(group);
                for (const FormatBreakToken& token : node.operators) {
                    const PrintToken& printToken = FormatBreakTokenValue(token);
                    if (printToken.node != nullptr) {
                        requiredChainBreakGroups_.insert_or_assign(printToken.node, group);
                        if (
                            node.chainKind != FormatBreakChainKind::Ternary ||
                            printToken.syntaxKind == SyntaxNodeKind::Colon
                        ) {
                            requiredChainBreakOperators_.insert(printToken.node);
                        }
                    }
                }
            }
        }
        for (const FormatBreakNode* child : node.children) {
            if (child != nullptr) {
                CollectCrossBlockChainBreaks(*child, blockIndex);
            }
        }
        for (const FormatBreakListItem& listItem : node.items) {
            if (listItem.node != nullptr) {
                CollectCrossBlockChainBreaks(*listItem.node, blockIndex);
            }
        }
        for (const FormatBreakNode* operand : node.operands) {
            if (operand != nullptr) {
                CollectCrossBlockChainBreaks(*operand, blockIndex);
            }
        }
    }

    void RecordCrossBlockChainBaseIndents(int baseIndent, const SyntaxNode* selectedGroup = nullptr) {
        for (const auto& [operatorNode, group] : requiredChainBreakGroups_) {
            if (pendingCrossBlockChainGroups_.contains(group) && (selectedGroup == nullptr || group == selectedGroup)) {
                requiredChainBreakBaseIndents_.insert_or_assign(operatorNode, baseIndent);
            }
        }
        if (selectedGroup == nullptr) {
            pendingCrossBlockChainGroups_.clear();
        } else {
            pendingCrossBlockChainGroups_.erase(selectedGroup);
        }
    }

    static bool CanParticipateInUniformCrossBlockChain(const PrintToken& token) {
        if (token.kind != PrintTokenKind::Known) {
            return false;
        }
        if (PrintTokenSyntaxHasClass(token, SyntaxNodeClass::ChainOperator)) {
            return true;
        }
        switch (token.syntaxKind) {
            case SyntaxNodeKind::Comma:
            case SyntaxNodeKind::Dot:
            case SyntaxNodeKind::Arrow:
            case SyntaxNodeKind::DotStar:
            case SyntaxNodeKind::ArrowStar:
            case SyntaxNodeKind::Question:
            case SyntaxNodeKind::Colon:
                return true;
            default:
                return false;
        }
    }

    bool MayHaveCrossBlockChain(size_t begin, size_t block, size_t afterBlock, size_t end) const {
        const auto hasCandidate = [&](size_t first, size_t last) {
            return std::any_of(
                tokens_.begin() + static_cast<std::ptrdiff_t>(first),
                tokens_.begin() + static_cast<std::ptrdiff_t>(last),
                CanParticipateInUniformCrossBlockChain
            );
        };
        // A chain crossing a block has operators outside both ends of the block: its body is a separate subtree,
        // so operators inside it cannot belong to the enclosing chain. Missing closers retain the broader scan.
        // Extra operators only cause a conservative fallthrough to exact analysis.
        return hasCandidate(begin, block) && hasCandidate(afterBlock, end);
    }

    void AnalyzeBlock(size_t currentTokenIndex_) {
        pendingCrossBlockChainGroups_.clear();
        if (currentTokenIndex_ >= tokens_.size()) {
            return;
        }
        const PrintToken& token = tokens_[currentTokenIndex_];
        const SyntaxNode* block = token.node == nullptr ? nullptr : token.node->parent;
        const SyntaxNode* item = CrossBlockSourceItem(block);
        if (item == nullptr) {
            return;
        }
        size_t begin = currentTokenIndex_;
        while (begin > 0 && PrintTokenSyntaxPathContains(tokens_[begin - 1], item)) {
            --begin;
        }
        size_t end = currentTokenIndex_ + 1;
        size_t afterBlock = end;
        while (end < tokens_.size() && PrintTokenSyntaxPathContains(tokens_[end], item)) {
            if (
                tokens_[end].syntaxKind == SyntaxNodeKind::RightBrace &&
                tokens_[end].node != nullptr &&
                tokens_[end].node->parent == block
            ) {
                afterBlock = end + 1;
            }
            ++end;
        }
        if (!MayHaveCrossBlockChain(begin, currentTokenIndex_, afterBlock, end)) {
            return;
        }
        FormatBreakModel model =
            BuildFormatBreakModel(std::span<const PrintToken>{tokens_.data() + begin, end - begin});
        if (model.root != nullptr) {
            CollectCrossBlockChainBreaks(*model.root, currentTokenIndex_);
        }
    }

    void Constrain(FormatBreakModelContext& effectiveContext) const {
        if (!requiredChainBreakOperators_.empty()) {
            effectiveContext.requiredChainBreakOperators = &requiredChainBreakOperators_;
        }
        if (!requiredChainBreakBaseIndents_.empty()) {
            effectiveContext.requiredChainBreakBaseIndents = &requiredChainBreakBaseIndents_;
        }
    }
    void AcceptEmission(std::span<const FormatBreakChainIndent> chains) {
        for (const FormatBreakChainIndent& chain : chains) {
            for (const FormatBreakToken& op : chain.chain->operators) {
                const auto group = requiredChainBreakGroups_.find(FormatBreakTokenValue(op).node);
                if (group != requiredChainBreakGroups_.end()) {
                    RecordCrossBlockChainBaseIndents(chain.baseIndent, group->second);
                }
            }
        }
    }
};

FormatChainContinuation::FormatChainContinuation(std::span<const PrintToken> tokens) :
    impl_(std::make_unique<Impl>(tokens)) {}
FormatChainContinuation::~FormatChainContinuation() = default;
void FormatChainContinuation::AnalyzeBlock(size_t tokenIndex) { impl_->AnalyzeBlock(tokenIndex); }
void FormatChainContinuation::Constrain(FormatBreakModelContext& context) const { impl_->Constrain(context); }
void FormatChainContinuation::AcceptEmission(std::span<const FormatBreakChainIndent> chains) {
    impl_->AcceptEmission(chains);
}
void FormatChainContinuation::FinishBlock(int fallbackBaseIndent) {
    impl_->RecordCrossBlockChainBaseIndents(fallbackBaseIndent);
}
