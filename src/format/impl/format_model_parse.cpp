#include "format/impl/format_model_parse.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <tree_sitter/api.h>
#include <tree_sitter_cpp.h>

#include "format/impl/format_config.h"
#include "format/impl/format_model_builder.h"

namespace {

// Order matches MacroCategory in the external scanner.
constexpr std::array kMacroCategoryMembers = {
    &FormatterConfig::rawMacroDefinitions,
    &FormatterConfig::bareIdentifierMacros,
    &FormatterConfig::callSyntaxMacros,
    &FormatterConfig::statementArgumentMacros,
    &FormatterConfig::declarationPrefixMacros,
    &FormatterConfig::typeSpecifierMacros,
    &FormatterConfig::preprocessorArgumentMacros,
    &FormatterConfig::semicolonlessCallMacros,
};

static_assert(kMacroCategoryMembers.size() <= 8);

struct ParseConfigScope;

thread_local const ParseConfigScope* g_parseConfig = nullptr;

struct TSParserDeleter {
    void operator()(TSParser* parser) const { ts_parser_delete(parser); }
};

struct ParseConfigScope {
    explicit ParseConfigScope(const FormatterConfig& value) : config(value), previous(g_parseConfig) {
        for (size_t category = 0; category < kMacroCategoryMembers.size(); ++category) {
            const auto bit = static_cast<std::uint8_t>(1u << category);
            for (const std::string& entry : config.*kMacroCategoryMembers[category]) {
                if (entry == "*") {
                    for (auto& initial : categoriesByInitial) {
                        initial |= bit;
                    }
                } else if (!entry.empty()) {
                    categoriesByInitial[static_cast<unsigned char>(entry.front())] |= bit;
                }
            }
        }
        g_parseConfig = this;
    }

    ~ParseConfigScope() { g_parseConfig = previous; }

    const FormatterConfig& config;
    const ParseConfigScope* previous;
    std::array<std::uint8_t, 256> categoriesByInitial{};
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

bool ConfigMacroCategoryMatches(unsigned category, std::string_view name) {
    if (g_parseConfig == nullptr || category >= kMacroCategoryMembers.size()) {
        return false;
    }
    // A matching exact name or nonempty prefix must begin with the same byte.
    // This filter only rejects; the original matcher handles every possible match.
    if (
        !name.empty() &&
        (g_parseConfig->categoriesByInitial[static_cast<unsigned char>(name.front())] & (1u << category)) == 0
    ) {
        return false;
    }
    return MacroCategoryMatches(g_parseConfig->config.*kMacroCategoryMembers[category], name);
}

TSParser* ThreadFormatParser() {
    thread_local std::unique_ptr<TSParser, TSParserDeleter> parser(ts_parser_new());
    thread_local bool languageReady = parser != nullptr && ts_parser_set_language(parser.get(), tree_sitter_cpp());
    return languageReady ? parser.get() : nullptr;
}

}  // namespace

extern "C" bool strictfmt_tree_sitter_cpp_macro_category_matches(unsigned category, const char* text, unsigned length) {
    return ConfigMacroCategoryMatches(category, std::string_view(text, static_cast<size_t>(length)));
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
