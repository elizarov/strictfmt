namespace format_error_fixture {

void BadCallSyntax() {
    Call(;
}

void BadDeclarationSyntax() {
    int = value;
}

void ConditionalMemberContinuation() {
#if FORMAT_USERVER_HAS_STATUS_FACTORY
MakeStatus()
#else
MakeFallbackStatus()
#endif
    .WithMessage("failed");
}

void IncludeExpressionFragment() {
    int value =
#include "format_userver_value.inc"
        1;
}

}  // namespace format_error_fixture
