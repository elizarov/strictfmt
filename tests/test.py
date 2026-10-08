"""Run the formatter and lint test groups with one report."""

import unittest

from format.format_test import FormatCommandTests
from lint.lint_test import LintTests


class MethodNameTestResult(unittest.TextTestResult):
    def getDescription(self, test: unittest.case.TestCase) -> str:
        return test.id().rsplit(".", maxsplit=1)[-1]


if __name__ == "__main__":
    runner = unittest.TextTestRunner(verbosity=2, resultclass=MethodNameTestResult)
    unittest.main(testRunner=runner)
