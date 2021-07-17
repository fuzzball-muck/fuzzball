#!/bin/bash
# Restart script for Docker
#
# Optional arguments are port numbers.
# If none are given, defaults to 4201.
#

prefix=/opt
exec_prefix=/usr

#
# In a Docker environment, the GAMEDIR should always be the value below.
# This assures that directory mapping will always point to the correct place.
#
GAMEDIR="$prefix/fbmuck"

# If you want your database to be accessible by someone other than the
# user the MUCK is running under, you should change this to the appropriate
# umask value. 077 is owner-only.
UMASK=077

#
# The following are the paths to the db files to load, save to, and archive to.
# DBOLD is the path and filename of the the previous database archive.
# DBIN is the path and filename of the database to read in.
# DBOUT is the path and filename of the database that the muck should save to.
#
# On a successful restart, DBIN is moved to DBOLD, and DBOUT is moved to DBIN,
# then the server is started.  The server will save it's db to DBOUT.
#
DBOLD="$GAMEDIR/data/std-db.old"
DBIN="$GAMEDIR/data/std-db.db"
DBOUT="$GAMEDIR/data/std-db.new"


#
# The ports that the MUCK server should listen to. This will be overridden if
# this script is called with port number arguments. If you want an SSL port,
# the MUCK must be configured with --with-ssl, make cert must have been run,
# and you must include "-sport PORTNUM" for each SSL port.
#
PORTS="-port 4201"

#
# This is the name of the fbmuck program to run.  You probably don't want to
# change this unless you renamed the server program to something other than
# fbmuck.
#
SERVER="${exec_prefix}/bin/fbmuck"


#
# This is the name of the file to log server restarts to.
#
RESTARTS_LOGFILE="$GAMEDIR/logs/restarts"


#
# This is the name of the file that the fbmuck program creates that contains
# the process ID of the server currently running in this game directory.
# You probably don't want to change this unless you change the PID_FILE
# #define in include/config.h
#
PIDFILE="$GAMEDIR/fbmuck.pid"


#
# You probably won't need to edit anything after this line.
#
#############################################################################
#############################################################################


cd $GAMEDIR
# echo "Restarting at: $(date)"

umask $UMASK


PANICDB="${DBOUT}.PANIC"
if [ -r $PANICDB ]; then
    end=$(tail -1 $PANICDB)
    if [ "x$end" = "x***END OF DUMP***" ]; then
		mv $PANICDB $DBOUT
    else
		echo "Warning: PANIC dump failed on "$(date) | mail $(whoami)
    fi
fi

# We may need to do an initial setup
if [ ! -r "$GAMEDIR/data" ]; then
    echo "Initializing your Fuzzball MUCK Base Directory"
    cp -a /opt/fbmuck-base/* "$GAMEDIR/"
fi

if [ -r $DBOUT ]; then
    mv -f $DBIN $DBOLD
    mv $DBOUT $DBIN
fi

if [ ! -r $DBIN ]; then
	echo "Hey!  The $DBIN file has to exist and be readable to restart the server!"
	echo "Restart attempt aborted."
	exit
fi

end=$(tail -1 $DBIN)
if [ "x$end" != 'x***END OF DUMP***' ]; then
	echo "WARNING!  The $DBIN file is incomplete and therefore corrupt!"
	echo "Restart attempt aborted."
	exit
fi

# We may need to set up SSL.
if [ "$USE_SSL" == "1" ]; then
    # If we're using Let's Encrypt, let's make a server PEM file.
    if [[ -f "/opt/fbmuck-ssl/privkey.pem" && -f "/opt/fbmuck-ssl/fullchain.pem" ]] ; then
        cat "/opt/fbmuck-ssl/privkey.pem" "/opt/fbmuck-ssl/fullchain.pem" > \
            "$GAMEDIR/data/server.pem"
    fi

    # Is SSL already set up?
    if [ ! -f "$GAMEDIR/data/server.pem" ]; then
        if [ -f "/opt/fbmuck-ssl/server.pem" ]; then
            cp "/opt/fbmuck-ssl/server.pem" "$GAMEDIR/data/server.pem"
        elif [ "$SELF_SIGN" == "1" ]; then
            # Generate certs
            openssl req -x509 -nodes -newkey rsa:4096 \
                    -keyout "/opt/fbmuck-ssl/privkey.pem" \
                    -out "/opt/fbmuck-ssl/fullchain.pem" \
                    -days 3650 \
                    -subj "/C=US/ST=NA/L=Fuzzball/O=Fuzzball/OU=Org/CN=muck"
            cat "/opt/fbmuck-ssl/privkey.pem" "/opt/fbmuck-ssl/fullchain.pem" > \
                "$GAMEDIR/data/server.pem"
        fi
    fi

    PORTS="$PORTS -sport 4202"
fi

mkdir -p logs

touch $RESTARTS_LOGFILE
echo "$(date +%Y-%m-%dT%H:%M:%S): $(who am i)" >> $RESTARTS_LOGFILE

# echo "Server started at: $(date)"
exec $SERVER -nodetach -gamedir $GAMEDIR -dbin $DBIN -dbout $DBOUT $PORTS
