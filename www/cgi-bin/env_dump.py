#!/usr/bin/env python3
import os
import sys

body = ""
try:
    length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
    if length > 0:
        body = sys.stdin.read(length)
except (ValueError, Exception):
    pass

print("Content-Type: text/plain")
print()
print("=== ENV DUMP (python) ===")
for key in sorted(os.environ.keys()):
    print(f"{key}={os.environ[key]}")
print()
print("=== BODY ===")
print(body)
