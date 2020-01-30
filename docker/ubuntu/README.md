# GekkoFyre/Ubuntu

### About

This docker image was created for the [Small World Deluxe](https://git.gekkofyre.io/amateur-radio/small-world-deluxe) project, in order to assist with Continuous Integration (CI/CD) via [GitLab Runner](https://docs.gitlab.com/runner/). The following, x86_64 development libraries are installed onto a standard [Ubuntu Docker image](https://hub.docker.com/_/ubuntu):

- `CMake`
- `Boost C++` Libraries
- `ZSTD` Library
- `LZ4` Library
- `Zipper` Library
- `ZLIB`
- `HamLib`
- `ICU` Libraries
- `Curl` Libraries
- `GFlags` Libraries
- `MinGW` Tools
- `Qt5` Libraries
- `Git` Toolset
- `LevelDB` Database Libraries

There might be a few others that are not listed here, particularly dependencies which are required for compilation of C/C++ development files such as the [GCC toolchain](https://gcc.gnu.org/). But please forward any questions and/or comments towards the [Small World Deluxe](https://git.gekkofyre.io/amateur-radio/small-world-deluxe) project at our [GitLab homepage](https://git.gekkofyre.io/) which is hosted by [GekkoFyre Networks](https://gekkofyre.io/), thanks :)

#### Extra Notes

If you wish to view the `Dockerfile` used to generate the image(s) in its entirety, you may do so [by clicking here](https://git.gekkofyre.io/amateur-radio/small-world-deluxe/tree/develop/docker/ubuntu/). We at [GekkoFyre Networks](https://gekkofyre.io/) believe in transparency for both our clients and users of any of our services.

The MinGW image we have created in this instance is based off a specialized, Ubuntu image made by the author, [mmozeiko](https://hub.docker.com/r/mmozeiko/mingw-w64). We'd like to extend our thanks towards them for providing their content to the greater public, as it's especially difficult to find cross-compilation resources when going from Linux -> Microsoft Windows. Or at least, it is in our experience.

#### Tags

|   Unique Tag    | Alternate #1 | Alternate #2 |
| :-------------: | :----------: | :----------: |
|    `latest`     |   `x86_64`   |    `N/A`     |
| `mingw_dekoder` | `mingw-w64`  |    `N/A`     |

#### Author(s)

This Docker image was created by the following individuals:

[Phobos Aryn'dythyrn D'thorga](https://drake.network/@phobos_dthorga) (phobos[dot]gekko[at]gekkofyre[dot]io)

------

Copyright © 2006 to 2019 – GekkoFyre Networks, All Rights Reserved.