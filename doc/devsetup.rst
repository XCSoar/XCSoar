###################################################
Setting up a development environment based on linux
###################################################

This describes the setup of a development environment suitable to
compile XCSoar for most supported platforms. The manual focuses on recent
releases of Debian-based flavors of GNU/Linux (including Ubuntu).

In the following instructions, ``sudo`` is used to execute commands with
root privileges. This is not enabled by default in Debian (but on some
Debian based distributions, like Ubuntu).

Download source code
====================

To download the XCSoar source code, make sure you have git installed::

 sudo apt-get update
 sudo apt-get install git

Download the source code of XCSoar by executing ``git`` in the
following way in your project directory::

 git clone --recurse-submodules https://github.com/XCSoar/XCSoar

Setting up the build environment
================================

On Debian or Ubuntu, use one of the following — in order of preference:

1. **Native build** on the host (recommended): install dependencies with
   the provisioning scripts, then run ``make``.
2. **Docker** on x86_64: use the pre-built image when you prefer not to
   install libraries on the host (see :ref:`docker-build` in :doc:`build`).
3. **Vagrant** VM: optional legacy path if you want an isolated VM with
   many targets preconfigured (see “Using a build VM with Vagrant” in
   :doc:`build`).

Native build
------------

Run the scripts from ``ide/provisioning`` to install build dependencies.
See :doc:`build` for per-target section names and compile commands.
For ``Run*`` test and debug utilities after building, see
:doc:`test_debug_utilities`::

   cd ide/provisioning
   sudo ./install-debian-packages.sh
   ./install-android-tools.sh

Docker
------

On an x86_64 Linux host, bind-mount the source tree and compile inside
the container (details in :doc:`build`)::

   docker run \
       --mount type=bind,source="$(pwd)",target=/opt/xcsoar \
       -it ghcr.io/xcsoar/xcsoar/xcsoar-build:latest /bin/bash

   xcsoar-compile UNIX USE_CCACHE=y

Vagrant
-------

See :doc:`build` for VM setup with VirtualBox and the Vagrantfile in
``ide/vagrant``.

Raspberry Pi
============

On a **Raspberry Pi 4 or 5**, install dependencies and build **natively on
the device** (``TARGET=UNIX`` is auto-detected)::

   cd ide/provisioning
   sudo ./install-debian-packages.sh UPDATE BASE LINUX LIBINPUT_GBM
   make -j$(nproc) USE_CCACHE=y

Cross-compiling for legacy Raspberry Pi 1–3
------------------------------------------

``TARGET=PI`` and ``TARGET=PI2`` are for cross-compiling on a desktop host
for old ARMv6/ARMv7 boards. Create an armhf sysroot (``setup-pi-sysroot.sh``
installs ``PI_HOST`` host tools by default)::

   cd ide/provisioning
   sudo ./setup-pi-sysroot.sh

Cross-compile (Pi 1 / ARMv6)::

   make TARGET=PI PI=/opt/pi/root

For Raspberry Pi 2/3 (ARMv7 with NEON)::

   make TARGET=PI2 PI=/opt/pi/root

Binaries are written to ``output/PI/`` and ``output/PI2/`` (not
``output/UNIX/``). See :doc:`build` for details.

The same sysroot steps work inside a privileged Docker container; see
:doc:`build` (“Using Docker” and “Cross-compiling for legacy Raspberry Pi
1–3”).

Optional: modern LaTeX editor for editing the Manual
====================================================

Most people today edit LaTeX files in specific editors, as this is much
more comfortable and efficient. This is highly recommended especially if
you are not very familiar with LaTeX: learning it is very easy with a
modern editor. Here, we install TeXstudio as an example, as it is very
widespread and supports the rather rare LuaLaTeX well.

To install, get the relevant package::

   sudo apt-get install texstudio

As the directory tree of XCSoar is very unusual for a LaTeX project,
we need to make some special configurations in order to allow for
quick compiling from within the editor, and for full synctex
functionality:

In “Options / Configure TeXStudio”, enable “show advanced options”.

In “Options / Configure TeXStudio / Commands / Commands / LuaLaTeX”,
replace::

 lualatex -synctex=1 -interaction=nonstopmode %.tex

with::

 lualatex -synctex=1 -interaction=nonstopmode \
    -output-directory=?a)../../../output/manual %.tex

In “Options / Configure TexStudio / Build / Build Options / Addition
Search Paths”: Enter in *both* fields (“Log file” and in the field
“PDF File”)::

 ../../../output/manual/

Add the following line to *both* the ``.profile`` and the ``.bashrc``
file of your user directory::

 export TEXINPUTS="..:../../../output/manual:../../../output/manual/en:../../..:"

Finally, you need to run ``make manual`` in the XCSoar base directory
at least once from the command line before you can compile from within
the TexStudio interface. This creates the path structure and generates
the figure files which are included into the manual. Of course, if you
change figures, you might have to run ``make manual`` again.

Inside TeXStudio, open the file :file:``XCSoar-manual.tex`` (or one of
the other root files) and right-click on this file to “set as explicit
root document”, in the structure view on the left. Now you are good to
go.  Make changes and press F5 to see the result immediately.
