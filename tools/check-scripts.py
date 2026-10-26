#!/usr/bin/env python3
"""Checks every shell script of Linsoft OS: bash -n and ShellCheck errors."""
import pathlib
import subprocess
import sys

root = pathlib.Path(__file__).resolve().parent.parent / "linsoft"
failed = 0
checked = 0
for path in sorted(p for p in root.rglob("*") if p.is_file()):
    try:
        head = path.open("rb").readline()
    except OSError:
        continue
    if not (head.startswith(b"#!") and (b"bash" in head or head.strip() == b"#!/bin/sh")):
        continue
    data = path.read_bytes()
    checked += 1
    if b"\r\n" in data:
        print(f"CRLF line endings: {path}")
        failed += 1
        continue
    for command in (["bash", "-n", str(path)], ["shellcheck", "-S", "error", str(path)]):
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            print(result.stdout + result.stderr)
            failed += 1
print(f"{checked} scripts checked, {failed} problems")
sys.exit(1 if failed else 0)