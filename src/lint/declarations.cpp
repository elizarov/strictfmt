#include "lint/declarations.h"

#include <algorithm>
#include <string>
#include <set>
#include <vector>

#include "syntax/tree_sitter.h"

namespace {

bool IsIdentifier(TSNode node) {
    const auto type = NodeType(node);
    return type == "identifier" ||
        type == "field_identifier" ||
        type == "type_identifier" ||
        type == "namespace_identifier";
}

bool HasDirectText(TSNode node, std::string_view text, std::string_view source) {
    bool found = false;
    VisitChildren(node, [&](TSNode child, std::string_view) { found |= NodeText(child, source) == text; });
    return found;
}

struct Declarator {
    TSNode name = {};
    TSNode bindings = {};
    bool callable = false;
    bool qualified = false;
    bool indirect = false;
    bool isConst = false;
    std::optional<bool> indirectObjectConst;
    bool overrides = false;
    bool isFinal = false;
    TSNode parameters = {};
};

Declarator ReadDeclarator(TSNode node, std::string_view source) {
    Declarator result;
    std::vector<TSNode> spine;
    while (!ts_node_is_null(node)) {
        spine.push_back(node);
        if (IsIdentifier(node)) {
            result.name = node;
            break;
        }
        const auto type = NodeType(node);
        if (
            type == "qualified_identifier" ||
            type == "template_function" ||
            type == "template_method" ||
            type == "template_type"
        ) {
            result.qualified = true;
            node = NodeField(node, "name");
            continue;
        }
        if (type == "structured_binding_declarator") {
            result.bindings = node;
            break;
        }
        if (type == "function_declarator") {
            VisitChildren(node, [&](TSNode child, std::string_view) {
                if (NodeType(child) == "virtual_specifier") {
                    result.overrides |=
                        HasDirectText(child, "override", source) || NodeText(child, source) == "override";
                    result.isFinal |= HasDirectText(child, "final", source) || NodeText(child, source) == "final";
                }
            });
        }
        TSNode next = NodeField(node, "declarator");
        if (ts_node_is_null(next) && (
            type == "parenthesized_declarator" ||
            type == "attributed_declarator" ||
            type == "variadic_declarator" ||
            type == "reference_declarator"
        )) {
            VisitChildren(node, [&](TSNode child, std::string_view) {
                const auto childType = NodeType(child);
                if (
                    ts_node_is_named(child) &&
                    childType != "comment" &&
                    childType != "attribute_declaration" &&
                    childType != "type_qualifier"
                ) {
                    next = child;
                }
            });
        }
        node = next;
    }
    // The operator nearest the name determines whether this declares a function,
    // a pointer/reference object, or an array. Return types are outside that boundary.
    for (auto it = spine.rbegin(); it != spine.rend(); ++it) {
        const auto type = NodeType(*it);
        if (type == "function_declarator" || type == "abstract_function_declarator") {
            result.callable = true;
            result.parameters = NodeField(*it, "parameters");
            result.isConst = HasDirectText(*it, "const", source);
            // Keep reading the outer declarator for a possible object initializer.
            continue;
        }
        if (
            type == "pointer_declarator" ||
            type == "member_pointer_declarator" ||
            type == "reference_declarator" ||
            type == "handle_declarator" ||
            type == "abstract_pointer_declarator" ||
            type == "abstract_reference_declarator"
        ) {
            result.indirectObjectConst = type != "reference_declarator" &&
                type != "abstract_reference_declarator" &&
                HasDirectText(*it, "const", source);
            if (!result.callable) {
                result.indirect = true;
                result.isConst = *result.indirectObjectConst;
            }
            break;
        }
    }
    return result;
}

bool CouldBeDirectInitializer(TSNode parameters, std::string_view source) {
    bool hasParameter = false;
    bool definiteFunction = false;
    VisitChildren(parameters, [&](TSNode parameter, std::string_view) {
        const auto type = NodeType(parameter);
        if (type == "comment" || !ts_node_is_named(parameter)) {
            return;
        }
        hasParameter = true;
        const auto parameterType = NodeField(parameter, "type");
        definiteFunction |= NodeType(parameterType) == "primitive_type" ||
            NodeType(parameterType) == "sized_type_specifier" ||
            HasDirectText(parameter, "const", source) ||
            HasDirectText(parameter, "volatile", source) ||
            IsIdentifier(NodeField(parameter, "declarator"));
    });
    return hasParameter && !definiteFunction;
}

bool CouldBeParameter(TSNode argument, std::string_view source) {
    const auto type = NodeType(argument);
    if (IsIdentifier(argument) || type == "qualified_identifier" || type == "call_expression") {
        return true;
    }
    if (type == "binary_expression") {
        const auto op = NodeText(NodeField(argument, "operator"), source);
        return op == "*" || op == "&" || op == "&&";
    }
    if (type == "argument_list" || type == "argument_sequence") {
        bool any = false;
        bool all = true;
        VisitChildren(argument, [&](TSNode child, std::string_view) {
            if (!ts_node_is_named(child) || NodeType(child) == "comment") {
                return;
            }
            any = true;
            all &= CouldBeParameter(child, source);
        });
        return any && all;
    }
    return false;
}

bool OnlyForwardDeclarations(TSNode node) {
    const auto type = NodeType(node);
    if (
        type == "class_specifier" || type == "struct_specifier" || type == "union_specifier" || type == "enum_specifier"
    ) {
        return ts_node_is_null(NodeField(node, "body"));
    }
    if (type == "comment" || !ts_node_is_named(node)) {
        return true;
    }
    if (
        type != "declaration_list" &&
        type != "namespace_declaration_list" &&
        type != "declaration" &&
        type != "namespace_definition"
    ) {
        return false;
    }
    bool only = true;
    VisitChildren(node, [&](TSNode child, std::string_view field) {
        if (type == "namespace_definition" && field != "body") {
            return;
        }
        only &= OnlyForwardDeclarations(child);
    });
    return only;
}

struct Context {
    DeclarationScope scope = DeclarationScope::Global;
    DeclarationAccess access = DeclarationAccess::Public;
    std::string_view className;
    bool templateParameters = false;
    bool templateTemplate = false;
    bool catchParameters = false;
    bool friendDeclaration = false;
    unsigned possibleAccess = 1u << static_cast<unsigned>(DeclarationAccess::Public);
};

class DeclarationVisitor {
public:
    DeclarationVisitor(std::string_view source, const std::function<void(const Declaration&)>& visit) :
        source_(source), visit_(visit) {}

