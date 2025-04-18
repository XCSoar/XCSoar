# XCSoar Docker Image

This Docker Image when built, will compile XCSoar for several targets in a clean room environment.

## Currently Supported Targets

Targets:
  - UNIX (linux & co)
  - UNIX-SDL (Software Rendering for X11 forward)
  - ANDROID
  - PC
  - KOBO
  - DOCS

## Instructions

The container itself is readonly. The build results will appear in `./output/`.

To run the container interactivly:
```bash
docker run \
    --mount type=bind,source="$(pwd)",target=/opt/xcsoar \
    -it ghcr.io/xcsoar/xcsoar/xcsoar-build:latest /bin/bash
```

To run the ANDROID build:
```bash
docker run \
    --mount type=bind,source="$(pwd)",target=/opt/xcsoar \
    -it ghcr.io/xcsoar/xcsoar/xcsoar-build:latest xcsoar-compile ANDROID
```

To build the container:
```bash
docker build \
    --file ide/docker/Dockerfile \
    -t xcsoar/xcsoar-build:latest ./ide/
```

### Running XCSoar as a GUI application from the container

Sometimes your runtime environment diverges too far from the build environment to be able to execute the binary natively.
In this case you can start XCSoar inside the container and let it be displayed on your X11 Server:
```bash
docker run \
    --mount type=bind,source="$(pwd)",target=/opt/xcsoar \
    --volume="$HOME/.Xauthority:/root/.Xauthority:rw" \
    -v $HOME/.xcsoar/:/root/.xcsoar \
    --env="DISPLAY" --net=host \
    -it ghcr.io/xcsoar/xcsoar/xcsoar-build:latest /bin/bash
```
Compile and run the binary (UNIX-SDL target):
```bash
xcsoar-compile UNIX-SDL
./output/UNIX/bin/xcsoar
```

### Using ccache

Just add `USE_CCACHE=y` to the `xcsoar-compile` or `make` command (as you would do if compiling locally).
ccache will store its db into `./.ccache/`, so the cache will be shared across all container instances.
