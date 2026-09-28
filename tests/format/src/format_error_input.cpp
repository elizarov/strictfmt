namespace format_error_fixture {

void BadCallSyntax() {
    Call(;
}

void BadDeclarationSyntax() {
    int = value;
}

void IncludeExpressionFragment() {
    int value =
#include "format_userver_value.inc"
        1;
}

}  // namespace format_error_fixture
