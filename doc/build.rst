################
Compiling XCSoar
################

The Build System
================

A big plain :file:`Makefile` is used to control the XCSoar build.  GNU
extensions are allowed.

This chapter describes the internals of our build system.

Linker parameters
-----------------

The following variables (or variable suffixes) appear in the
``Makefile`` (conforming to ``automake`` conventions):

- ``LDFLAGS``: Linker flags, such as ``-static`` or ``-Wl,...``, but
  not ``-l``.

- ``LDLIBS``: All ``-l`` flags, e.g. ``-lGL``.

- ``LDADD``: Path names of static libraries,
  e.g. :file:`/usr/lib/libz.a`.

Search directories (``-L``) are technically linker "flags", but they
are allowed in ``LDLIBS``, too.


Compiling XCSoar on Linux
=========================

The ``make`` command is used to launch the XCSoar build process.

Most of this chapter describes how to build XCSoar on Linux, with
examples for Debian/Ubuntu. A cross-compiler is used to build binaries
for other operating systems (for example Android and Windows).


Getting the Source Code
-----------------------

The XCSoar source code is managed with `git <http://git-scm.com/>`__. It
can be downloaded with the following command::

  git clone git://github.com/XCSoar/XCSoar

To update your repository, type::

  git pull

To update third-party libraries used by XCSoar (such as `Boost
<http://www.boost.org/>`__), type::

  git submodule update --init --recursive

For more information, please read to the :program:`git` documentation.

Requirements
------------

The following is needed for all targets:

-  GNU make

-  GNU compiler collection (``gcc``), version 10 or later or clang/LLVM
   10 (with "make CLANG=y")

-  GNU gettext

-  `rsvg <http://librsvg.sourceforge.net/)>`__

-  `ImageMagick 6.4 <http://www.imagemagick.org/>`__

-  `xsltproc <http://xmlsoft.org/XSLT/xsltproc2.html>`__

-  `Info-ZIP <http://www.info-zip.org/>`__

-  Perl

-  FFmpeg

The following command installs these on Debian::

  sudo apt install make \
      librsvg2-bin xsltproc \
      imagemagick gettext ffmpeg \
      git quilt zip \
      m4 automake \
      ttf-bitstream-vera fakeroot

Target-specific Build Instructions
----------------------------------

Compiling for Linux/UNIX
~~~~~~~~~~~~~~~~~~~~~~~~

The following additional packages are needed to build for Linux and
similar operating systems:

-  `zlib <http://www.zlib.net/>`__

- `c-ares <https://c-ares.haxx.se/`__

-  `CURL <http://curl.haxx.se/>`__

-  `Lua <http://www.lua.org/>`__

-  `libinput <https://www.freedesktop.org/wiki/Software/libinput/>`__
   (not required when using Wayland or on the KOBO)

-  `SDL <http://www.libsdl.org/>`__

-  `SDL_ttf <http://www.libsdl.org/projects/SDL_ttf/>`__

-  `libpng <http://www.libpng.org/>`__

-  `libjpeg <http://libjpeg.sourceforge.net/>`__

-  OpenGL (Mesa)

-  to run XCSoar, you need one of the following fonts (Debian package):
   DejaVu (``fonts-dejavu``), Roboto (``fonts-roboto``), Droid
   (``fonts-droid``), Freefont (``fonts-freefont-ttf``)

The following command installs these on Debian::

  sudo apt install make g++  zlib1g-dev \
      libsodium-dev \
      libfreetype6-dev \
      libpng-dev libjpeg-dev \
      libtiff5-dev libgeotiff-dev \
      libc-ares-dev \
      libcurl4-openssl-dev \
      libc-ares-dev \
      liblua5.4-dev \
      libxml-parser-perl \
      libasound2-dev \
      librsvg2-bin xsltproc \
      imagemagick gettext \
      mesa-common-dev libgl1-mesa-dev libegl1-mesa-dev \
      libinput-dev \
      fonts-dejavu

To compile, run::

  make

You may specify one of the following targets with ``TARGET=x``:

========== =================================================
``UNIX``   regular build (the default setting)
``UNIX32`` generate 32 bit binary
``UNIX64`` generate 64 bit binary
``OPT``    alias for UNIX with optimisation and no debugging
========== =================================================

Compiling for Android
~~~~~~~~~~~~~~~~~~~~~

For Android, you need:

- `Android SDK level 26 <http://developer.android.com/sdk/>`__

- `Android NDK r23b <http://developer.android.com/sdk/ndk/>`__

- `Ogg Vorbis <http://www.vorbis.com/>`__

- Java JDK

On Debian::
  
  sudo apt install default-jdk-headless vorbis-tools adb

The required Android SDK components are:

