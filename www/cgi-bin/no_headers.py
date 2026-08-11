#!/usr/bin/env python3
# Deliberately malformed: no "Content-Type: ...\n\n" header block,
# just straight to body. Tests whether your webserv detects the
# missing header terminator and responds with an error (or handles
# it gracefully) instead of hanging or sending a broken response.
print("This CGI forgot to send headers.")
