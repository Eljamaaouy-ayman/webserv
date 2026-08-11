#!/usr/bin/env python3
import sys

# Simulates a crashing CGI: no output at all, non-zero exit code.
# Your webserv should catch this and return a 500 Internal Server Error,
# not hang, not crash, and not forward garbage to the client.
sys.exit(1)
