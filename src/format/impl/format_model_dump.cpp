#include "format/impl/format_model_dump.h"

#include <cstdio>
#include <optional>
#include <string>
#include <string_view>

#include "format/impl/format_config.h"
#include "format/impl/format_model.h"
#include "format/impl/format_model_parse.h"
#include "format/impl/format_pretty_printer.h"
#include "tools/tools_common.h"
#include "util/file_path.h"

namespace {

constexpr size_t kQuotedTextLimit = 60;
constexpr int kYamlIndentSpaces = 2;

void WriteIndent(FILE* output, int indent) {
    for (int index = 0; index < indent * kYamlIndentSpaces; ++index) {
        std::fputc(' ', output);
    }
}

void WriteString(FILE* output, std::string_view text) {
    if (!text.empty()) {
        std::fwrite(text.data(), 1, text.size(), output);
    }
}

void WriteQuotedText(FILE* output, std::string_view text) {
    std::fputc('"', output);
    for (const unsigned char ch : text) {
        switch (ch) {
            case '\\':
                std::fputs("\\\\", output);
                break;
            case '"':
                std::fputs("\\\"", output);
                break;
            case '\n':
                std::fputs("\\n", output);
                break;
            case '\r':
                std::fputs("\\r", output);
                break;
            case '\t':
                std::fputs("\\t", output);
                break;
            default:
                if (ch < 0x20) {
                    std::fprintf(output, "\\x%02X", static_cast<unsigned int>(ch));
                } else {
                    std::fputc(static_cast<char>(ch), output);
                }
                break;
        }
    }
    std::fputc('"', output);
}

void WriteTextValue(FILE* output, std::string_view text) {
    if (text.size() <= kQuotedTextLimit) {
        WriteQuotedText(output, text);
        return;
    }
    std::fprintf(output, "%zu", text.size());
}

void WriteNode(FILE* output, const SyntaxNode& node, int indent, bool listItem) {
    const int fieldIndent = listItem ? indent + 1 : indent;
    WriteIndent(output, indent);
    if (listItem) {
        std::fputs("- kind: ", output);
    } else {
        std::fputs("kind: ", output);
    }
    WriteString(output, SyntaxNodeKindName(node.kind));
    std::fputc('\n', output);

    if (!node.text.empty()) {
        WriteIndent(output, fieldIndent);
        std::fputs("text: ", output);
        WriteTextValue(output, node.text);
        std::fputc('\n', output);
    }

    if (node.children.empty()) {
        return;
    }
    WriteIndent(output, fieldIndent);
    std::fputs("children:\n", output);
    for (const SyntaxNode* child : node.children) {
        if (child != nullptr) {
            WriteNode(output, *child, fieldIndent + 1, true);
        }
    }
}

void PrintUsage(FILE* output) { std::fprintf(output, "Usage: format_model_dump <source-file>\n"); }

enum class DumpTreeKind {
    Syntax,
    Break,
};

int DumpTreeText(
    std::string_view sourceText,
    const FormatterConfig& config,
    std::string_view sourcePath,
    DumpTreeKind kind,
    FILE* output,
    FILE* errorOutput,
    std::string_view commandName
) {
    FormatModel model = ParseFormatModel(sourceText, config);
    if (model.root == nullptr) {
        if (!model.parse.ok) {
            const std::string error =
                model.parse.error.empty() ? std::string("parser setup failed") : model.parse.error;
            std::fprintf(
                errorOutput,
                "%.*s: parse failed: %s\n",
                static_cast<int>(commandName.size()),
                commandName.data(),
                error.c_str()
            );
            return 1;
        }
        std::fprintf(
            errorOutput, "%.*s: parse produced no root node\n", static_cast<int>(commandName.size()), commandName.data()
        );
        return 1;
    }

    if (kind == DumpTreeKind::Syntax) {
        WriteNode(output, *model.root, 0, false);
    } else {
        DumpFormatBreakTrees(config, model, sourcePath, output);
    }
    if (model.parse.ok) {
        return 0;
    }
    const std::string error = model.parse.error.empty() ? std::string("parser setup failed") : model.parse.error;
    std::fprintf(
        errorOutput, "%.*s: parse failed: %s\n", static_cast<int>(commandName.size()), commandName.data(), error.c_str()
    );
    return 1;
}

int DumpTreeFile(
    std::string_view sourcePath,
    const std::optional<std::string>& explicitStylePath,
    DumpTreeKind kind,
    FILE* output,
    FILE* errorOutput,
    std::string_view commandName
) {
    const std::string path = AbsolutePath(sourcePath);
    const std::optional<std::string> text = ReadFileBinary(path);
    if (!text.has_value()) {
        std::fprintf(
            errorOutput,
            "%.*s: cannot read file: %s\n",
            static_cast<int>(commandName.size()),
            commandName.data(),
            path.c_str()
        );
        return 1;
    }

    FormatStyleCache styleCache(explicitStylePath);
    std::string configError;
    const FormatterConfig* config = styleCache.ConfigForPath(path, configError);
    if (config == nullptr) {
        std::fprintf(
            errorOutput, "%.*s: %s\n", static_cast<int>(commandName.size()), commandName.data(), configError.c_str()
        );
        return 2;
    }

    return DumpTreeText(*text, *config, path, kind, output, errorOutput, commandName);
}

}  // namespace

int DumpFormatModel(
    std::string_view sourcePath,
    const std::optional<std::string>& explicitStylePath,
    FILE* output,
    FILE* errorOutput,
    std::string_view commandName
) {
    return DumpTreeFile(sourcePath, explicitStylePath, DumpTreeKind::Syntax, output, errorOutput, commandName);
}

int DumpFormatModelText(
    std::string_view sourceText,
    const FormatterConfig& config,
    FILE* output,
    FILE* errorOutput,
    std::string_view commandName
) {
    return DumpTreeText(sourceText, config, "<stdin>", DumpTreeKind::Syntax, output, errorOutput, commandName);
}

int DumpFormatBreakTree(
    std::string_view sourcePath,
    const std::optional<std::string>& explicitStylePath,
    FILE* output,
    FILE* errorOutput,
    std::string_view commandName
) {
    return DumpTreeFile(sourcePath, explicitStylePath, DumpTreeKind::Break, output, errorOutput, commandName);
}

int DumpFormatBreakTreeText(
    std::string_view sourceText,
    const FormatterConfig& config,
    std::string_view sourcePath,
    FILE* output,
    FILE* errorOutput,
    std::string_view commandName
) {
    return DumpTreeText(sourceText, config, sourcePath, DumpTreeKind::Break, output, errorOutput, commandName);
}

int RunFormatModelDump(int argc, char** argv) {
    if (argc != 1) {
        PrintUsage(stderr);
        return 2;
    }
    return DumpFormatModel(argv[0], std::nullopt, stdout, stderr, "format_model_dump");
}
