FROM ubuntu:18.04

RUN apt update && apt dist-upgrade -y && apt-get install -y libssl1.1

COPY src/fbmuck src/fb-resolver scripts/docker-entrypoint.sh /usr/bin/
RUN chmod a+rx /usr/bin/docker-entrypoint.sh