- Android SDK Build-Tools 28.0.3

- SDK Platform 26

These can be installed from the Android Studio SDK Manager, or using the
SDK command line tools:

tools/bin/sdkmanager  "build-tools;28.0.3"  "platforms;android-26"

The ``Makefile`` assumes that the Android SDK is installed in
``~/opt/android-sdk-linux`` and the NDK is installed in
``~/opt/android-ndk-r23b``. You can use the options ``ANDROID_SDK`` and
``ANDROID_NDK`` to override these paths.

Load/update the IOIO source code::

  git submodule update --init --recursive

To compile, run::

  make TARGET=ANDROID

Use one of the following targets:

.. list-table::
 :widths: 20 80
 :header-rows: 1

 * - Name
   - Description
 * - ``ANDROID``
   - for ARM CPUs (same as ``ANDROID7``)
 * - ``ANDROID7``
   - for ARMv7 CPUs (32 bit)
 * - ``ANDROIDAARCH64``
   - for 64 bit ARM CPUs
 * - ``ANDROID86``
   - for x86-32 CPUs
 * - ``ANDROIDX64``
   - for x86-64 CPUs
 * - ``ANDROIDFAT``
   - "fat" package for all supported CPUs

Compiling for Windows
~~~~~~~~~~~~~~~~~~~~~

To cross-compile to (desktop) Windows, you need
`Mingw-w64 <http://mingw-w64.org>`__.

The following command installs it on Debian::

  sudo apt install g++-mingw-w64

To compile for 32 bit Windows, run::

  make TARGET=PC

Use one of the following targets:

========= ============================
``PC``    32 bit Windows (i686)
``WIN64`` Windows x64 (amd64 / x86-64)
========= ============================

Compiling for iOS and macOS
~~~~~~~~~~~~~~~~~~~~~~~~~~~

On macOS, the following tools are required:

- png2icns from `libicns <http://icns.sourceforge.net>`__ to build for
  macOS

- `dpkg <https://alioth.debian.org/projects/dpkg>`__ to build the iOS
  IPA package

- `mkisofs <http://cdrecord.org/private/cdrecord.html>`__ to build the
  macOS DMG package

To compile for iOS / AArch64, run::

  make TARGET=IOS64 ipa

To compile for iOS / ARMv7, run::

  make TARGET=IOS32 ipa

To compile for macOS / x86_64, run::

  make TARGET=OSX64 dmg

Compiling for macOS (with Homebrew)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Install the required Homebrew packages::

  brew install automake autoconf libtool imagemagick ffmpeg \
      librsvg quilt pkg-config

Then compile::

  make dmg

Compiling on the Raspberry Pi 4
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Install additional dependencies::

  sudo apt install libdrm-dev libgbm-dev \
      libgles2-mesa-dev \
      libinput-dev

Compile::

  make

Compiling for the Raspberry Pi 1-3
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You need an ARM toolchain. For example, you can use the Debian package
``g++-arm-linux-gnueabihf``::

  make TARGET=PI

To optimize for the Raspberry Pi 2 (which has an ARMv7 with NEON instead
of an ARMv6)::

  make TARGET=PI2

These targets are only used for cross-compiling on a (desktop) computer.
If you compile on the Raspberry Pi, the default target will auto-detect
the Pi.

Compiling for the Cubieboard
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To compile, run::

  make TARGET=CUBIE

This target is only used for cross-compiling on a (desktop) computer. If
you compile on the Cubieboard, the default target will auto-detect the
Cubieboard.

Compiling for Kobo E-book Readers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

An ARM toolchain is bootstrapped during the build automatically.

To compile XCSoar, run::

  make TARGET=KOBO

To build the kobo install file ``KoboRoot.tgz``, you need the following
Debian packages::

  sudo apt install fakeroot ttf-bitstream-vera python3-setuptools

Then compile using this command::

  make TARGET=KOBO output/KOBO/KoboRoot.tgz

Building USB-OTG Kobo Kernel
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To build a USB-OTG capable kernel for the Kobo, clone the git
repository::

  git clone git://git.xcsoar.org/xcsoar/max/linux.git

Check out the correct branch. For the Kobo Mini, this is the “kobo”
branch, for the Kobo Glo HD, the branch is called “kobo-glohd”, and for
the Kobo Aura 2, use the branch “kobo-aura2”::

  git checkout kobo

Configure the kernel using the configuration files from the
``kobo/kernel`` directory in XCSoar’s ``git`` repository. For the Kobo
Mini, install a `gcc 4.4 cross
compiler <http://openlinux.amlogic.com:8000/download/ARM/gnutools/arm-2010q1-202-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2>`__,
for example in ``/opt``. For the Kobo Glo HD and Aura 2, install a `gcc
4.6 cross
compiler <https://launchpad.net/gcc-arm-embedded/4.6/4.6-2012-q4-update/+download/gcc-arm-none-eabi-4_6-2012q4-20121016.tar.bz2>`__

