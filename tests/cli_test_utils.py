"""Portable UTF-8 text transport for command-line tests."""

import subprocess


def run_text_command(command, *, input_text=None, cwd=None, timeout=None):
    # Binary pipes preserve explicit CR/LF bytes instead of translating them on Windows.
    result = subprocess.run(
        command,
        input=None if input_text is None else input_text.encode("utf-8"),
        cwd=cwd,
        timeout=timeout,
        check=False,
        capture_output=True,
    )
    return subprocess.CompletedProcess(
        result.args,
        result.returncode,
        result.stdout.decode("utf-8").replace("\r\n", "\n").replace("\r", "\n"),
        result.stderr.decode("utf-8").replace("\r\n", "\n").replace("\r", "\n"),
    )
