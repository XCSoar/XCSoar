# XCSoar Flatpak

The Flatpak packages the existing native Linux Wayland build
(`TARGET=WAYLAND`); it is not a separate XCSoar compiler target. It is
intended for glibc- and musl-based distributions, including postmarketOS and
Alpine, provided Flatpak is available.

The bundle has not yet been run on a Linux phone or other target device. The
build integration has been exercised, but runtime validation needs suitable
hardware.

`FLATPAK_APP_ID` defaults to `org.xcsoar.XCSoar`. It is the persistent
installation and update identity, so official releases must keep that value.
Downstream forks can use their own identity, for example:

```sh
make flatpak FLATPAK_APP_ID=org.example.XCSoar
```

For a local development build from a Git checkout, install Flatpak and
`flatpak-builder`, add the Flathub remote, then run this from the repository
root:

```sh
flatpak remote-add --if-not-exists flathub \
  https://dl.flathub.org/repo/flathub.flatpakrepo
make flatpak
```

`make flatpak` obtains the required Flatpak SDK from `FLATPAK_REMOTE`
(`flathub` by default), then creates a portable test bundle in
`output/UNIX/flatpak`:

```sh
make flatpak
```

If the remote has a different name, use e.g.
`make flatpak FLATPAK_REMOTE=my-remote`.

## Apple Container

Flatpak uses Bubblewrap to create a nested mount namespace during the build.
Apple Container does not grant that capability by default. Start the Linux
container with `CAP_SYS_ADMIN` (not `ALL`) before running `make flatpak`:

```sh
container run --name xcsoar-flatpak --interactive --tty --cap-add SYS_ADMIN \
  --volume "$PWD:/workspace" --workdir /workspace debian:trixie bash
```

Install the required tools in that container, add Flathub, and run the normal
commands above. Omitting `--rm` lets the container retain the downloaded
Flatpak SDK between builds. The Make target already disables `rofiles-fuse`,
which is also unavailable in nested Apple containers.

Use `make flatpak-clean` to discard its cached build directory and bundle.

The CI workflow builds native `x86_64` and `aarch64` bundles and uploads each
as a GitHub Actions artifact.  A published Flatpak repository or Flathub
submission can be added later without changing the application build.
