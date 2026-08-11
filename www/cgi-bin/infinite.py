#!/usr/bin/env python3
import sys
import time

# Deliberately never sends headers or exits.
# Your webserv should detect this (via timeout) and kill the process,
# returning something like 504 Gateway Timeout instead of hanging forever.
while True:
    time.sleep(1)