To compile a kernel image for the Kobo Mini, type::

  make \
      CROSS_COMPILE=/opt/arm-2010q1/bin/arm-none-linux-gnueabi- \
      ARCH=arm uImage

To compile a kernel image for the Kobo Glo HD, type::

  make \
      CROSS_COMPILE=/opt/gcc-arm-none-eabi-4_6-2012q4/bin/arm-none-eabi- \
      ARCH=arm uImage

Copy ``uImage`` to the Kobo. Kernel images can be installed with the
following command::

  dd if=/path/to/uImage of=/dev/mmcblk0 bs=512 seek=2048

Note that XCSoar’s ``rcS`` script may overwrite the kernel image
automatically under certain conditions. To use a new kernel permanently,
install it in ``/opt/xcsoar/lib/kernel``. Read the file ``kobo/rcS`` to
find out more about this.

To include kernel images in ``KoboRoot.tgz``, copy ``uImage.otg``,
``uImage.kobo``, ``uImage.glohd.otg``, ``uImage.glohd``,
``uImage.aura2`` and ``uImage.aura2.otg`` to ``/opt/kobo/kernel``.

Editing the Manuals
~~~~~~~~~~~~~~~~~~~

The XCSoar documententation (except for the developer manual) is
written using the TeX markup language. You can edit the source files
with any text editor, although a specific TeX editor (e.g. LateXila)
makes it easier.

Source files are located in the en, fr, de, pl subdirectories of the
:file:`doc/manual` directory. The Developer manual is in the
doc/manual/en directory. The generated files are put into the
output/manual directory.

To generate the PDF manuals, you need the TexLive package, plus some
European languages.

The following command installs these on Debian::

  sudo apt install texlive \
      texlive-latex-extra \
      texlive-luatex \
      texlive-lang-french \
      texlive-lang-polish \
      texlive-lang-german \
      texlive-lang-portuguese \
      liblocale-po-perl

The documentation is distributed as PDF files. Generating the PDFs from
the TeX files is done by typing::

  make manual

A lot of warnings are generated... this is normal. Check for the
presence of PDF files to ensure that the generation process was
successful.

Options
-------

Parallel Build
~~~~~~~~~~~~~~

Most contemporary computers have multiple CPU cores. To take advantage
of these, use the ``make -j`` option::

  make -j12

This command launches 12 compiler processes at the same time.

Rule of thumb: choose a number that is slightly larger than the number
of CPU cores in your computer. 12 is a good choice for a computer with 8
CPU cores.

Optimised Build
~~~~~~~~~~~~~~~

By default, debugging is enabled and compiler optimisations are
disabled. The resulting binaries are very slow. During development, that
is helpful, because it catches more bugs.

To produce optimised binaries, use the option ``DEBUG``::

  make DEBUG=n

Be sure to clean the output directory before you change the ``DEBUG``
setting, because debug and non-debug output files are not compatible.

The convenience target ``OPT`` is a shortcut for::

  TARGET=UNIX DEBUG=n TARGET_OUTPUT_DIR=output/OPT

It allows building both debug and non-debug incrementally, because two
different output directories are used.

Compiling with ccache
~~~~~~~~~~~~~~~~~~~~~

To speed up the compilation of XCSoar we can use ``ccache`` to cache the
object files for us. All we have to do is install ccache and add
``USE_CCACHE=y`` to the make command line::

  sudo apt install ccache
  make TARGET=UNIX USE_CCACHE=y

Using a build VM with Vagrant
-----------------------------

An easy way to install a virtual machine with all build dependencies
required for various targets (e.g. Linux, Windows, Android and Kobo), is
using Vagrant.

The following is needed to install the VM with Vagrant:

-  `Vagrant <https://www.vagrantup.com/>`__

-  `VirtualBox <https://www.virtualbox.org/>`__

The Vagrantfile can be found in the ``ide/vagrant`` subfolder of the
source. To set up the VM, and connect to it, type::

  cd ide/vagrant
  vagrant up
  vagrant ssh

The XCSoar source directory on the host is automatically mounted as a
shared folder at ``/xcsoar-host-src`` in the VM. For performance
reasons, it is not recommended to compile directly in this folder. A git
clone of this directory is automatically created in the home directory
(`` /xcsoar-src``), which should be used instead. In this git clone, the
XSoar source directory on the host is preconfigured as a git remote
named “host”, and the XCSoar master directory is preconfigured as a
remote named “master”.

To shutdown the VM, type::

  vagrant halt
