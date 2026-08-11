#!/usr/bin/env python3
# Tests CGI-issued redirects. A CGI can respond with just a Location
# header (no Content-Type/body needed) and the server is expected to
# turn this into a proper 302 response to the client.
print("Status: 302 Found")
print("Location: /index.html")
print()
