#include "format/impl/format_model_parse.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <tree_sitter/api.h>
#include <tree_sitter_cpp.h>

#include "format/impl/format_config.h"
#include "format/impl/format_model_builder.h"

namespace {

enum class ScannerMacroCategory : unsigned {
    RawMacroDefinition = 0,
    BareIdentifier = 1,
    CallSyntax = 2,
    StatementArgument = 3,
    DeclarationPrefix = 4,
    TypeSpecifier = 5,
    PreprocessorArgument = 6,
    SemicolonlessCall = 7,
};

thread_local const FormatterConfig* g_parseConfig = nullptr;

struct TSParserDeleter {
    void operator()(TSParser* parser) const { ts_parser_delete(parser); }
};

struct ParseConfigScope {
    explicit ParseConfigScope(const FormatterConfig& config) : previous(g_parseConfig) { g_parseConfig = &config; }

    ~ParseConfigScope() { g_parseConfig = previous; }

    const FormatterConfig* previous = nullptr;
};

bool MacroEntryMatches(std::string_view entry, std::string_view name) {
    if (!entry.empty() && entry.back() == '*') {
        entry.remove_suffix(1);
        return name.starts_with(entry);
    }
    return entry == name;
}

bool MacroCategoryMatches(const std::vector<std::string>& entries, std::string_view name) {
    for (const std::string& entry : entries) {
        if (MacroEntryMatches(entry, name)) {
            return true;
        }
    }
    return false;
}

bool ConfigMacroCategoryMatches(ScannerMacroCategory category, std::string_view name) {
    if (g_parseConfig == nullptr) {
        return false;
    }
    switch (category) {
        case ScannerMacroCategory::RawMacroDefinition:
            return MacroCategoryMatches(g_parseConfig->rawMacroDefinitions, name);
        case ScannerMacroCategory::BareIdentifier:
            return MacroCategoryMatches(g_parseConfig->bareIdentifierMacros, name);
        case ScannerMacroCategory::CallSyntax:
            return MacroCategoryMatches(g_parseConfig->callSyntaxMacros, name);
        case ScannerMacroCategory::StatementArgument:
            return MacroCategoryMatches(g_parseConfig->statementArgumentMacros, name);
        case ScannerMacroCategory::DeclarationPrefix:
            return MacroCategoryMatches(g_parseConfig->declarationPrefixMacros, name);
        case ScannerMacroCategory::TypeSpecifier:
            return MacroCategoryMatches(g_parseConfig->typeSpecifierMacros, name);
        case ScannerMacroCategory::PreprocessorArgument:
            return MacroCategoryMatches(g_parseConfig->preprocessorArgumentMacros, name);
        case ScannerMacroCategory::SemicolonlessCall:
            return MacroCategoryMatches(g_parseConfig->semicolonlessCallMacros, name);
    }
    return false;
}

TSParser* ThreadFormatParser() {
    thread_local std::unique_ptr<TSParser, TSParserDeleter> parser(ts_parser_new());
    thread_local bool languageReady = parser != nullptr && ts_parser_set_language(parser.get(), tree_sitter_cpp());
    return languageReady ? parser.get() : nullptr;
}

}  // namespace

extern "C" bool strictfmt_tree_sitter_cpp_macro_category_matches(unsigned category, const char* text, unsigned length) {
    return ConfigMacroCategoryMatches(
        static_cast<ScannerMacroCategory>(category), std::string_view(text, static_cast<size_t>(length))
    );
}

FormatModel ParseFormatModel(std::string_view text, const FormatterConfig& config) {
    const ParseConfigScope configScope(config);
    auto sourceText = std::make_unique<std::string>(text);
    TSParser* parser = ThreadFormatParser();
    if (parser == nullptr) {
        FormatModel model;
        model.sourceText = std::move(sourceText);
        model.parse.error = "parser setup failed";
        return model;
    }
    ts_parser_reset(parser);
    TSTree* tree =
        ts_parser_parse_string(parser, nullptr, sourceText->data(), static_cast<uint32_t>(sourceText->size()));
    if (tree == nullptr) {
        FormatModel model;
        model.sourceText = std::move(sourceText);
        model.parse.error = "parse setup failed";
        return model;
    }
    const TSNode root = ts_tree_root_node(tree);
    FormatModel model = BuildFormatModel(root, std::move(sourceText));
    ts_tree_delete(tree);
    return model;
}
