Fuzzball MUCK [![GNU/Linux Build Status](https://travis-ci.org/fuzzball-muck/fuzzball.svg?branch=master)](https://travis-ci.org/fuzzball-muck/fuzzball) [![Windows Build Status](https://ci.appveyor.com/api/projects/status/ktwrfcsjbv4xt3op/branch/master?svg=true)](https://ci.appveyor.com/project/fuzzball-muck/fuzzball/branch/master) [![FreeBSD Build Status](https://api.cirrus-ci.com/github/fuzzball-muck/fuzzball.svg)](https://cirrus-ci.com/github/fuzzball-muck/fuzzball)
===============

Fuzzball is a [MUCK][wiki-muck] server, an online text-based multi-user chat and roleplaying game.  Multiple people may join, set up characters, talk, and interact, exploring and building a common world.  Nearly every aspect of the game can be customized via [MPI-driven descriptions][help-mpi] and [MUF programs][help-muf].

## Documentation

* [User commands](https://www.fuzzball.org/docs/muckhelp.html)
* [MPI functions][help-mpi]
* [MUF reference][help-muf]
* [MINK - The Muck Information Kiosk][help-mink]
* [Fuzzball website][web-home]
* [A live version of the docs on GitHub](https://fuzzball-muck.github.io/fuzzball/)

Fuzzball also offers a built-in help system.  Connect, then type ```help``` for guidance.

## Downloads
* **Stable**
  * *Tested and supported, recommended for everyday usage*
  * **Unix**: download from [the Fuzzball homepage][web-home], follow the steps in [the Unix README][docs-buildsrc-nix], skipping *Get Source*
  * **Windows**: download from [the Fuzzball homepage][web-home], follow the steps under *Running* in [the Windows README](README_WINDOWS.md#running)
* **Development**
  * *Try out the latest features and help find issues, but if it breaks you get to keep all the pieces*
  * **Unix**: follow the steps in [the Unix README][docs-buildsrc-nix].
  * **Windows**: download a pre-built package from [the Appveyor build artifacts page](https://ci.appveyor.com/project/fuzzball-muck/fuzzball/branch/master/artifacts), follow the steps under *Running* in [the Windows README](README_WINDOWS.md#running).

### Building and Running from Source
* [Unix README][docs-buildsrc-nix]
* [Windows README](README_WINDOWS.md#building)

## Usage

For a comprehensive guide to running a muck, check out [MINK - The Muck Information Kiosk][help-mink].

If you only need a quick test environment, read the instructions for your operating system in [Building and Running from Source][docs-buildsrc] right above.

[docs-buildsrc]: #building-and-running-from-source
[docs-buildsrc-nix]: README_UNIX.md#building
[help-mpi]: https://www.fuzzball.org/docs/mpihelp.html
[help-muf]: https://www.fuzzball.org/docs/mufman.html
[help-mink]: http://www.rdwarf.com/users/mink/muckman/
[web-home]: https://www.fuzzball.org
[wiki-muck]: https://en.wikipedia.org/wiki/MUCK
