// Unsupported-shape fixture.
//
// These sections parse through tree-sitter recovery, but their syntactic shape is
// not supported by strictfmt. This fixture records current behavior only. Stable
// indentation and spacing are not guaranteed for these cases.

namespace format_unsupported_fixture {

extern int* ConditionalDeclarationSuffix(void)
#ifdef FORMAT_USERVER_THROW
    FORMAT_USERVER_THROW
#endif
    ;

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
