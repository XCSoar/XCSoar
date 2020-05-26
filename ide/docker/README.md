# XCSoar Docker Image

This Docker image when built, will compile XCSoar for several targets in a clean room environment.


## Currently Supported Targets

- UNIX (linux & co)
- UNIX-SDL (to run XCSoar from within the Docker container)
- ANDROID
- PC
- KOBO
- DOCS

## Instructions for building XCSoar

The build container should be considered read only. When used as described below
the output will appear in the XCSoar project directory the same way as when
building on directly the host system.


### Build the container

Ensure your current working directory is the XCSoar project root. Then run:

```
docker build --file ide/docker/Dockerfile -t xcsoar-build ./ide/
```
This will take time and download a lot of software. You only need to run this
once as long as the XCSoar build dependencies stay the same.


### Run the container


Ensure your current working directory is the XCSoar project root.

**Interactively:**
```
docker run -it --rm -v $PWD:/opt/xcsoar:delegated xcsoar-build
```

**To build a specific target, e.g. ANDROID:**
```
docker run -it --rm -v $PWD:/opt/xcsoar:delegated xcsoar-build xcsoar-compile ANDROID
```

Note: the `delegated` volume modifier is used to speed up compilation on non Linux hosts 
(macOS and Windows), it allows for eventual consistent filesystem change propagation 
from the container to the host system.


## Instructions for running XCSoar within the Docker container

You can also use the Docker container to run the Linux build of XCSoar on a non-Linux
system. This could be helpful to perform quick tests. It will use software rendering.
Needless to say that the experience is inferior to running XCSoar on the host
system directly. However, installing all build dependencies might also be overkill
for small things.

### macOS

1. Install dependencies (socat and an X11 server) using Homebrew:
    ```shell script
    brew install socat
    brew cask install xquartz
    ```
    Please ensure to restart your Mac after you have installed XQuartz, it is needed
    for its environment to work correctly.

1. To forward X11 network traffic out of the Docker container to the host X11-Server run
`socat` on the host machine in a separate terminal (it will block):
    ```shell script
    socat TCP-LISTEN:6000,reuseaddr,fork UNIX-CLIENT:\"$DISPLAY\"
    ```

1. To compile XCSoar for this kind of usage, run:
    ```shell script
    docker run -it --rm -v $PWD:/opt/xcsoar:delegated xcsoar-build xcsoar-compile UNIX-SDL
    ```

1. After compilation you may run XCSoar if you have an X11
server installed and running and the `socat` command from above running.
    ```shell script
    docker run -it --rm -v $PWD:/opt/xcsoar:delegated xcsoar-build output/UNIX/bin/xcsoar
    ```

### Windows

*TODO*
(The process should be the same as on macOS, however, we need someone to verify
this.)
