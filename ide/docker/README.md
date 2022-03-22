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
### Running Docker as Non Root User
If you execute the docker image with root user id, the output directory is 
readonly for other users.. So, it's  a good idea to use another user for this.
 
If there is a user group named docker, all users of this group can execute docker images.
In 

[Docker post install] (https://docs.docker.com/engine/install/linux-postinstall/)
is described how to configure it.
```
sudo groupadd docker
sudo usermod -aG docker $USERNAME # replace $USERNAME with your user id
newgrp docker
```
or 
```
sudo setfacl --modify user:$USERNAME:rw /var/run/docker.sock  $USERNAME # replace $USERNAME with your user id
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
