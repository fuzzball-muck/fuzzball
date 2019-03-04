Building and Running Fuzzball on Unix
===============
*For documentation and general help, see [the general README](README.md)*

## Building
Tools needed:
* Make, a C compiler, [PCRE library](https://pcre.org/), and friends
* Optionally, an SSL library, e.g. [OpenSSL](https://openssl.org/)
* Optionally, the Git revision control system

For an Ubuntu system, apt-get install these packages
```sh
build-essential # Make tools, compiler
libpcre3-dev    # PCRE headers
libssl-dev      # SSL library headers
git             # Git revision control system
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
* Or [download a zip archive from Github](https://github.com/fuzzball-muck/fuzzball/archive/master.zip)

### Configure
```sh
./configure --with-ssl # Or choose your own options
```
* See ```./configure --help```
  * If needed, specify SSL headers via ```--with-ssl=/path/to/dir```, or PCRE headers via ```--with-pcre=/path/to/dir```
* When testing, use a different ```--prefix```, e.g. ```./configure --prefix="$HOME/fuzzball-test"```
* If using TLS, cat your certificate ```fullchain.pem``` and key ```privkey.pem``` into ```game/data/server.pem```, or later configure ```@tune ssl_cert_file``` and ```@tune ssl_key_file```

### Build
```sh
make clean && make
make install
make install-sysv-inits  # Skip to not run at startup
```

## Running
For a comprehensive guide to running a MUCK, check out [MINK - The Muck Information Kiosk][help-mink].

### Quick-start
* **Follow the build directions above**
* Copy the relevant databases (*alternatively, use ```fuzzball/scripts/fbmuck-add```*)
```sh
FB_DIR="$(pwd)"   # Directory containing Fuzzball
PREFIX="."        # If installing to a different prefix, set it here
cp -r "$FB_DIR/game/."       "$PREFIX/game"                          # Copy game information
cp -r "$FB_DIR/dbs/basedb/." "$PREFIX/game"                          # Copy database
mv "$PREFIX/game/data/basedb.db" "$PREFIX/game/data/std-db.db" # Rename database to standard
```
* Start Fuzzball
```sh
/usr/local/sbin/fb-restart
# If using a custom prefix, $PREFIX/sbin/fb-restart
```

[help-mink]: http://www.rdwarf.com/users/mink/muckman/
