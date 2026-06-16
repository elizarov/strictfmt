#include "format/strictfmt_cli.h"

#include "format/format.h"

int RunStrictfmtCli(int argc, char** argv) {
    if (argc <= 0) {
        return RunFormat(0, nullptr);
    }
    return RunFormat(argc - 1, argv + 1);
}
