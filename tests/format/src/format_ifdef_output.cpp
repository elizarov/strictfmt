#ifndef FORMAT_IFDEF_FIXTURE_HPP
#define FORMAT_IFDEF_FIXTURE_HPP

// Golden fixture for allowed conditional preprocessor formatting.
// This project forbids ifdefs, and these examples come only from userver, so the fixture
// is named format_ifdef_* rather than format_userver_ifdef_*. Keep future userver examples
// that patch whole declarations, statements, fields, methods, macros, includes, or
// comma-separated list items here. Keep conditional fragments inside expressions,
// statement headers, or declaration suffixes in format_error_input.cpp.

#include <userver/utils/assert.hpp>
#include <algorithm>
#include <boost/atomic/atomic.hpp>

#include "src/kafka/impl/consumer_impl.hpp"
#include <userver/utest/utest.hpp>
#include <fmt/format.h>
#include <string_view>
#include "userver/chaotic/io/my/custom_object.hpp"

#include <userver/logging/log.hpp>
#include <google/protobuf/descriptor.h>
#include <grpcpp/grpcpp.h>
#include <array>

#if FORMAT_USERVER_USE_SYSTEM_HEADER
#include <format_userver/system.hpp>
#else
#include "format_userver/system.hpp"
#endif

#ifdef FORMAT_USERVER_PROTECT_ATTR
#define FORMAT_USERVER_PROTECTED_ATTR __attribute__((noinline, flatten))
#else
#define FORMAT_USERVER_PROTECTED_ATTR __attribute__((always_inline, flatten))
#endif

#ifdef FORMAT_USERVER_HAS_ATTRIBUTE
#if FORMAT_USERVER_HAS_NODEBUG
#define USERVER_IMPL_NODEBUG __attribute__((__nodebug__))
#define USERVER_IMPL_NODEBUG_INLINE_FUNC __attribute__((__nodebug__, __always_inline__))
#elif FORMAT_USERVER_HAS_ALWAYS_INLINE
// GCC may have no __nodebug__ attribute.
#define USERVER_IMPL_NODEBUG_INLINE_FUNC __attribute__((__always_inline__))
#endif
#endif

#if FORMAT_USERVER_LEGACY_FMT
#define FORMAT_USERVER_CONST
#else
#define FORMAT_USERVER_CONST const
#endif

#if FORMAT_USERVER_HAS_NAMESPACE_ALIAS
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CURL_FORMAT_USERVER_NAMESPACE fixture::
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CURL_FORMAT_USERVER_NAMESPACE
#endif

