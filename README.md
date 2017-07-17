Fuzzball MUCK [![Linux Build Status](https://travis-ci.org/fuzzball-muck/fuzzball.svg?branch=master)](https://travis-ci.org/fuzzball-muck/fuzzball) [![Windows Build Status](https://ci.appveyor.com/api/projects/status/ktwrfcsjbv4xt3op/branch/master?svg=true)](https://ci.appveyor.com/project/fuzzball-muck/fuzzball/branch/master)
===============

Fuzzball is a [MUCK][wiki-muck] server, an online text-based multi-user chat and roleplaying game.  Multiple people may join, set up characters, talk, and interact, exploring and building a common world.  Nearly every aspect of the game can be customized via [MPI-driven descriptions][help-mpi] and [MUF programs][help-muf].

## Documentation

* [User commands](https://www.fuzzball.org/docs/muckhelp.html)
* [MPI functions][help-mpi]
* [MUF reference][help-muf]
* [MINK - The Muck Information Kiosk][help-mink]
* [Fuzzball website][web-home]

Fuzzball also offers a built-in help system.  Connect, then type ```help``` for guidance.

## Downloads
* **Stable**
  * *Tested and supported, recommended for everyday usage*
  * **Linux**: download from [the Fuzzball homepage][web-home], follow the steps in [the Linux README][docs-buildsrc-lin], skipping *Get Source*
  * **Windows**: download from [the Fuzzball homepage][web-home], follow the steps under *Running* in [the Windows README](README_WINDOWS.md#running)
* **Development**
  * *Try out the latest features and help find issues, but if it breaks you get to keep all the pieces*
  * **Linux**: follow the steps in [the Linux README][docs-buildsrc-lin].
  * **Windows**: download a pre-built package from [the Appveyor build artifacts page](https://ci.appveyor.com/project/fuzzball-muck/fuzzball/branch/master/artifacts), follow the steps under *Running* in [the Windows README](README_WINDOWS.md#running).

### Building and Running from Source
* [Linux README][docs-buildsrc-lin]
* [Windows README](README_WINDOWS.md#building)

## Usage

For a comprehensive guide to running a muck, check out [MINK - The Muck Information Kiosk][help-mink].

If you only need a quick test environment, read the instructions for your operating system in [Building and Running from Source][docs-buildsrc] right above.

[docs-buildsrc]: #building-and-running-from-source
[docs-buildsrc-lin]: README_LINUX.md#building
[help-mpi]: https://www.fuzzball.org/docs/mpihelp.html
[help-muf]: https://www.fuzzball.org/docs/mufman.html
[help-mink]: http://www.rdwarf.com/users/mink/muckman/
[web-home]: https://www.fuzzball.org
[wiki-muck]: https://en.wikipedia.org/wiki/MUCK
