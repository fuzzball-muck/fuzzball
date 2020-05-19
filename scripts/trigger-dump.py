"""Python script to trigger a dump on a MUCK and wait for it.

Uses pyfuzzball repo here: https://github.com/tanabi/pyfuzzball

Change the password, please :)

Returns status code 0 on success, or 1 on failure.  Failure will only be
if there happens to be another dump triggered at the exact moment you're
triggering your own dump.
"""

#
# CONFIGURATION - Change these
#
HOST = 'localhost'
PORT = 4201
SSL = False
AUTHTOKEN = 'change-me-please'

#
# LEAVE THIS STUFF ALONE
#
from pyfuzzball.mcp import MCP
import sys

# Open MCP connection
m = MCP(HOST, PORT, SSL, True)

# Negotiate
m.negotiate(['org-fuzzball-dump'])

# Call dump with auth token
m.call('org-fuzzball-dump', 'dump', {
    'auth': AUTHTOKEN
})

# Process results
results = m.process()

# results[1] will have the dump output messages most likely.  You
# could process these instead of waiting for the MCP event if you
# preferred, but I wrote an MCP parser, so by golly I'm going to
# use it :)
while not results[0] and 'org-fuzzball-dump' not in results[0]:
    results = m.process()

# 'success' should be in the parameters if it worked.
if 'success' in results[0]['org-fuzzball-dump'][0]['parameters']:
    sys.exit(0)
else:
    sys.exit(1)
