FROM ubuntu:20.04
RUN apt update && apt dist-upgrade -y
RUN apt-get install -y build-essential \
      libpcre3-dev libssl-dev git autoconf \
      automake autoconf-archive
COPY . fuzzball/
RUN cd fuzzball && \
    ./configure --with-ssl --prefix /root/scratch && make clean && \
    make && make install && cd docs && \
    ../src/prochelp ../src/mpihelp.raw mpihelp.txt mpihelp.html && \
    ../src/prochelp ../src/mufman.raw mufman.txt mufman.html && \
    ../src/prochelp ../src/muckhelp.raw muckhelp.txt muckhelp.html

FROM ubuntu:20.04
RUN apt update && apt dist-upgrade -y \
    && apt-get install -y libssl1.1 openssl \
    && mkdir -p /opt/fbmuck-base \
    && mkdir -p /opt/fbmuck-ssl
COPY --from=0 /root/scratch /usr

# Copy docker-entrypoint.sh into /usr/bin
COPY scripts/docker-entrypoint.sh /usr/bin/

RUN chmod a+rx /usr/bin/docker-entrypoint.sh

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
