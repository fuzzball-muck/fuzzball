Fuzzball MUCK [![Build Status](https://travis-ci.org/fuzzball-muck/fuzzball.svg?branch=master)](https://travis-ci.org/fuzzball-muck/fuzzball)
===============

Fuzzball is a [MUCK][wiki-muck] server, an online text-based multi-user chat and roleplaying game.  Multiple people may join, set up characters, talk, and interact, exploring and building a common world.  Nearly every aspect of the game can be customized via [MPI-driven descriptions][help-mpi] and [MUF programs][help-muf].

This repository represents the ongoing development of the Fuzzball MUCK server software and related functionality.  More to come.

## Documentation

* [User commands](https://www.fuzzball.org/docs/muckhelp.html)
* [MPI functions][help-mpi]
* [MUF reference][help-muf]
* [MINK - The Muck Information Kiosk][help-mink]
* [Fuzzball website](https://www.fuzzball.org/)

Fuzzball also offers a built-in help system.  Connect, then type ```help``` for assistance.

## Building
Tools needed:
* Make, a C compiler, and friends
* Optionally, an SSL library, e.g. [OpenSSL](https://openssl.org/)

For an Ubuntu system, apt-get install the following packages
```sh
make gcc		# Make tools, compiler
libssl-dev		# SSL library headers
```

### Get source
* First time
```sh
cd ~          # Or any other desired directory
git clone     https://github.com/fuzzball-muck/fuzzball.git
cd fuzzball
```
* To update
```sh
cd ~/fuzzball  # Same path as above, plus the 'fuzzball' directory
git pull
```
### Configure
```sh
./configure --with-ssl --enable-ipv6 # Or choose your own options
```
* See ```./configure --help```
* Options may include ```--with-ssl```, ```--enable-ipv6```, etc
* When testing, use a different ```--prefix```, e.g. ```./configure --prefix="$HOME/fuzzball-test"```

### Build
```sh
make clean && make
make cert                # Skip if SSL is not enabled, or you have your own certificate
make install
make install-sysv-inits  # Skip to not run at startup
```

## Usage

For a more comprehensive guide, check out [MINK - The Muck Information Kiosk][help-mink]

### Quick-start
* **Follow the build directions above**
* Copy the relevant databases (*alternatively, use ```fuzzball/scripts/fbmuck-add```*)
```sh
FB_DIR="$(pwd)"   # Directory containing Fuzzball
PREFIX=""         # If installing to a different prefix, set it here
mkdir --parents              "$PREFIX/var/fbmuck"                          # Create data directory
cp -r "$FB_DIR/game/."       "$PREFIX/var/fbmuck"                          # Copy game information
cp -r "$FB_DIR/dbs/basedb/." "$PREFIX/var/fbmuck"                          # Copy database
mv "$PREFIX/var/fbmuck/data/basedb.db" "$PREFIX/var/fbmuck/data/std-db.db" # Rename database to standard
```
* Start Fuzzball
```sh
/usr/share/fbmuck/restart-script
# If using a custom prefix, $PREFIX/share/fbmuck/restart-script
```

[wiki-muck]: https://en.wikipedia.org/wiki/MUCK
[help-mpi]: https://www.fuzzball.org/docs/mpihelp.html
[help-muf]: https://www.fuzzball.org/docs/mufman.html
[help-mink]: http://www.rdwarf.com/users/mink/muckman/
