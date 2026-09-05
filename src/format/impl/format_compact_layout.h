#pragma once

#include <memory>
#include <string_view>

struct FormatBreakModel;
struct FormatBreakNode;
struct FormatBreakToken;

struct FormatCompactLine {
    bool hasContextOnlyTokens = false;
    bool valid = false;
    bool producesText = false;
    int widthWithoutLeadingText = 0;
    int widthWithLeadingText = 0;
};

// Measures the exact legal compact physical line, including normalized spellings
// and spacing. Invalid measurements require general layout solving. The cache is
// scoped to one immutable break model; it owns no solver choices or cost state.
class FormatCompactLayout {
public:
    explicit FormatCompactLayout(const FormatBreakModel& model);
    ~FormatCompactLayout();

    const FormatCompactLine& Measure(const FormatBreakNode& node) const;
    static FormatCompactLine MeasureToken(const FormatBreakToken& token, std::string_view text);

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};
