#include "strictfmt/cli.h"
#include "tools/format.h"

int RunStrictfmtCli(int argc, char** argv) {
    if (argc <= 0) {
        return RunFormat(0, nullptr);
    }
    return RunFormat(argc - 1, argv + 1);
}
