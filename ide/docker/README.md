# XCSoar Docker Image

This Docker image when built, will compile XCSoar for several targets in a clean room environment.


## Currently Supported Targets

- UNIX (linux & co)
- ANDROID
- PC
- KOBO
- DOCS

## Instructions

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
