A lot has changed since the last native windows build, particularly with the server code itself.
It now supports TLS, write behind saving and deltas, and a whole bunch of other features. Most
of these have been enabled in the server itself for the native windows compile, and a lot of
stuff has been fixed since the last build. If you want to compile this code for windows, you'll
need a few things:

  - The OpenSSL Library (if you want to enable SSL support)
  - Some sort of win32 c++ compiler

First, install the OpenSSL suite if you're going to use it. Second, edit the Makefile.win file
in this directory. About 15 lines down, change the OPENSSLDIR define to represent where you
installed the OpenSSL suite. Next, run precompile.bat from this directory. This will create
a few folders, rename some files, and set things up for compilation. Next, make sure your
environment variables for your compiler are set correctly. Visual Studio has a file called
VCVARS32.BAT that contains all of the relevant setup necessary. Finally, run NMAKE and if
everything goes right, you should have a working executable in the game directory, as well
as a restart.exe file. To create a restart.ini file, just run restart with the -c option
and it will generate one for you. If you compiled with the SSL option enabled, you'll need
to generate a keyfile/certificate for your server.  letsencrypt is recommended for this.
For more information on how to generate this file, see the OpenSSL
documentation, or e-mail for assistance. If you have specific questions about this variant 
of the code feel free to email Keet Fox using the following address: winfuzz at foxpaws.net.
