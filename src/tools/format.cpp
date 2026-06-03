#include "tools/format.h"

#include <string_view>

#include "tools/impl/format_model.h"
#include "tools/impl/format_model_parse.h"
#include "tools/impl/format_pretty_printer.h"

namespace {

enum class LineEndingKind {
    Lf,
    CrLf,
    Cr,
};

std::string_view PlatformLineEnding() {
#ifdef _WIN32
    return "\r\n";
#else
    return "\n";
#endif
}

std::string_view LineEndingText(LineEndingKind kind) {
    switch (kind) {
        case LineEndingKind::Lf:
            return "\n";
        case LineEndingKind::CrLf:
            return "\r\n";
        case LineEndingKind::Cr:
            return "\r";
    }
    return PlatformLineEnding();
}

std::string_view SourceOutputLineEnding(std::string_view text) {
    bool seenLf = false;
    bool seenCrLf = false;
    bool seenCr = false;
    for (size_t index = 0; index < text.size(); ++index) {
        const char ch = text[index];
        if (ch == '\r') {
            if (index + 1 < text.size() && text[index + 1] == '\n') {
                seenCrLf = true;
                ++index;
            } else {
                seenCr = true;
            }
        } else if (ch == '\n') {
            seenLf = true;
        }
    }

    const int styleCount = (seenLf ? 1 : 0) + (seenCrLf ? 1 : 0) + (seenCr ? 1 : 0);
    if (styleCount != 1) {
        return PlatformLineEnding();
    }
    if (seenCrLf) {
        return LineEndingText(LineEndingKind::CrLf);
    }
    if (seenCr) {
        return LineEndingText(LineEndingKind::Cr);
    }
    return LineEndingText(LineEndingKind::Lf);
}

std::string WithLineEndings(std::string_view text, std::string_view lineEnding) {
    std::string result;
    result.reserve(lineEnding.size() == 1 ? text.size() : text.size() + text.size() / 24);
    for (size_t index = 0; index < text.size(); ++index) {
        const char ch = text[index];
        if (ch == '\r') {
            if (index + 1 < text.size() && text[index + 1] == '\n') {
                ++index;
            }
            result.append(lineEnding);
        } else if (ch == '\n') {
            result.append(lineEnding);
        } else {
            result.push_back(ch);
        }
    }
    return result;
}

}  // namespace

SourceFormatResult FormatSourceText(std::string_view text, const FormatterConfig& config, std::string_view sourcePath) {
    FormatModel model = ParseFormatModel(text);
    SourceFormatResult result;
    if (!model.parse.ok) {
        result.ok = false;
        result.error = model.parse.error.empty() ? "parser setup failed" : model.parse.error;
        return result;
    }
    result.formatted =
        WithLineEndings(FormatModelText(config, model, sourcePath), SourceOutputLineEnding(*model.sourceText));
    result.changed = model.sourceText != nullptr && *model.sourceText != result.formatted;
    return result;
}
