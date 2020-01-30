[TOC]

# Small World Deluxe

### Premise for Docker containers

#### Why?

For quite a while now, [GitLab Runner](https://docs.gitlab.com/runner/) has supported the use of Docker containers within its Go-based [Continuous Integration/Development](https://www.atlassian.com/continuous-delivery/continuous-integration) (i.e. CI/CD) system. This allows for the following, among many more that have not been mentioned below:

- Allows for concurrent job execution
  - Use multiple tokens with multiple server
  - Limit number of concurrent jobs per-token
- Supports a wide variety of operating systems, all under the one system with little setup and/or knowledge required
- Works on GNU/Linux, macOS, and Windows (pretty much anywhere you can run Docker)
- Allows customization of the job running environment
- Enables caching
- And so much more!

Through the use of Docker, we can virtualize almost any operating system and environment to our exact specifications 

#### How?

Through the use of `Dockerfile`'s, we are able to create the Docker containers specifically customized to our needs so that we may make use of GitLab Runner (and thusly Continuous Integration).

Some notes to take into consideration for [Small World Deluxe](https://git.gekkofyre.io/amateur-radio/small-world-deluxe) are that we use `arch/mingw_sworld/Dockerfile` for all the MinGW related builds. This allows for Linux -> Microsoft Windows cross-compilation to happen and is the reason we even have automatically built executables for the latter platform in the first place.

The file, `arch/latest/Dockerfile`, is the base Docker container for the aforementioned MinGW build. If you wish to find out more about these two containers, then please proceed to `arch/README.md` for further details.