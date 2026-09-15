#include "format/impl/format_layout_tree.h"

#include <algorithm>
#include <stdexcept>

#include "format/impl/format_break_model_builder.h"
#include "format/impl/format_layout_projection.h"
#include "format/impl/format_list_continuation.h"
#include "format/impl/format_chain_continuation.h"

FormatLayoutTree::FormatLayoutTree(std::span<const PrintToken> tokens) : tokens_(tokens) {
    owners_.push_back({});
    for (size_t index = 0; index < tokens.size(); ++index) {
        FormatLayoutOwnerId id = AddOwner(tokens[index].node);
        while (id != 0) {
            FormatLayoutOwner& owner = owners_[id];
            owner.begin = std::min(owner.begin, index);
            owner.end = index + 1;
            id = owner.parent;
        }
    }
    structuralIndents_.resize(owners_.size());
    lists_ = std::make_unique<FormatListContinuation>(*this);
    chains_ = std::make_unique<FormatChainContinuation>(*this);
}

FormatLayoutTree::~FormatLayoutTree() = default;
FormatListContinuation& FormatLayoutTree::Lists() { return *lists_; }
FormatChainContinuation& FormatLayoutTree::Chains() { return *chains_; }

FormatLayoutOwnerId FormatLayoutTree::AddOwner(const SyntaxNode* syntax) {
    if (syntax == nullptr) {
        return 0;
    }
    if (const auto found = ownerIds_.find(syntax); found != ownerIds_.end()) {
        return found->second;
    }
    const FormatLayoutOwnerId parent = AddOwner(syntax->parent);
    const FormatLayoutOwnerId id = owners_.size();
    owners_.push_back({.id = id, .parent = parent, .syntax = syntax, .begin = tokens_.size()});
    owners_[parent].children.push_back(id);
    ownerIds_.emplace(syntax, id);
    return id;
}

std::span<const PrintToken> FormatLayoutTree::Tokens() const { return tokens_; }
const FormatLayoutOwner& FormatLayoutTree::Owner(FormatLayoutOwnerId id) const { return owners_.at(id); }

FormatLayoutOwnerId FormatLayoutTree::FindOwner(const SyntaxNode* syntax) const {
    const auto found = ownerIds_.find(syntax);
    return found == ownerIds_.end() ? 0 : found->second;
}

FormatLayoutOwnerId FormatLayoutTree::SourceItem(const SyntaxNode* syntax) const {
    for (auto id = FindOwner(syntax); id != 0; id = owners_[id].parent) {
        const auto parent = owners_[id].parent;
        if (parent != 0 && SyntaxNodeHasClass(*owners_[parent].syntax, SyntaxNodeClass::SourceItemScope)) {
            return id;
        }
    }
    return 0;
}

void FormatLayoutTree::BeginToken(const PrintToken& token, int structuralIndent) {
    for (auto id = FindOwner(token.node); id != 0 && !structuralIndents_[id]; id = owners_[id].parent) {
        structuralIndents_[id] = structuralIndent;
    }
}

void
    FormatLayoutTree::ConstrainBodyHeader(FormatLayoutRegionContext& context, std::span<const PrintToken> tokens) const
{
    if (tokens.empty()) {
        return;
    }
    for (const auto& token : tokens) {
        const auto* body = token.node == nullptr ? nullptr : token.node->parent;
        const auto* declaration = body == nullptr ? nullptr : body->parent;
        if (
            token.node == nullptr ||
            token.node->kind != SyntaxNodeKind::LeftBrace ||
            declaration == nullptr ||
            !SyntaxNodeHasClass(*body, SyntaxNodeClass::CompoundBlock) || (
                declaration->kind != SyntaxNodeKind::FunctionDefinition &&
                !SyntaxNodeHasClass(*declaration, SyntaxNodeClass::DeclaredTypeSpecifier)
            )
        ) {
            continue;
        }
        const auto owner = FindOwner(declaration);
        if (owner == 0 || owners_[owner].begin >= tokens.front().sourceIndex) {
            continue;
        }
        if (!structuralIndents_[owner]) {
            throw std::logic_error("layout header owner has no structural indentation");
        }
        context.continuedBodyHeader = body;
        context.continuedBodyHeaderOwnerIndent = *structuralIndents_[owner];
        return;
    }
}

const FormatBreakModel& FormatLayoutTree::CompleteModel(FormatLayoutOwnerId owner) {
    const auto found = completeModels_.find(owner);
    if (found != completeModels_.end()) {
        return found->second;
    }
    const FormatLayoutOwner& item = owners_.at(owner);
    return completeModels_
        .emplace(owner, BuildFormatBreakModel(tokens_.subspan(item.begin, item.end - item.begin))).first->second;
}

FormatLayoutRegion&
    FormatLayoutTree::AddRegion(std::span<const PrintToken> tokens, const FormatLayoutRegionContext& context)
{
    FormatLayoutRegion& region = regions_.emplace_back();
    region.owner = tokens.empty() ? 0 : SourceItem(tokens.front().node);
    region.tokens.assign(tokens.begin(), tokens.end());
    auto common = tokens.empty() ? 0 : FindOwner(tokens.front().node);
    for (const auto& token : tokens) {
        auto other = FindOwner(token.node);
        while (common != other) {
            if (common > other) {
                common = owners_[common].parent;
            } else {
                other = owners_[other].parent;
            }
        }
    }
    region.model = ProjectFormatLayout(CompleteModel(common), region.tokens, context);
    return region;
}

void FormatLayoutTree::Complete(FormatLayoutProgram program) {
    if (program_) {
        throw std::logic_error("layout tree is already complete");
    }
    program_ = std::move(program);
}
const FormatLayoutProgram& FormatLayoutTree::Program() const { return program_.value(); }
void FormatLayoutTree::RecordBlockIndent(const SyntaxNode* token, int indent) {
    blockIndents_.insert_or_assign(token, indent);
}
std::optional<int> FormatLayoutTree::BlockIndent(const SyntaxNode* token) const {
    const auto found = blockIndents_.find(token);
    return found == blockIndents_.end() ? std::nullopt : std::optional(found->second);
}

void FormatLayoutTree::VisitCompleteModels(const std::function<void(const FormatBreakModel&)>& visitor) const {
    for (const auto& [owner, model] : completeModels_) {
        visitor(model);
    }
}
