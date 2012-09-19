#! /bin/bash
# An example script for a Muck UPS warning system.
# This script requires the 'nc' netcat program.

# This should match the AUTHTOKEN used in the serverside mcp-extern.muf prog.
# No quotes, doublequotes or backslashes allowed.  This script will botch them.
AUTHTOKEN="NON-TRIVIAL_PASSWORD"

# Machine that the Muck resides on.
HOST="localhost"

# Port that the Muck is listening to
PORT="8888"


# You shouldn't need to alter these vars.
MCPAUTHSTR="$RANDOM$RANDOM$RANDOM"
MCPARGAUTHSTR="$RANDOM$RANDOM$RANDOM"

MESG=""
case $NOTIFYTYPE in
	ONBATT) MESG="
WARNING: The power at this Muck's location has gone out.  If the power outage
persists, this Muck will shut down as a safety measure in about half an hour.
This has been an automated message.
" ;;
	ONLINE) MESG="
NOTE: The power has been restored at this Muck's location so it will not
need to shut down.  This has been an automated message.
" ;;
	LOWBATT) MESG="
WARNING: This Muck will be shut down momentarily due to a power outage.
We appologize for the inconvenience.  This has been an automated message.
" ;;
	SHUTDOWN) MESG="
WARNING: This Muck will be shut down momentarily due to a power shutdown.
We appologize for the inconvenience.  This has been an automated message.
" ;;
esac

if [ "x$MESG" != "x" ]; then
	(
		echo '#$#mcp authentication-key: "'$MCPAUTHSTR'" version: "2.1" to: "2.1"'
		echo '#$#mcp-negotiate-can '$MCPAUTHSTR' package: "mcp-negotiate" min-version: "2.0" max-version: "2.0"'
		echo '#$#mcp-negotiate-can '$MCPAUTHSTR' package: "org-fuzzball-extern" min-version: "1.0" max-version: "1.0"'
		echo '#$#mcp-negotiate-end '$MCPAUTHSTR
		echo '#$#org-fuzzball-extern-wall '$MCPAUTHSTR' auth: "'$AUTHTOKEN'" mesg*: "" _data-tag: "'$MCPARGAUTHSTR'"'
		echo "$MESG" | sed 's/^/#$#* '$MCPARGAUTHSTR' mesg: /'
		echo '#$#: '$MCPARGAUTHSTR
		sleep 1
		echo 'QUIT'
	) | nc $HOST $PORT >> /dev/null
fi

