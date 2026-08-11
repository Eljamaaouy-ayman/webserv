#!/usr/bin/env python3
import time
import sys

# Sleep duration passed as ?seconds=N in QUERY_STRING, default 3
import os
qs = os.environ.get("QUERY_STRING", "")
seconds = 3
for pair in qs.split("&"):
    if pair.startswith("seconds="):
        try:
            seconds = int(pair.split("=", 1)[1])
        except ValueError:
            pass

time.sleep(seconds)

print("Content-Type: text/plain")
print()
print(f"Slept for {seconds} seconds")
