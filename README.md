Fuzzball MUCK [![GNU/Linux Build Status][ci-linux-badge]](https://github.com/fuzzball-muck/fuzzball/actions/workflows/main.yml) [![Windows Build Status][ci-windows-badge]](https://ci.appveyor.com/project/fuzzball-muck/fuzzball/branch/master) [![FreeBSD Build Status][ci-freebsd-badge]](https://cirrus-ci.com/github/fuzzball-muck/fuzzball)
===============

Fuzzball is a [MUCK](https://en.wikipedia.org/wiki/MUCK) server, an online text-based multi-user chat and roleplaying game.  Multiple people may join, set up characters, talk, and interact, exploring and building a common world.  Nearly every aspect of the game can be customized via [MPI-driven descriptions](https://www.fuzzball.org/docs/mpihelp.html) and [MUF programs](https://www.fuzzball.org/docs/mufman.html).

## Documentation

* [Fuzzball website](https://www.fuzzball.org/)
* [GitHub repository](https://github.com/fuzzball-muck/fuzzball/)
* [A live version of the docs on GitHub](https://fuzzball-muck.github.io/fuzzball/)
* [User commands](https://www.fuzzball.org/docs/muckhelp.html)
* [MPI functions](https://www.fuzzball.org/docs/mpihelp.html)
* [MUF reference](https://www.fuzzball.org/docs/mufman.html)
* [MINK - The Muck Information Kiosk](https://fuzzball-muck.github.io/muckman/)

Fuzzball also offers a built-in help system.  Connect, then type ```help``` for guidance.

## Downloads
* **Stable**
  * *Tested and supported, recommended for everyday usage*
  * **Unix**: download from [the Fuzzball homepage](https://www.fuzzball.org/), follow the steps in [the Unix README](README_UNIX.md#building), skipping *Get Source*
  * **Windows**: download from [the Fuzzball homepage](https://www.fuzzball.org/), follow the steps under *Running* in [the Windows README](README_WINDOWS.md#running)
* **Development**
  * *Try out the latest features and help find issues, but if it breaks you get to keep all the pieces*
  * **Unix**: follow the steps in [the Unix README](README_UNIX.md#building).
  * **Windows**: download a pre-built package from [the Appveyor build artifacts page](https://ci.appveyor.com/project/fuzzball-muck/fuzzball/branch/master/artifacts), follow the steps under *Running* in [the Windows README](README_WINDOWS.md#running).

### Building and Running from Source
* [Unix README](README_UNIX.md#building)
* [Windows README](README_WINDOWS.md#building)

## Usage

For a comprehensive guide to running a muck, check out [MINK - The Muck Information Kiosk](https://fuzzball-muck.github.io/muckman/).

If you only need a quick test environment, read the instructions for your operating system in [Building and Running from Source](#building-and-running-from-source) right above.

## Feedback

You can send bug reports, feature requests, or other general feedback to <a href='mailto:feedback@fuzzball.org'>feedback@fuzzball.org</a>

[ci-linux-badge]: https://github.com/fuzzball-muck/fuzzball/actions/workflows/main.yml/badge.svg
[ci-windows-badge]: https://ci.appveyor.com/api/projects/status/ktwrfcsjbv4xt3op/branch/master?svg=true&passingText=Windows%3A%20passing&pendingText=Windows%3A%20pending&failedText=Windows%3A%20failing
[ci-freebsd-badge]: https://api.cirrus-ci.com/github/fuzzball-muck/fuzzball.svg
