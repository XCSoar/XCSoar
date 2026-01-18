# <img src="./Data/graphics/logo.svg" width="5%" alt="XCSoar Logo"> XCSoar

[![.github/workflows/build-native.yml](https://github.com/XCSoar/XCSoar/actions/workflows/build-native.yml/badge.svg)](https://github.com/XCSoar/XCSoar/actions/workflows/build-native.yml)
[![.github/workflows/build-container.yml](https://github.com/XCSoar/XCSoar/actions/workflows/build-container.yml/badge.svg)](https://github.com/XCSoar/XCSoar/actions/workflows/build-container.yml)
[![.github/workflows/build-docs.yml](https://github.com/XCSoar/XCSoar/actions/workflows/build-docs.yml/badge.svg)](https://github.com/XCSoar/XCSoar/actions/workflows/build-docs.yml)
[![TCHAR count](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/XCSoar/XCSoar/badges/.github/badges/tchar-count.json)](https://github.com/XCSoar/XCSoar/actions/workflows/tchar-count.yml)

XCSoar is a tactical glide computer for Android, Linux, macOS,
and Windows.

This file is aimed at developers.  Developers should [read the
developer manual](https://xcsoar.readthedocs.io/en/latest/).

Users can refer to the Users' Manual which, for the latest release, can be
downloaded via the [XCSoar home page](https://xcsoar.org/discover/manual.html).

## Getting the source

The XCSoar source code is managed with git. It can be fetched with the
following command:

```bash
git clone --recurse-submodules https://github.com/XCSoar/XCSoar
```

To update your repository, use the following command:

```bash
git pull --recurse-submodules
```

For more information, please refer to the [git
documentation](http://git-scm.com/).

## Compiling from source

Please read the current [Developer's
Manual](https://xcsoar.readthedocs.io/en/latest/build.html) for
detailed build instructions.

## Submitting patches

Patches may be submitted using the Developers' mail list or GitHub. Refer to
chapter 3 of the current Developers' Manual for details of how to write and
submit patches upstream.
