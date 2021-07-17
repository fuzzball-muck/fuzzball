# Fuzzball in Docker

Fuzzball has a Dockerfile, which is published to Docker Hub, and it also has a docker-compose.yml file to make it simple to run your container with the proper environment variables and volumes set.

To see our Docker Hub entry, go here: https://hub.docker.com/r/fbmuck/fbmuck

## Fuzzball with docker-compose

### Building

The docker-compose version of building the container uses compose format version 3.7, thus you need a minimum Docker version of 18.06.0 or above. To build with docker-compose, execute the following in the directory where this README file appears:

```
docker-compose build
```

Fuzzball for Docker is designed to run inside an Ubuntu 20.04 container. The Docker build process is [multi-stage](https://docs.docker.com/develop/develop-images/multistage-build/), meaning it will first build an image whose purpose is solely to build the source, then create a second lean image whose purpose is solely to run it. 

## Running

Several environment variables control the behavior of Fuzzball when run with docker-compose:

* FB_PORT - This is the port we will expose for non-SSL connections.
* FB_SSL_PORT - This is the port we will expose for SSL connections.  Due to limitations in docker-compose, we will always open this port even if SSL is not being used.  You can always comment out the lines in docker-compose if this is a bother to you, or perhaps make this the same as FB_PORT (I haven't tried this).
* FB_DATA_PATH - This is the local directory that we will mount as a Docker volume and store our MUCK data.  If this directory does not exist, it will be created.  If it is empty, it will be initialized with starterdb automatically.
* FB_CERT_PATH - This is the local directory that we will mount as a Docker volume where we will load certs.  There's a lot of nuance here so read the section below on SSL.
* FB_USE_SSL - If this is '1', we will use SSL.  Any other value (or unset) will not enable SSL.
* FB_SELF_SIGN - If this is '1', we will generate self-signed certs if there are no certs already made and FB_USE_SSL is 1.  Otherwise, if FB_USE_SSL is 1 and this is unset, and there are no SSL certificates, you will get an error.

There is an .env file in this directory, which contains some defaults for these:

* FB_PORT defaults to 4201
* FB_SSL_PORT defaults to 4202
* FB_DATA_PATH defaults to ./fb-data
* FB_CERT_PATH defaults to ./fb-cert
* FB_USE_SSL defaults to 1
* FB_SELF_SIGN defaults to 1

If you just want to try out Fuzzball, using docker-compose and the default ```.env``` file is the easiest way to do so.  Once you have built the container with docker-compose, you can simply issue:

```
docker-compose up
```

You can add ```-d``` if you want to background it.  See the docker-compose documentation if you would like to pass in environment variables.

## Running Fuzzball with raw Docker

You don't have to use Docker Compose, you can use raw docker if you prefer.

### Building

Building this container with Docker is as simple as issuing the following in the directory this README file appears in:

```
docker build .
```

### Running

Port "4201" is used for non-SSL and "4202" is used for SSL, which you may expose however you wish.

The environment variable "USE_SSL" should be "1" if you wish to use SSL, and can be ignored if you do not.  "SELF_SIGN" should be "1" if you want it to generate self-signed certificates if there is not an existing certificate, leave it off if you don't want that.

If you want your MUCK and certs to persist from run to run (which you probably do), you should map volumes to:

* /opt/fbmuck
* /opt/fbmuck-ssl

The first will be your MUCK data directory, the second will be where SSL certs are placed.  For SSL notes, see the section below.  If the MUCK data volume is empty, it will be initialized with starterdb.

Here's a sample command line call to run the container without docker-compose:

```
docker run -d --init -p 4201:4201 -p 4202:4202 -e USE_SSL=1 -e SELF_SIGN=1 \
       -v "$(pwd)"/fb-data:/opt/fbmuck -v "$(pwd)"/fb-cert:/opt/fbmuck-ssl \
       fbmuck \
       /usr/bin/docker-entrypoint.sh
```

## Terminating the container

Either implementation of the running Fuzzball Docker container respect the ```fbmuck``` binary's kill signals, when run as described in this document.

Both the ```Dockerfile``` and ```docker-compose.yaml``` files contain lines which you can uncomment to leverage SIGUSR2 (if you wish the database to be saved before container shutdown). Otherwise, SIGKILL (the default) will terminate the Fuzzball process without saving.

## SSL

The way SSL is handled currently is a little rough around the edges.  We intend to evolve and refine this; input from people who use the Docker container is highly encouraged to help us figure out how to make this work optimally.


If FB_USE_SSL=1 (docker-compose) or USE_SSL=1 (plain Docker), then the container will look for ```fullchain.pem``` and ```privkey.pem``` in /opt/fbmuck-ssl -- this should be mounted as a volume if you are using straight Docker, or it will be the FB_CERT_PATH directory if you are using docker-compose.

If the files are NOT there, and FB_SELF_SIGN=1 (docker-compose) or SELF_SIGN=1 (plain docker), they will be generated with a 10 year expiration.

Then, we will look for /opt/fbmuck/data/server.pem -- /opt/fbmuck should be a volume, and if you are using docker-compose, you will find this file at "${FB_DATA_PATH}/data/server.pem".  If the server.pem file exists, we will use it as-is.  This is important -- it is used as-is, which means if you want to replace it with the certs in the cert volume, you must delete server.pem.

If server.pem does not yet exist, the ```fullchain.pem``` and ```privkey.pem``` files will be concatenated together and copied to /opt/fbmuck/data/server.pem and it is used.

### Let's Encrypt

Let's Encrypt will make fullchain.pem and privkey.pem files.  Let's Encrypt is not baked into the fbmuck Docker image because Let's Encrypt requires either a web port to be open to the internet or some DNS entries.  This is not practical to configure in a way that would be useful to all potential consumers of our Docker Container.

Thus, you have two options; you can make your own Dockerfile that uses our Dockerfile as a base and ads in the Let's Encrypt configuration, or you can have the Docker host server manage Let's Encrypt.  This section will describe the latter.

Assuming you have Let's Encrypt set up on the Docker host server, you can add a "post hook" that will replace server.pem.

Create the following script in:

/etc/letsencrypt/renewal-hooks/post/fbmuck.sh

```
#!/bin/bash

# Assumes FB_DATA_PATH is in environment -- alternatively, you can hard
# code the path to the data volume in this file.
#
# Replace YOUR_HOST_NAME with your let's encrypt host name.

cat /etc/letsencrypt/live/YOUR_HOST_NAME/privkey.pem \
    /etc/letsencrypt/live/YOUR_HOST_NAME/fullchain.pem > \
    "${FB_DATA_PATH}/data/server.pem"
```

Make it executable:

```
chmod a+rx /etc/letsencrypt/renewal-hooks/post/fbmuck.sh
```

Then, log in to your MUCK and make sure the following tune parameter is set:

```
@tune ssl_auth_reload_certs=yes
```
