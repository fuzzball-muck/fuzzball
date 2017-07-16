Building and Running Fuzzball on Windows
===============
*For documentation and general help, see [the general README](README.md)*

## Building
Tools needed:
* [Visual Studio 2015 or higher](https://www.visualstudio.com/downloads/download-visual-studio-vs)
* [Conan.io package manager](https://www.conan.io/downloads), or a manually installed SSL library, e.g. [OpenSSL for Windows](https://wiki.openssl.org/index.php/Binaries)
* Optionally, the [Git revision control system](https://git-scm.com/download/win)

### Get source
* First time
  * Open a Git Bash shell in the desired folder (*if Windows Explorer integration is active, right-click, ```Git Bash```*)
```sh
cd ~          # Or any other desired directory
git clone     https://github.com/fuzzball-muck/fuzzball.git
cd fuzzball
```
* To update
  * Open a Git Bash shell
```sh
cd ~/fuzzball  # Same path as above, plus the 'fuzzball' directory
git pull
```
* Or [download a zip archive from Github](https://github.com/fuzzball-muck/fuzzball/archive/master.zip)

### Configure
*Use the Windows command prompt instead of Git Bash*
* Set up Visual Studio environment variables
  * Use ```Program Files``` instead of ```Program Files (x86)``` if on a 32-bit machine
  * Fuzzball does not currently support compiling as 64-bit
```bat
"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86
```
* Set up dependencies with Conan.io
```bat
conan install -s arch=x86
set OPENSSLDIR=C:\path\to\fuzzball\bin
```
* *Or*, use a manually-installed version of OpenSSL
```bat
set OPENSSLDIR=C:\path\to\openssl
```

### Build
```bat
nmake /f makefile.win
```

## Running
* Install the [32-bit Visual C++ Redistributable for Visual Studio 2015][visual-runtime] (*Fuzzball currently does not support 64-bit*)
* Manage the server with ```restart.exe```

For a comprehensive guide to running a MUCK, check out [MINK - The Muck Information Kiosk][help-mink].

### Quick-start
* **Follow the build directions, or [download a pre-built package][docs-downloads]**
* Set up configuration in ```restart.ini```
```bat
cd "path\to\fuzzball\folder"
restart -c
```
* Edit and save ```restart.ini``` with your options, e.g. with ```notepad restart.ini```
* If using SSL, save your certificate and key as ```data\server.pem```, or later configure ```@tune ssl_cert_file``` and ```@tune ssl_key_file```
* Start the server
```bat
restart
```

[help-mink]: http://www.rdwarf.com/users/mink/muckman/
[docs-downloads]: README.md#downloads
[visual-runtime]: https://www.microsoft.com/en-us/download/details.aspx?id=48145
