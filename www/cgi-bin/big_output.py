#!/usr/bin/env python3
import sys

print("Content-Type: text/plain")
print()
sys.stdout.flush()

# ~5MB of output, tests that your server correctly streams/buffers
# large CGI responses instead of truncating or blocking forever.
line = "x" * 79 + "\n"
for _ in range(65000):
    sys.stdout.write(line)
