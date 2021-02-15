FROM ubuntu:20.04

RUN apt update && apt dist-upgrade -y \
    && apt-get install -y libssl1.1 openssl \
    && mkdir -p /opt/fbmuck-base \
    && mkdir -p /opt/fbmuck-ssl

# Copy binaries into /usr/bin
COPY src/fbmuck src/fb-resolver scripts/docker-entrypoint.sh /usr/bin/

RUN chmod a+rx /usr/bin/docker-entrypoint.sh

# Copy FB base into /opt/fbmuck-base in case we need to start a blank DB.
COPY game/ dbs/starterdb/ /opt/fbmuck-base/

# Rename the base db to the right db file name
RUN mv /opt/fbmuck-base/data/starterdb.db /opt/fbmuck-base/data/std-db.db

ENTRYPOINT /usr/bin/docker-entrypoint.sh