    void CollectMacros(TSNode node) {
        if (NodeType(node) == "preproc_def") {
            macros_.insert(std::string(NodeText(NodeField(node, "name"), source_)));
        }
        VisitChildren(node, [&](TSNode child, std::string_view) { CollectMacros(child); });
    }

    unsigned Walk(TSNode node, Context context) {
        const auto type = NodeType(node);
        if (type == "parameter_list" && ambiguousParameters_.contains(ts_node_start_byte(node))) {
            return context.possibleAccess;
        }
        if (type == "friend_declaration") {
            context.friendDeclaration = true;
        }
        if (type == "catch_clause") {
            context.catchParameters = true;
        }
        if (type == "comment" || type == "string_literal" || type == "raw_string_literal") {
            return context.possibleAccess;
        }
        if (type == "preproc_def" || type == "preproc_function_def") {
            Emit(NodeField(node, "name"), DeclarationKind::Macro, context);
            return context.possibleAccess;
        }
        if (type == "raw_macro_replacement" || type == "macro_replacement_list") {
            return context.possibleAccess;
        }
        if (type == "namespace_definition") {
            TSNode name = NodeField(node, "name");
            if (!OnlyForwardDeclarations(NodeField(node, "body"))) {
                NamespaceNames(name, context);
            }
            context.scope = DeclarationScope::Global;
            context.className = {};
        } else if (type == "class_specifier" || type == "struct_specifier" || type == "union_specifier") {
            TSNode name = NodeField(node, "name");
            if (!ts_node_is_null(NodeField(node, "body"))) {
                Emit(
                    name,
                    type == "class_specifier" ? DeclarationKind::Class :
                        type == "struct_specifier" ? DeclarationKind::Struct :
                        DeclarationKind::Union,
                    context
                );
            }
            context.className = NodeText(name, source_);
            context.scope = DeclarationScope::Member;
            context.access = type == "class_specifier" ? DeclarationAccess::Private : DeclarationAccess::Public;
            context.possibleAccess = 1u << static_cast<unsigned>(context.access);
        } else if (type == "enum_specifier") {
            if (!ts_node_is_null(NodeField(node, "body"))) {
                Emit(NodeField(node, "name"), DeclarationKind::Enum, context);
            }
        } else if (type == "enumerator") {
            Emit(NodeField(node, "name"), DeclarationKind::EnumConstant, context);
        } else if (type == "concept_definition") {
            Emit(NodeField(node, "name"), DeclarationKind::Concept, context);
        } else if (type == "namespace_alias_definition") {
            Emit(NodeField(node, "name"), DeclarationKind::Namespace, context);
        } else if (type == "alias_declaration" || type == "function_pointer_alias_declaration") {
            Emit(NodeField(node, "name"), DeclarationKind::TypeAlias, context);
            const auto descriptor = NodeField(node, "type");
            const auto declarator = NodeField(descriptor, "declarator");
            RememberAlias(
                NodeField(node, "name"),
                ReadDeclarator(declarator, source_).callable ||
                    (ts_node_is_null(declarator) && IsFunctionAlias(NodeField(descriptor, "type"))),
                node
            );
        } else if (
            type == "type_parameter_declaration" ||
            type == "optional_type_parameter_declaration" ||
            type == "variadic_type_parameter_declaration"
        ) {
            TSNode name = NodeField(node, "name");
            if (ts_node_is_null(name)) {
                VisitChildren(node, [&](TSNode child, std::string_view) {
                    if (IsIdentifier(child)) {
                        name = child;
                    }
                });
            }
            Emit(
                name,
                context.templateTemplate ? DeclarationKind::TemplateTemplateParameter :
                    DeclarationKind::TypeTemplateParameter,
                context
            );
        } else if (type == "template_template_parameter_declaration") {
            VisitChildren(node, [&](TSNode child, std::string_view field) {
                Context nested = context;
                nested.templateTemplate = field != "parameters";
                Walk(child, nested);
            });
            return context.possibleAccess;
        } else if (
            type == "declaration" ||
            type == "field_declaration" ||
            type == "function_definition" ||
            type == "type_definition" ||
            type == "parameter_declaration" ||
            type == "optional_parameter_declaration" ||
            type == "variadic_parameter_declaration" ||
            type == "for_range_loop"
        ) {
            VisitDeclarators(node, context);
        } else if (type == "lambda_capture_initializer") {
            Emit(NodeField(node, "left"), DeclarationKind::Variable, Context{.scope = DeclarationScope::Local});
        }

        if (
            type == "function_definition" ||
            type == "compound_statement" ||
            type == "lambda_expression" ||
            type == "for_range_loop"
        ) {
            context.scope = DeclarationScope::Local;
            context.className = {};
            context.catchParameters = false;
        }
        if (type == "template_parameter_list") {
            context.templateParameters = true;
            context.templateTemplate = false;
        }
        if (type == "parameter_list") {
            context.templateParameters = false;
            context.templateTemplate = false;
        }
        const Context incoming = context;
        unsigned alternativeAccess = 0;
        bool hasAlternative = false;
        VisitChildren(node, [&](TSNode child, std::string_view field) {
            if (field == "alternative") {
                hasAlternative = true;
                alternativeAccess |= Walk(child, incoming);
                return;
            }
            if (NodeType(child) == "access_specifier_label") {
                const auto text = NodeText(child, source_);
                if (text.starts_with("private")) {
                    context.access = DeclarationAccess::Private;
                } else if (text.starts_with("protected")) {
                    context.access = DeclarationAccess::Protected;
                } else {
                    context.access = DeclarationAccess::Public;
                }
                context.possibleAccess = 1u << static_cast<unsigned>(context.access);
            }
            Context nested = context;
            if (type == "template_declaration" && field != "parameters") {
                nested.templateParameters = false;
            }
            const auto access = Walk(child, nested);
            const auto childType = NodeType(child);
            if (childType == "preproc_if" || childType == "preproc_ifdef") {
                context.possibleAccess = access;
            }
        });
        if (type == "preproc_if" || type == "preproc_ifdef" || type == "preproc_elif" || type == "preproc_elifdef") {
            return context.possibleAccess | (hasAlternative ? alternativeAccess : incoming.possibleAccess);
        }
        return context.possibleAccess;
    }

private:
    struct Alias {
        std::string_view name;
        uint32_t begin;
        uint32_t end;
        bool callable;
    };

