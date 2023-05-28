FROM ubuntu:20.04
RUN apt update && apt dist-upgrade -y
RUN apt-get install -y build-essential \
      libpcre3-dev libssl-dev git autoconf \
      automake autoconf-archive
COPY . fuzzball/
RUN cd fuzzball && \
    ./configure --with-ssl --prefix /root/scratch && make clean && \
    make && make install

FROM ubuntu:20.04
RUN apt update && apt dist-upgrade -y \
    && apt-get install -y libssl1.1 openssl \
    && mkdir -p /opt/fbmuck-base \
    && mkdir -p /opt/fbmuck-ssl

COPY --from=0 /root/scratch /usr

# Copy docker-entrypoint.sh into /usr/bin
COPY scripts/docker-entrypoint.sh /usr/bin/

# It will be looking for globals in /root/scratch/share because of how
# we are building.  The link will fix it so we're looking in the right
# spot.
RUN chmod a+rx /usr/bin/docker-entrypoint.sh && ln -s /usr /root/scratch

# Copy FB base into /opt/fbmuck-base in case we need to start a blank DB
COPY game/ dbs/starterdb/ /opt/fbmuck-base/
# Additionally, copy files in docs into the fbmuck-base/data directory
# so users have helpful things like help.txt
COPY docs/ /opt/fbmuck-base/data

# Rename the base db to the right db file name
RUN mv /opt/fbmuck-base/data/starterdb.db /opt/fbmuck-base/data/std-db.db

# Uncomment this before building if you want the database saved upon
# process termination
#STOPSIGNAL SIGUSR2

ENTRYPOINT ["bash", "/usr/bin/docker-entrypoint.sh"]
