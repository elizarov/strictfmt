// Unsupported-shape fixture.
//
// These sections parse through tree-sitter recovery, but their syntactic shape is
// not supported by strictfmt. This fixture records current behavior only. Stable
// indentation and spacing are not guaranteed for these cases.

namespace format_unsupported_fixture {

void ExpressionFragment() {
    constexpr int kOptmask = ARES_OPT_FLAGS | ARES_OPT_TIMEOUTMS | ARES_OPT_TRIES | ARES_OPT_DOMAINS |
#if ARES_VERSION < 0x011400
        ARES_OPT_SOCK_STATE_CB |
#endif
    ARES_OPT_LOOKUPS;
}

extern int* ConditionalDeclarationSuffix(void)
#ifdef FORMAT_USERVER_THROW
    FORMAT_USERVER_THROW
#endif
;

void PreprocessorSelectedIfHeader(Connection* conn) {
#if FORMAT_USERVER_PIPELINE_STATUS
if (conn->pipelineStatus == kPipelineOff)
#else
if (Flush(conn) < 0)
#endif
    goto sendFailed;
    sendFailed:;
}

void ConditionalArgumentExpressionFragment() {
    Open(
#ifdef FORMAT_USERVER_FLAG_A
        kFlagA |
#endif
#ifdef FORMAT_USERVER_FLAG_B
        kFlagB |
#endif
        kBaseFlag
    );
}

void PreprocessorEndedConsequence(Status status, Handle& handle, Handle next_handle) {
#if FORMAT_USERVER_HAS_PIPELINING
if (status == Status::kSync) {
HandlePipelineSync();
} else if (status != Status::kAborted)
#endif
    handle = std::move(next_handle);
}

const char* ConditionalStringLiteral() {
    return "prefix "
#if FORMAT_USERVER_USE_UTC
        "UTC "
#else
        "GMT "
#endif
    "suffix";
}

}  // namespace format_unsupported_fixture