    std::vector<Alias> aliases_;

    bool IsFunctionAlias(TSNode type) const {
        if (!IsIdentifier(type)) {
            return false;
        }
        const auto name = NodeText(type, source_);
        const auto at = ts_node_start_byte(type);
        for (auto it = aliases_.rbegin(); it != aliases_.rend(); ++it) {
            if (it->name == name && it->begin <= at && at < it->end) {
                return it->callable;
            }
        }
        return false;
    }
    void RememberAlias(TSNode name, bool callable, TSNode declaration) {
        if (!IsIdentifier(name)) {
            return;
        }
        auto scope = ts_node_parent(declaration);
        while (!ts_node_is_null(scope)) {
            const auto type = NodeType(scope);
            if (
                type == "translation_unit" ||
                type == "namespace_declaration_list" ||
                type == "field_declaration_list" ||
                type == "compound_statement" ||
                type == "template_declaration"
            ) {
                break;
            }
            scope = ts_node_parent(scope);
        }
        aliases_.push_back({NodeText(name, source_), ts_node_end_byte(name), ts_node_end_byte(scope), callable});
    }

    std::set<uint32_t> ambiguousParameters_;
    std::set<std::string, std::less<>> macros_;
    std::string_view source_;
    const std::function<void(const Declaration&)>& visit_;

