"""CLI contracts and golden diagnostics for naming rules."""
import os
import shlex
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROJECT = Path(os.environ.get('STRICTFMT_PROJECT_ROOT', ROOT.parents[1]))
EXE = [os.environ.get('STRICTFMT_EXE', str(PROJECT / 'build/strictfmt'))]
EXE += shlex.split(os.environ.get('STRICTFMT_EXE_ARGS', ''))


class LintTests(unittest.TestCase):
    def run_tool(self, *args, text=None):
        return subprocess.run(EXE + list(args), input=text, encoding="utf-8", text=True, capture_output=True)

    def test_golden(self):
        result = self.run_tool('--lint-only', str(ROOT / 'input.cpp'))
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertEqual(result.stderr.replace(str(ROOT / 'input.cpp'), 'input.cpp'), (ROOT / 'output.txt').read_text(encoding="utf-8"))
        self.assertNotIn('namespace BadNamespace', result.stdout)

    def test_ignored_regexp_exceptions_inherit_replace_and_clear(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            parent = root / '.cpp-format'
            child = root / 'child'
            child.mkdir()
            config = child / '.cpp-format'
            parent.write_text("Lint:\n  Naming:\n    Objects:\n      Kinds: Variable\n      Case: lower_case\n"
                              "      IgnoredRegexp: 'LegacyName'\n    Constants:\n      Kinds: EnumConstant\n"
                              "      Case: CamelCase\n      Prefix: k\n", encoding="utf-8")
            source = 'int LegacyName;\nint _;\nint exс;\nint _name;\n'
            args = ['--stdin', '--lint-only', '--stdin-filename', str(child / 'input.cpp')]
            for override, rejected in [(None, ['_', 'exс', '_name']),
                                       ('LegacyName|_', ['exс', '_name']),
                                       ('_', ['LegacyName', 'exс', '_name']),
                                       ('', ['LegacyName', '_', 'exс', '_name'])]:
                with self.subTest(override=override):
                    config.write_text('Inherit: Parent\n' + ('' if override is None else
                                      f"Lint:\n  Naming:\n    Objects:\n      IgnoredRegexp: '{override}'\n"), encoding="utf-8")
                    result = self.run_tool(*args, text=source)
                    self.assertEqual(result.returncode, 1, result.stderr)
                    self.assertEqual(result.stderr.count('error:'), len(rejected), result.stderr)
                    for name in rejected:
                        self.assertIn(f"Variable '{name}'", result.stderr)
            config.write_text("Inherit: Parent\nLint:\n  Naming:\n    Objects:\n      IgnoredRegexp: 'LegacyName|_'\n"
                              "    Constants:\n      IgnoredRegexp: 'k[0-9][a-zA-Z0-9]*'\n", encoding="utf-8")
            result = self.run_tool(*args, text='enum class Good { k400, k3dsUrl, k3DSUrl, k3ds_url, k3с }; int k3dsUrl;')
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertEqual(result.stderr.count('error:'), 3, result.stderr)
            self.assertIn("Variable 'k3dsUrl'", result.stderr)
            self.assertIn("EnumConstant 'k3ds_url'", result.stderr)
            self.assertIn("EnumConstant 'k3с'", result.stderr)
            parent_result = self.run_tool('--stdin', '--lint-only', '--stdin-filename', str(root / 'input.cpp'),
                                          text='int _; enum class Good { k400 };')
            self.assertEqual(parent_result.stderr.count('error:'), 2, parent_result.stderr)

    def test_affixes_leave_nonempty_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            config = Path(tmp) / '.cpp-format'
            source = 'struct Fields { int _; int __; int good_; };'
            for case in ['', 'aNy_CasE', 'lower_case']:
                for regex in ['', '_']:
                    with self.subTest(case=case, regex=regex):
                        config.write_text('Lint:\n  Naming:\n    Fields:\n      Kinds: Field\n'
                                          "      Suffix: '_'\n" + (f'      Case: {case}\n' if case else '') +
                                          f"      IgnoredRegexp: '{regex}'\n", encoding="utf-8")
                        result = self.run_tool('--stdin', '--lint-only', '--style', str(config), text=source)
                        count = int(not regex) + int(case == 'lower_case')
                        self.assertEqual(result.returncode, int(count != 0), result.stderr)
                        self.assertEqual(result.stderr.count('error:'), count, result.stderr)

    def test_default_and_opt_out(self):
        args = ['--stdin', '--style', str(ROOT / '.cpp-format')]
        failed = self.run_tool(*args, text='int BadName;')
        self.assertEqual(failed.returncode, 1)
        self.assertIn("Variable 'BadName'", failed.stderr)
        self.assertEqual(failed.stdout, '')
        allowed = self.run_tool(*args, '--no-lint', '--validate', text='int BadName;')
        self.assertEqual(allowed.returncode, 0, allowed.stderr)
        self.assertEqual(allowed.stdout, 'int BadName;\n')

    def test_no_config_skips_lint(self):
        with tempfile.TemporaryDirectory() as tmp:
            config = Path(tmp) / '.cpp-format'
            config.write_text('', encoding="utf-8")
            result = self.run_tool('--stdin', '--lint-only', '--style', str(config), text='invalid C++ @@@')
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout, '')
            self.assertEqual(result.stderr, '')

    def test_inheritance_and_disabling(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / '.cpp-format').write_text((ROOT / '.cpp-format').read_text(encoding="utf-8"), encoding="utf-8")
            child = root / 'child'
            child.mkdir()
            config = child / '.cpp-format'
            config.write_text("Inherit: Parent\nLint:\n  Naming:\n    Objects:\n      Case: CamelCase\n", encoding="utf-8")
            args = ['--stdin', '--lint-only', '--stdin-filename', str(child / 'a.cpp')]
            result = self.run_tool(*args, text='int BadName;')
            self.assertEqual(result.returncode, 0, result.stderr)
            result = self.run_tool(*args, text='int good_name;')
            self.assertEqual(result.returncode, 1, result.stderr)
            config.write_text('Inherit: Parent\nLint:\n  Enabled: false\n', encoding="utf-8")
            self.assertEqual(self.run_tool(*args, text='int BadName;').returncode, 0)

    def test_bad_options(self):
        for extra in ['--no-lint', '--validate', '-i', '-n', '--diff', '--dump-syntax-tree']:
            with self.subTest(extra=extra):
                result = self.run_tool('--lint-only', extra, '--stdin', text='')
                self.assertEqual(result.returncode, 2, result.stderr)
        with tempfile.TemporaryDirectory() as tmp:
            config = Path(tmp) / '.cpp-format'
            for option in ['Unknown: true', 'Case: snake_case', 'IgnoredRegexp: "["', 'Kinds: Banana']:
                config.write_text('Lint:\n  Naming:\n    Rule:\n      Kinds: Variable\n      ' + option + '\n', encoding="utf-8")
                result = self.run_tool('--stdin', '--style', str(config), text='int good;')
                self.assertEqual(result.returncode, 2, result.stderr)

    def test_invalid_config_structure(self):
        examples = [
            "Lint: false\n",
            "Lint:\n  Enabled: true\n  Enabled: false\n",
            "Lint:\nLint:\n",
            "Lint:\n  Naming:\n  Naming:\n",
            "Lint:\n  Naming:\n    Rule:\n      Kinds: Variable\n    Rule:\n      Kinds: Variable\n",
            "Lint:\n  Naming:\n    Rule:\n      Case: lower_case\n",
            "Lint:\n  Naming:\n    Rule:\n      Kinds: Variable\n        Case: lower_case\n",
        ]
        for key, value in [('Kinds', ''), ('Kinds', 'Variable,'), ('Scope', ''), ('Scope', 'Local,'), ('Access', 'Friends'), ('Const', 'yes')]:
            examples.append(f"Lint:\n  Naming:\n    Rule:\n      Kinds: Variable\n      {key}: {value}\n")
        with tempfile.TemporaryDirectory() as tmp:
            config = Path(tmp) / '.cpp-format'
            for text in examples:
                with self.subTest(config=text):
                    config.write_text(text, encoding="utf-8")
                    result = self.run_tool('--stdin', '--style', str(config), text='int good;')
                    self.assertEqual(result.returncode, 2, result.stderr)

    def test_orthogonal_selectors_and_names(self):
        with tempfile.TemporaryDirectory() as tmp:
            config = Path(tmp) / '.cpp-format'
            config.write_text("Lint:\n  Naming:\n    LocalStatic:\n      Kinds: Variable\n      Scope: Local\n      Static: true\n      Const: false\n      Case: UPPER_CASE\n      Prefix: p\n      Suffix: x\n    Members:\n      Kinds: Variable, Field\n      Access: Public\n      Case: camelBack\n", encoding="utf-8")
            args = ['--stdin', '--lint-only', '--style', str(config)]
            good = 'int Global; struct Good { int camelBack; static int staticBack; }; void F() { static int pGOODx; const int Whatever = 0; }'
            result = self.run_tool(*args, text=good)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = self.run_tool(*args, text='void F() { static int pBadx; int AnyName; }')
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertEqual(result.stderr.count('error:'), 1)

    def test_const_object_and_method_selectors(self):
        with tempfile.TemporaryDirectory() as tmp:
            config = Path(tmp) / '.cpp-format'
            config.write_text("Lint:\n  Naming:\n    ConstantObjects:\n      Kinds: Variable\n      Const: true\n      Case: UPPER_CASE\n    ConstMethods:\n      Kinds: Method\n      Const: true\n      Case: UPPER_CASE\n", encoding="utf-8")
            result = self.run_tool('--stdin', '--lint-only', '--style', str(config), text='constexpr const int* bad_pointer = nullptr; const int* pointee = nullptr; struct Good { int bad_method() const; const int* non_const_method(); };')
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertIn("Variable 'bad_pointer'", result.stderr)
            self.assertIn("Method 'bad_method'", result.stderr)
            self.assertEqual(result.stderr.count('error:'), 2)

    def test_variables_and_bindings_are_independent_categories(self):
        with tempfile.TemporaryDirectory() as tmp:
            config = Path(tmp) / '.cpp-format'
            text = 'void F() { int array[2]{}; auto [BadBinding, AnotherBadBinding] = array; int BadVariable; }'
            config.write_text('Lint:\n  Naming:\n    Objects:\n      Kinds: Variable\n      Case: lower_case\n', encoding="utf-8")
            args = ['--stdin', '--lint-only', '--style', str(config)]
            for kinds, binding_count, variable_count in [
                ('Variable', 0, 1), ('Binding', 2, 0), ('Variable, Binding', 2, 1),
            ]:
                with self.subTest(kinds=kinds):
                    config.write_text(f'Lint:\n  Naming:\n    Objects:\n      Kinds: {kinds}\n      Case: lower_case\n', encoding="utf-8")
                    result = self.run_tool(*args, text=text)
                    self.assertEqual(result.returncode, 1, result.stderr)
                    self.assertEqual(result.stderr.count("Binding '"), binding_count, result.stderr)
                    self.assertEqual(result.stderr.count("Variable '"), variable_count, result.stderr)
            config.write_text('Lint:\n  Naming:\n    Objects:\n      Kinds: Variable\n      Case: lower_case\n'
                              '    Bindings:\n      Kinds: Binding\n      Case: CamelCase\n', encoding="utf-8")
            result = self.run_tool(*args, text=text)
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertEqual(result.stderr.count('error:'), 1, result.stderr)
            self.assertIn("Variable 'BadVariable'", result.stderr)

    def test_locations_and_parse_failure(self):
        for newline in ['\n', '\r\n', '\r']:
            result = self.run_tool('--stdin', '--lint-only', '--style', str(ROOT / '.cpp-format'), text='// comment' + newline + 'int BadName;')
            self.assertEqual(result.returncode, 1)
            self.assertIn('<stdin>:2:5:', result.stderr)
        result = self.run_tool('--stdin', '--lint-only', '--style', str(ROOT / '.cpp-format'), text='int x = ;')
        self.assertEqual(result.returncode, 1)
        self.assertIn('parse failed', result.stderr)

    def test_batch_no_writes_on_lint_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / '.cpp-format').write_text((ROOT / '.cpp-format').read_text(encoding="utf-8"), encoding="utf-8")
            good = root / 'a.cpp'
            bad = root / 'b.cpp'
            good.write_text('int good;', encoding="utf-8")
            bad.write_text('int Bad;', encoding="utf-8")
            result = self.run_tool('-i', '-r', tmp, '--concurrency', '2')
            self.assertEqual(result.returncode, 1)
            self.assertEqual(good.read_text(encoding="utf-8"), 'int good;')
            self.assertEqual(bad.read_text(encoding="utf-8"), 'int Bad;')


if __name__ == '__main__':
    unittest.main(verbosity=2)
