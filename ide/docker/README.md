# XCSoar Docker Image

This Docker image compiles XCSoar for several targets in a clean-room
environment. See ``doc/build.rst`` (“Using Docker”) for full instructions,
including legacy Raspberry Pi cross-compile in a privileged container.

## Currently Supported Targets (`xcsoar-compile`)

| Target | Description |
|--------|-------------|
| ``UNIX`` | Native Linux/Unix build (OpenGL, default desktop target) |
| ``UNIX-SDL`` | Software rendering via SDL (useful with X11 forwarding) |
| ``WAYLAND`` | Experimental Wayland display server build |
| ``WIN64OPENGL`` | Windows x64, OpenGL ES via ANGLE (**recommended**) |
| ``WIN32OPENGL`` | Windows 32-bit, OpenGL ES via ANGLE (**recommended**) |
| ``ANDROID`` | Android fat package (all ABIs) |
| ``KOBO`` | Kobo e-reader build plus ``KoboRoot.tgz`` |
| ``DOCS`` | Build the PDF manuals (``make manual``) |
| ``UNIX-DEBIAN`` | Build a ``.deb`` package (``dpkg-buildpackage``) |
| ``PC`` | Windows 32-bit GDI (**deprecated**; use ``WIN32OPENGL``) |
| ``WIN64`` | Windows x64 GDI (**deprecated**; use ``WIN64OPENGL``) |

Pass additional ``make`` options after the target name (e.g. ``USE_CCACHE=y
everything``).

**Platform notes:** The image targets **Linux x86_64** hosts. **iOS** builds
are not supported in Docker — use a Mac with Xcode (see ``doc/build.rst``,
*Compiling for iOS*). On **macOS**, Android via Docker can hang or fail under
emulation; prefer a native Android build or a Linux Docker host (see
``doc/build.rst``, *Using Docker*).

## Instructions

The container itself is readonly. The build results will appear in `./output/`.

To run the container interactively:
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

To cross-compile Windows (OpenGL, recommended):
```bash
docker run \
    --mount type=bind,source="$(pwd)",target=/opt/xcsoar \
    ghcr.io/xcsoar/xcsoar/xcsoar-build:latest \
    xcsoar-compile WIN64OPENGL USE_CCACHE=y everything
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