    void Emit(TSNode name, DeclarationKind kind, const Context& context) {
        if (
            !ts_node_is_null(name) &&
            IsIdentifier(name) &&
            (kind == DeclarationKind::Macro || !macros_.contains(NodeText(name, source_)))
        ) {
            Declaration declaration{name, {kind, context.scope, context.access}};
            declaration.possibleAccess = context.possibleAccess;
            visit_(declaration);
        }
    }
    void NamespaceNames(TSNode node, const Context& context) {
        if (ts_node_is_null(node)) {
            return;
        }
        if (IsIdentifier(node)) {
            Emit(node, DeclarationKind::Namespace, context);
            return;
        }
        VisitChildren(node, [&](TSNode child, std::string_view) { NamespaceNames(child, context); });
    }
    void VisitDeclarators(TSNode node, const Context& context) {
        const auto type = NodeType(node);
        const bool isParameter = type == "parameter_declaration" ||
            type == "optional_parameter_declaration" ||
            type == "variadic_parameter_declaration";
        const bool isTypedef = type == "type_definition" || HasDirectText(node, "typedef", source_);
        const bool isStatic = HasDirectText(node, "static", source_);
        const bool isConstexpr = HasDirectText(node, "constexpr", source_);
        const bool isConst = isConstexpr || HasDirectText(node, "const", source_);
        const bool isFriend = context.friendDeclaration || HasDirectText(node, "friend", source_);
        VisitChildren(node, [&](TSNode child, std::string_view field) {
            if (field != "declarator") {
                return;
            }
            Declarator declarator = ReadDeclarator(child, source_);
            if (declarator.qualified) {
                return;
            }
            const auto emit = [&](TSNode nameNode) {
                if (ts_node_is_null(nameNode) || macros_.contains(NodeText(nameNode, source_))) {
                    return;
                }
                Declaration declaration{
                    nameNode,
                    {
                        DeclarationKind::Variable,
                        context.scope,
                        context.access,
                        isConstexpr || declarator.indirectObjectConst.value_or(isConst),
                        isConstexpr,
                        isStatic
                    }
                };
                declaration.possibleAccess = context.possibleAccess;
                if (!ts_node_is_null(declarator.bindings)) {
                    declaration.facts.kind = DeclarationKind::Binding;
                } else if (isTypedef) {
                    declaration.facts.kind = DeclarationKind::Typedef;
                    RememberAlias(
                        nameNode,
                        declarator.callable || (!declarator.indirect && IsFunctionAlias(NodeField(node, "type"))),
                        node
                    );
                } else if (isParameter) {
                    declaration.facts.scope = DeclarationScope::Local;
                    declaration.facts.kind = context.catchParameters ? DeclarationKind::Variable :
                        context.templateParameters ? DeclarationKind::ValueTemplateParameter :
                        type == "variadic_parameter_declaration" ? DeclarationKind::ParameterPack :
                        DeclarationKind::Parameter;
                } else if (declarator.callable || (
                    !declarator.indirect &&
                    NodeType(child) != "init_declarator" &&
                    IsFunctionAlias(NodeField(node, "type"))
                )) {
                    if (ts_node_is_null(NodeField(node, "type"))) {
                        return;
                    }
                    const DeclarationFacts objectFacts = declaration.facts;
                    declaration.facts.kind = context.scope == DeclarationScope::Member && !isFriend ?
                        DeclarationKind::Method : DeclarationKind::Function;
                    declaration.facts.isConst = declarator.isConst;
                    if (isFriend) {
                        declaration.facts.scope = DeclarationScope::Global;
                    }
                    const auto name = NodeText(declarator.name, source_);
                    // A final method without a virtual specifier inherits its virtual identity.
                    const bool inheritedFinal = declarator.isFinal && !HasDirectText(node, "virtual", source_);
                    if (name == "main" || name == context.className || declarator.overrides || inheritedFinal) {
                        return;
                    }
                    if (
                        type != "function_definition" &&
                        declaration.facts.kind == DeclarationKind::Function &&
                        NodeText(NodeField(node, "type"), source_) != "void" &&
                        CouldBeDirectInitializer(declarator.parameters, source_)
                    ) {
                        declaration.alternative = objectFacts;
                        ambiguousParameters_.insert(ts_node_start_byte(declarator.parameters));
                    }
                } else if (context.scope == DeclarationScope::Member && !isStatic) {
                    declaration.facts.kind = DeclarationKind::Field;
                }
                if (
                    !declarator.callable &&
                    !declarator.indirect &&
                    ts_node_is_null(declarator.bindings) &&
                    !isParameter &&
                    !isTypedef &&
                    context.scope != DeclarationScope::Member &&
                    NodeType(child) == "init_declarator" &&
                    NodeType(NodeField(child, "value")) == "argument_list" &&
                    CouldBeParameter(NodeField(child, "value"), source_)
                ) {
                    declaration.alternative = declaration.facts;
                    declaration.alternative->kind = DeclarationKind::Function;
                    declaration.alternative->isConst = false;
                }
                visit_(declaration);
            };
            if (!ts_node_is_null(declarator.bindings)) {
                VisitChildren(declarator.bindings, [&](TSNode name, std::string_view) {
                    if (IsIdentifier(name)) {
                        emit(name);
                    }
                });
            } else {
                emit(declarator.name);
            }
        });
    }
};

}  // namespace

void VisitDeclarations(TSNode root, std::string_view source, const std::function<void(const Declaration&)>& visit) {
    DeclarationVisitor visitor(source, visit);
    visitor.CollectMacros(root);
    visitor.Walk(root, {});
}

bool MatchesDeclaration(const DeclarationSelector& selector, const DeclarationFacts& declaration) {
    return (selector.kinds & (uint64_t{1} << static_cast<unsigned>(declaration.kind))) != 0 &&
        (selector.scopes == 0 || (selector.scopes & (1u << static_cast<unsigned>(declaration.scope))) != 0) && (
            !selector.access ||
            (declaration.scope == DeclarationScope::Member && *selector.access == declaration.access)
        ) &&
        (!selector.isConst || *selector.isConst == declaration.isConst) &&
        (!selector.isConstexpr || *selector.isConstexpr == declaration.isConstexpr) &&
        (!selector.isStatic || *selector.isStatic == declaration.isStatic);
}