namespace format_userver_fixture {

#if FORMAT_USERVER_LEGACY_FMT
template <typename S>
const S& LegacyRuntime(const S& s) {
    return s;
}
#endif

#if defined(FORMAT_USERVER_PLATFORM) && __has_include(<format_userver/header.hpp>)
void HasIncludeGuardedFunction() {
    UsePlatformHeader();
}
#else
void HasIncludeGuardedFallback() {
    UseFallbackHeader();
}
#endif

void ConditionalStatementGuard() {
#ifdef SOCK_CLOEXEC
    x &= ~SOCK_CLOEXEC;
#endif
#include "format_userver_statement.inc"
    UseAfterInclude();
}

void ConditionalLocalDeclaration() {
#ifdef FORMAT_USERVER_LIBCPP
    const std::string expected = "libcpp";
#else
    const std::string expected = "stdlib";
#endif
    Use(expected);
}

constexpr utils::StringLiteral kFormatUserverPrefixes[] = {
#ifdef FORMAT_USERVER_PREFIX
    FORMAT_USERVER_STRINGIZE(FORMAT_USERVER_PREFIX),
#endif
#ifdef FORMAT_USERVER_SOURCE_PREFIX
    FORMAT_USERVER_STRINGIZE(FORMAT_USERVER_SOURCE_PREFIX)
#endif
};

void ConditionalParameters(
    int first,
#ifdef FORMAT_USERVER_EXTRA_PARAMETER
    std::string_view label,
#endif
    int last
);

template <
    typename Value,
#if FORMAT_USERVER_EXTRA_TEMPLATE_PARAMETER
    typename Allocator,
#endif
    typename Result
> struct ConditionalTemplateParameters {};

int ConditionalSubscript(int (&matrix)[2][2], int row) {
    return matrix[
#if FORMAT_USERVER_SECOND_ROW
        1
#else
        row
#endif
    ][0];
}

enum class ConditionalEnum {
    kOne,
#if FORMAT_USERVER_EXTRA_ENUM
    kTwo,
#endif
    kThree,
};

std::vector<std::string> ConditionalBracedListItems() {
    std::vector<std::string> list{
        "one",
        "two",
#if MORE
        "three",
#endif
#if EVEN_MORE
        "four"
#endif
    };
    return list;
}

void ConditionalArgumentFragment() {
    Use(
#ifdef FORMAT_USERVER_FAST_ARGUMENT
        FastArgument(),
#else
        SlowArgument(),
#endif
        "argument label"
    );
}

auto PreprocessorSelectedListItem() {
    return TimestampToJsonFailureTestParam{
        TimestampMessageData{0, kMaxTimestampNanos + 1},
        PrintErrorCode::kInvalidValue,
        "field1",
        {},
#if FORMAT_USERVER_PROTOBUF_GE_6033000
        false
#else
        true
#endif
    };
}

namespace include_after_comment {}  // namespace include_after_comment

#include "format_userver_after_namespace.inc"

struct ConditionalMembers {
#ifdef FORMAT_USERVER_HAS_FIELD
    int value;
#else
    void Value();
#endif
#ifndef FORMAT_USERVER_DISABLE_METHOD
    void Method();
#endif
};

#if FORMAT_USERVER_HAS_DECLARATION
constexpr int kConditionalDeclaration = 1;
#else
constexpr int kConditionalDeclaration = 0;
#endif

extern "C" {
#ifndef FORMAT_USERVER_CLANG
    [[gnu::visibility("default")]] [[gnu::externally_visible]]
#endif
    int FormatUserverExternAttribute();
}

void ConditionalLocalConstQualifier() {
#if FORMAT_USERVER_OPENSSL_HAS_CONST_SIGNATURE
    const
#endif
    ASN1_BIT_STRING* signature = nullptr;
    UseSignature(signature);
}

class ConditionalStringLiteral : public utils::zstring_view {
public:
#if defined(__clang__) && __clang_major__ < 18
    // clang-16 and below lose (optimize out) the pointer to `literal` with consteval. Clang-18 is know to work
    constexpr
#else
    consteval
#endif
    ConditionalStringLiteral(const char* literal) noexcept : zstring_view{literal} {
        // data()[size()] == '\0' is guaranteed by std::string_view that calls std::strlen(literal)
    }
};

class ConditionalQueryNameLiteral : public utils::zstring_view {
public:
#if defined(__clang__) && __clang_major__ < 18
    // clang-16 and below lose (optimize out) the pointer to `literal` with consteval. Clang-18 is know to work
    constexpr
#else
    consteval
#endif
    ConditionalQueryNameLiteral(const char* literal) noexcept : zstring_view{literal} {}
};

void FormatterPreprocessorContinuationRegression(const char* base_file) {
#if defined(FORMAT_PREFIX_PATH_BASE) || defined(FORMAT_SOURCE_PATH_BASE) || \
    defined(FORMAT_BUILD_PATH_BASE)
    base_file = base_file + PathBaseSize(base_file);
#endif
}

#ifdef ENONET  // No ENONET in Mac OS
int formatterPreprocessorCommentSpacing;
#endif  // ENONET

}  // namespace format_userver_fixture

#endif  // FORMAT_IFDEF_FIXTURE_HPP
