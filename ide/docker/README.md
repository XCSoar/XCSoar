# XCSoar Docker Image

This Docker Image when built, will compile XCSoar for several targets in a clean room environment.


## Currently Supported Targets

- UNIX (linux & co)
- UNIX-SDL (Software Rendering for X11 forward)
- ANDROID
- PC
- KOBO
- DOCS

## Instructions

The container itself is readonly. The build results will appear in `./output/`.

To build the container:
```
docker build \
    --file ide/docker/Dockerfile \
    -t xcsoar/xcsoar-build:latest ./ide/
```

To run the container interactivly:
```
docker run \
    --mount type=bind,source="$(pwd)",target=/opt/xcsoar \
    -it xcsoar/xcsoar-build:latest /bin/bash
```

To run the ANDROID build:
```
docker run \
    --mount type=bind,source="$(pwd)",target=/opt/xcsoar \
    -it xcsoar/xcsoar-build:latest xcsoar-compile ANDROID
```
