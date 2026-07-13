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

  git clone --recurse-submodules https://github.com/XCSoar/XCSoar

To update your repository, type::

  git pull --recurse-submodules

To update third-party libraries used by XCSoar (such as `Boost
<http://www.boost.org/>`__), type::

  git submodule update --init --recursive

For more information, please read the :program:`git` documentation.

Requirements
------------

The following is needed for all targets:

-  GNU make

-  GNU compiler collection (``gcc``), version 10 or later or clang/LLVM
   12 (with "make CLANG=y")

-  GNU gettext

-  `rsvg <https://librsvg.sourceforge.net/>`__

-  `ImageMagick 6.4 <https://www.imagemagick.org/>`__

-  `xsltproc <http://xmlsoft.org/XSLT/xsltproc2.html>`__

-  `Info-ZIP <http://www.info-zip.org/>`__

-  Perl

-  `SoX <http://sox.sourceforge.net/>`__

On Debian and Ubuntu, install these with the ``BASE`` section of
:file:`ide/provisioning/install-debian-packages.sh` (GCC/g++ is installed
with ``LINUX``)::

  cd ide/provisioning
  sudo ./install-debian-packages.sh UPDATE BASE

Package lists for all targets live in that script; pass one or more
section names instead of running it with no arguments. Available
sections:

=========== ===========================================================
``BASE``    tools common to all targets (make, gettext, git, ccache, …)
``LINUX``   native Linux/UNIX build libraries and compiler
``LIBINPUT_GBM`` libinput, GBM, DRM, GLES (Pi and similar embedded Linux)
``WAYLAND`` Wayland protocols
``DEBIAN``  building ``.deb`` packages
``LLVM``    compiling with Clang
``ARM``     ARM cross toolchain (Kobo bootstrap, generic ARM)
``PI_HOST`` Pi cross-compile host tools (debootstrap, qemu, …)
``WIN``     MinGW Windows cross compiler and NSIS
``KOBO``    Kobo image packaging tools
``ANDROID`` JDK and Android host tools (not SDK/NDK)
``MANUAL``  TeXLive for ``make manual``
=========== ===========================================================

See also :doc:`devsetup` for a Linux development environment overview.

On Debian and Ubuntu, the usual order is: **native build** on the host
(recommended), then **Docker** if you avoid installing dependencies
locally, then **Vagrant** for an optional preconfigured VM. See
:ref:`docker-build` and “Using a build VM with Vagrant” below.

Target-specific Build Instructions
----------------------------------

Compiling for Linux/UNIX
~~~~~~~~~~~~~~~~~~~~~~~~

The following additional packages are needed to build for Linux and
similar operating systems:

-  `zlib <https://www.zlib.net/>`__

-  `c-ares <https://c-ares.org/>`__

-  `CURL <https://curl.se/>`__

-  `Lua <https://www.lua.org/>`__

-  `libinput <https://www.freedesktop.org/wiki/Software/libinput/>`__
   (not required when using Wayland or on the KOBO)

-  `SDL <https://www.libsdl.org/>`__

-  `SDL_ttf <https://github.com/libsdl-org/SDL_ttf>`__

-  `libpng <http://www.libpng.org/pub/png/libpng.html>`__

-  `libjpeg <https://libjpeg.sourceforge.net/>`__

-  `NetCDF <https://www.unidata.ucar.edu/software/netcdf/>`__
  (native Linux builds)

-  OpenGL (Mesa)

-  to run XCSoar, you need one of the following fonts (Debian package):
   DejaVu (``fonts-dejavu``), Roboto (``fonts-roboto``), Droid
   (``fonts-droid``), Freefont (``fonts-freefont-ttf``)

Install host packages on Debian/Ubuntu::

  cd ide/provisioning
  sudo ./install-debian-packages.sh UPDATE BASE LINUX LIBINPUT_GBM

To compile, run::

  make

You may specify one of the following targets with ``TARGET=x``:

========== =================================================
``UNIX``   regular build (the default setting)
``UNIX32`` generate 32 bit binary
``UNIX64`` generate 64 bit binary
``OPT``    alias for UNIX with optimisation and no debugging
========== =================================================

Experimental Wayland build
^^^^^^^^^^^^^^^^^^^^^^^^^^

Install the ``WAYLAND`` provisioning section in addition to ``LINUX``::

  cd ide/provisioning
  sudo ./install-debian-packages.sh UPDATE BASE LINUX WAYLAND

Compile::

  make TARGET=WAYLAND

Software rendering without OpenGL
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For headless builds (CI, no GPU), use the virtual framebuffer::

  make TARGET=UNIX VFB=y

For software rendering with SDL (same as Docker ``UNIX-SDL``), disable
OpenGL and enable SDL2::

  make TARGET=UNIX OPENGL=n ENABLE_SDL=y USE_SDL2=y

``VFB`` and SDL are mutually exclusive with the default OpenGL/EGL path.

Compiling for Android
~~~~~~~~~~~~~~~~~~~~~

For Android, you need:

- `Android SDK level 35 <http://developer.android.com/sdk/>`__

- `Android NDK r26d <http://developer.android.com/sdk/ndk/>`__

- `Ogg Vorbis <http://www.vorbis.com/>`__

- Java JDK

On Debian, install host packages and the SDK/NDK via the provisioning
scripts::

  cd ide/provisioning
  sudo ./install-debian-packages.sh UPDATE BASE LINUX ANDROID
  ./install-android-tools.sh

The required Android SDK components are:

- Android SDK Build-Tools 35.0.0

- SDK Platform 35

These can be installed from the Android Studio SDK Manager. On Debian/Ubuntu,
:file:`ide/provisioning/install-android-tools.sh` (run after the ``ANDROID``
provisioning section) downloads the command-line tools and installs the
required packages automatically.

To install the same components manually::

  ~/opt/android-sdk-linux/cmdline-tools/bin/sdkmanager \
      --sdk_root=~/opt/android-sdk-linux \
      "build-tools;35.0.0" "platforms;android-35"

The ``Makefile`` assumes that the Android SDK is installed in
``~/opt/android-sdk-linux`` and the NDK is installed in
``~/opt/android-ndk-r26d``. You can use the options ``ANDROID_SDK`` and
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

On Debian, install the cross compiler and NSIS (for the installer)::

  cd ide/provisioning
  sudo ./install-debian-packages.sh UPDATE BASE LINUX WIN

A minimal 64-bit OpenGL build::

  make TARGET=WIN64OPENGL

Use one of the following targets:

================ =================================================
``WIN64OPENGL``  Windows x64 (amd64 / x86-64), OpenGL via ANGLE (recommended)
``WIN32OPENGL``  Windows 32-bit (i686), OpenGL via ANGLE (recommended)
``PC``           32-bit Windows (i686), GDI legacy build (**deprecated**)
``WIN64``        Windows x64 (amd64 / x86-64), GDI legacy build (**deprecated**)
================ =================================================

The GDI targets ``PC`` and ``WIN64`` are deprecated and will be removed in a
future release. New development and releases focus on the OpenGL targets
above.

Typical OpenGL build commands::

  make -j$(nproc) TARGET=WIN64OPENGL USE_CCACHE=y everything
  make -j$(nproc) TARGET=WIN32OPENGL USE_CCACHE=y everything

To build the NSIS installer as well (64-bit or 32-bit)::

  make -j$(nproc) TARGET=WIN64OPENGL USE_CCACHE=y everything installer

OpenGL Windows builds use SDL2 and GLES2 via ANGLE (D3D11 backend). ANGLE
libraries are fetched automatically on first build by
:file:`windows/fetch-angle-from-github.sh` into the target output tree (see
:file:`build/angle.mk`).

Build outputs (64-bit example; 32-bit uses ``WIN32OPENGL`` and ``x86`` ANGLE
arch instead):

- ``output/WIN64OPENGL/bin/XCSoar.exe`` — main executable
- ``output/WIN64OPENGL/bin/XCSoar.zip`` — portable package (exe, ANGLE DLLs,
  bundled fonts)
- ``output/WIN64OPENGL/bin/XCSoar-<version>-WIN64OPENGL-Installer.exe`` —
  NSIS installer (``installer`` target only)
- ``output/WIN64OPENGL/bin/libEGL.dll``,
  ``output/WIN64OPENGL/bin/libGLESv2.dll`` — ANGLE runtime (also inside zip
  and installer)

Some features are compiled only when ``OPENGL=y`` (all OpenGL Windows targets),
for example EDL weather and MbTiles map overlays. The deprecated GDI builds do
not include them.

Compiling for iOS
~~~~~~~~~~~~~~~~~~~~~~~~~~~

To compile for iOS, you need a Mac with Xcode (at least the Command Line Tools) installed.

Install the required Homebrew packages via the provisioning script::

  ./ide/provisioning/install-darwin-packages.sh BASE IOS

Note that you always need to install the BASE packages and the specific IOS or MACOS packages.

To compile for iOS / AArch64, run::

  make TARGET=IOS64 ipa

To compile with the iOS simulator SDK, run::

  make TARGET=IOS64SIM ipa

To build and run simulator tests automatically, run::

  make check-ios-sim

This target builds simulator artifacts (``TARGET=IOS64SIM``), installs
``XCSoar.app`` into an available iPhone simulator, and runs selected test
binaries. There is no fixed default device name (``simctl`` depends on
installed runtimes and Xcode). With ``SIM_DEVICE_NAME`` unset, the script
picks an available iPhone from ``simctl list``; you can set ``SIM_DEVICE_NAME``
to require a specific model.

Implementation note: the runner uses a Python script
(``darwin/check-ios-sim.py``) and discovers simulators via
``xcrun simctl list devices available --json``.
Execution is delegated to the existing Perl TAP harness
(``test/src/testall.pl``) using generated simulator wrapper executables.

Optional environment variables::

  SIM_DEVICE_NAME="iPhone 16"      # Optional: require this model; else any iPhone
  SIM_TESTS_MODE=all               # Default mode: run all test_* / Test* binaries
  SIM_TESTS_MODE=smoke             # Run only smoke subset from SIM_SMOKE_TESTS
  SIM_SMOKE_TESTS="TestCRC8 ..."  # Override smoke test selection
  SIM_SKIP_TESTS="TestWrapText"    # Space-separated tests to skip in simulator

To compile for iOS / ARMv7, run::

  make TARGET=IOS32 ipa

Compiling for macOS (with Homebrew)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Install the required Homebrew packages via the provisioning script::

  ./ide/provisioning/install-darwin-packages.sh BASE MACOS

Note that you always need to install the BASE packages and the specific IOS or MACOS packages.

To compile for macOS / ARM64, run::

  make TARGET=MACOS dmg

To compile for macOS / x86_64, run::

  make TARGET=OSX64 dmg

Debugging for iOS and macOS
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Debugging under iOS and macOS is possible using the LLDB debugger.
To make this convenient, Xcode or Visual Studio can be used.
An example Xcode project is provided in `darwin/XCSoar.xcodeproj`. 
It includes one target for iOS and macOS and will automatically build 
the XCSoar binary for the selected device target, using the build
helper script `darwin/build.sh`.
For iOS debugging with Visual Studio Code, the `iOS Debug`
extension (https://github.com/nisargjhaveri/vscode-ios-debug) can be used.
Note that this also requires an Xcode installation.

Compiling on the Raspberry Pi (4 / 5)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On current Raspberry Pi boards, compile **natively on the device**. Pi 4 and
Pi 5 are fast enough for practical development builds; cross-compiling from a
desktop host is no longer the usual workflow.

On the Pi, install the same embedded-Linux graphics packages as a desktop
``UNIX`` build, plus ``LIBINPUT_GBM`` (DRM/GBM/GLES)::

  cd ide/provisioning
  sudo ./install-debian-packages.sh UPDATE BASE LINUX LIBINPUT_GBM

Compile (``TARGET=UNIX`` is selected automatically on Raspberry Pi)::

  make -j$(nproc) USE_CCACHE=y

Cross-compiling for legacy Raspberry Pi 1–3
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``TARGET=PI`` and ``TARGET=PI2`` remain for cross-compiling **on a desktop
host** for old ARMv6/ARMv7 boards (Pi 1–3). This path needs an armhf sysroot.
The same steps work on a native Debian/Ubuntu host or inside the
:ref:`docker-build` container (privileged, for ``setup-pi-sysroot.sh``).
``setup-pi-sysroot.sh`` installs ``PI_HOST`` tools on the host by default::

  cd ide/provisioning
  sudo ./setup-pi-sysroot.sh

Cross-compile (Pi 1 / ARMv6; binaries in ``output/PI/bin/``)::

  make TARGET=PI PI=/opt/pi/root

For Raspberry Pi 2/3 (ARMv7 with NEON; ``output/PI2/bin/``)::

  make TARGET=PI2 PI=/opt/pi/root

Although ``TARGET=PI`` and ``TARGET=PI2`` compile as ``UNIX`` internally,
binaries are written to ``output/PI/`` and ``output/PI2/`` respectively
(the flavour name is preserved for the output directory).

Compiling for the Cubieboard
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``TARGET=CUBIE`` cross-compiles XCSoar for ARMv7 + NEON (Cubieboard-class
boards) on a desktop host. You need an armhf sysroot and the cross
toolchain; install the ``ARM`` provisioning section::

  cd ide/provisioning
  sudo ./install-debian-packages.sh UPDATE ARM

Cross-compile (binaries in ``output/CUBIE/bin/``)::

  make TARGET=CUBIE CUBIE=/opt/cubie/root

The default sysroot path is ``/opt/cubie/root``. Unlike the Raspberry Pi
path, XCSoar does not ship an automated sysroot script for Cubie; the
sysroot must provide ARMhf development libraries and the Mali/EGL headers
under ``usr/local/stow/sunxi-mali/`` (see ``build/targets.mk``).

If you compile on the Cubieboard itself, the default ``UNIX`` target
auto-detects the board.

OpenVario flight computer images
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For **OpenVario** hardware (Cubieboard-based flight computers, CH-070
and other displays), building the full OS image is handled outside this
tree. Follow the current instructions in the
`meta-openvario <https://github.com/Openvario/meta-openvario>`__
repository (``MACHINE`` selection, Docker build container, ``bitbake``,
and so on). See also `OpenVario <https://openvario.org>`__.

Compiling for Kobo E-book Readers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

An ARM toolchain is bootstrapped during the build automatically.

To compile XCSoar, run::

  make TARGET=KOBO

To build the kobo install file ``KoboRoot.tgz``, install the Kobo
packaging section in addition to the Kobo build dependencies (the
``KOBO`` target bootstraps its own ARM toolchain during the build)::

  cd ide/provisioning
  sudo ./install-debian-packages.sh UPDATE BASE LINUX ARM KOBO

Then compile using this command::

  make TARGET=KOBO output/KOBO/KoboRoot.tgz

Building USB-OTG Kobo Kernel
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To build a USB-OTG capable kernel for the Kobo, clone the git
repository::

  git clone https://github.com/XCSoar/linux.git

Check out the correct branch. For the Kobo Mini, this is the branch
``kobo-mini``, for the Kobo Glo HD, the branch is called
``kobo-glohd``, and for the Kobo Aura 2, use the branch
``kobo-aura2``::

  git checkout kobo-mini

Configure the kernel using the configuration files from the
``kobo/kernel`` directory in XCSoar’s ``git`` repository. For the Kobo
Mini, install a `gcc 4.4 cross
compiler <https://master.dl.sourceforge.net/project/iadfilehost/devtools/arm-2010q1-202-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2>`__,
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

.. _build-system-reference:

Graphics Backends by Target
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Defaults shown are from the build system (they can be overridden with
``ENABLE_SDL=y|n`` and ``OPENGL=y|n``).

.. list-table::
 :widths: 12 20 12 18 38
 :header-rows: 1

 * - Target
   - Platform / ABI
   - SDL (default)
   - Graphics Engine (default)
   - Notes
 * - ``UNIX``
   - Linux/Unix (native)
   - no
   - OpenGL
   - Default on Unix-like hosts; main desktop build.
 * - ``UNIX32``
   - Linux/Unix 32-bit
   - no
   - OpenGL
   - ``UNIX`` with ``-m32``.
 * - ``UNIX64``
   - Linux/Unix 64-bit
   - no
   - OpenGL
   - ``UNIX`` with ``-m64``.
 * - ``OPT``
   - Linux/Unix optimized
   - no
   - OpenGL
   - Alias for ``UNIX`` with ``DEBUG=n`` (set ``TARGET_OUTPUT_DIR`` if
     you want a separate output tree).
 * - ``WAYLAND``
   - Linux/Unix (Wayland)
   - no
   - OpenGL (EGL)
   - Experimental Wayland display server build.
 * - ``FUZZER``
   - Linux/Unix (libFuzzer)
   - no
   - Software (VFB)
   - Builds fuzz targets with clang + libFuzzer.
 * - ``PC``
   - Windows 32-bit (i686)
   - no
   - GDI
   - MinGW-w64 cross-compile target. **Deprecated**; use ``WIN32OPENGL``.
 * - ``WIN64``
   - Windows 64-bit (x86_64)
   - no
   - GDI
   - Flavor of ``PC`` with 64-bit toolchain. **Deprecated**; use
     ``WIN64OPENGL``.
 * - ``WIN64OPENGL``
   - Windows 64-bit (x86_64)
   - yes
   - OpenGL ES (ANGLE)
   - Recommended Windows 64-bit build with SDL and OpenGL ES.
 * - ``WIN32OPENGL``
   - Windows 32-bit (i686)
   - yes
   - OpenGL ES (ANGLE)
   - Recommended Windows 32-bit build with SDL and OpenGL ES.
 * - ``ANDROID``
   - Android ARMv7
   - no
   - OpenGL ES (EGL)
   - Alias for ``ANDROID7``.
 * - ``ANDROID7``
   - Android ARMv7 (32-bit)
   - no
   - OpenGL ES (EGL)
   - Default Android ABI (armeabi-v7a).
 * - ``ANDROIDAARCH64``
   - Android ARM64 (64-bit)
   - no
   - OpenGL ES (EGL)
   - arm64-v8a.
 * - ``ANDROID86``
   - Android x86 (32-bit)
   - no
   - OpenGL ES (EGL)
   - x86 ABI.
 * - ``ANDROIDX64``
   - Android x86_64 (64-bit)
   - no
   - OpenGL ES (EGL)
   - x86_64 ABI.
 * - ``ANDROIDFAT``
   - Android multi-ABI
   - no
   - OpenGL ES (EGL)
   - "Fat" package across supported ABIs.
 * - ``MACOS``
   - macOS ARM64
   - yes
   - OpenGL (ANGLE)
   - Apple Silicon (min macOS 12.0).
 * - ``OSX64``
   - macOS x86_64
   - yes
   - OpenGL (ANGLE)
   - Intel (min macOS 12.0).
 * - ``IOS32``
   - iOS armv7
   - yes
   - OpenGL ES
   - Legacy 32-bit iOS (min iOS 10.0).
 * - ``IOS64``
   - iOS arm64
   - yes
   - OpenGL ES
   - Device build (min iOS 11.0).
 * - ``IOS64SIM``
   - iOS simulator arm64
   - yes
   - OpenGL ES
   - Simulator SDK (min iOS 11.0).
 * - ``PI``
   - Raspberry Pi 1
   - no
   - OpenGL (EGL/KMS)
   - ARMv6 cross-compile target (Pi 1).
 * - ``PI2``
   - Raspberry Pi 2/3
   - no
   - OpenGL (EGL/KMS)
   - ARMv7 + NEON cross-compile target (Pi 2 and 3).
 * - ``CUBIE``
   - Cubieboard
   - no
   - OpenGL (EGL/KMS)
   - Cross-compile target (ARMv7 + NEON); OpenVario images: `meta-openvario
     <https://github.com/Openvario/meta-openvario>`__.
 * - ``KOBO``
   - Kobo e-readers
   - no
   - Framebuffer (software)
   - Cross-compile target (ARMv7 + NEON).
 * - ``NEON``
   - Generic ARMv7 + NEON
   - yes
   - OpenGL
   - Internal flavor used by ``PI2``, ``CUBIE``, ``KOBO``.

For a full list of build variables and their defaults, see the
parameter list at the top of ``Makefile`` and the files in
``build/*.mk``. For ``Run*`` test/debug programs, see
:doc:`test_debug_utilities`.

Editing the Manuals
~~~~~~~~~~~~~~~~~~~

The XCSoar documentation (except for the developer manual) is
written using the TeX markup language. You can edit the source files
with any text editor, although a specific TeX editor (e.g. LateXila)
makes it easier.

Source files are located in the en, fr, de, pl subdirectories of the
:file:`doc/manual` directory. The Developer manual is in the
doc/manual/en directory. The generated files are put into the
output/manual directory.

To generate the PDF manuals, you need the TexLive package, plus some
European languages.

Install the ``MANUAL`` section on Debian/Ubuntu::

  cd ide/provisioning
  sudo ./install-debian-packages.sh UPDATE MANUAL

The documentation is distributed as PDF files. Generating the PDFs from
the TeX files is done by typing::

  make manual

A lot of warnings are generated... this is normal. Check for the
presence of PDF files to ensure that the generation process was
successful.

Options
-------

.. _parallel-build:

Parallel Build
~~~~~~~~~~~~~~

Most contemporary computers have multiple CPU cores. To take advantage
of these, use the ``make -j`` option. On Linux, ``-j$(nproc)`` matches
the number of available CPU cores::

  make -j$(nproc)

A fixed job count works too (slightly above core count is a common rule
of thumb)::

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
object files for us. ``ccache`` is installed by the ``BASE`` provisioning
section; add ``USE_CCACHE=y`` to the make command line::

  make TARGET=UNIX USE_CCACHE=y

.. _development-workflow:

Development workflow
~~~~~~~~~~~~~~~~~~~~

Day-to-day development on ``UNIX`` typically uses debug builds
(``DEBUG=y``, the default) with warnings as errors: ``WERROR`` defaults
to ``DEBUG``, so a normal ``make`` already fails on compiler warnings.
Use parallel builds (``-j$(nproc)``) and ``USE_CCACHE=y`` for faster
turnaround; see :ref:`parallel-build` below for details.

Incremental build::

  make -j$(nproc) USE_CCACHE=y

Full build with unit tests (matches what many contributors run locally
before submitting changes)::

  make -j$(nproc) USE_CCACHE=y everything check

Test and debug utilities
^^^^^^^^^^^^^^^^^^^^^^^^

Standalone tools (``RunTask``, ``RunAnalysis``, ``FeedNMEA``, …) for
component testing without the full app are described in
:doc:`test_debug_utilities`. Build them with::

  make -j$(nproc) debug

or as part of ``everything`` (below). To build one program::

  make -j$(nproc) output/UNIX/bin/RunTask

The ``everything`` target adds optional outputs, those ``debug`` utilities,
unit-test binaries, and test harness programs; ``check`` runs the tests (build
them first with ``everything`` or ``build-check``).

``debug`` lists all ``Run*`` tools in :file:`build/test.mk`; ``build-harness``
builds the ``test_*`` harness programs (also included in ``everything``).
Utilities are built for the host platform (``UNIX``, Windows OpenGL, macOS),
not for embedded targets such as Android or Kobo.

CI uses a headless software renderer and optional sanitizers (clean the
output tree when switching ``SANITIZE=``)::

  make -j$(nproc) TARGET=UNIX VFB=y USE_CCACHE=y everything check
  make -j$(nproc) TARGET=UNIX VFB=y SANITIZE=y DEBUG_GLIBCXX=y everything check

To force warnings-as-errors on an optimised build (``DEBUG=n`` disables
``WERROR`` by default), pass ``WERROR=y`` explicitly.

.. _docker-build:

Using Docker
------------

Use Docker when you prefer not to install XCSoar build dependencies on
the host. Native builds (provisioning scripts + ``make`` on Debian/Ubuntu)
are simpler and recommended when that is acceptable.

The Docker image in :file:`ide/docker/` provides a Debian-based build
environment with the same provisioning scripts as a native host. Bind-mount
the XCSoar source tree; build products appear in ``output/`` on the host.

Pull the published image (or build locally — see :file:`ide/docker/README.md`)::

  docker pull ghcr.io/xcsoar/xcsoar/xcsoar-build:latest

Interactive shell (compile with ``make`` or ``xcsoar-compile``)::

  docker run \
      --mount type=bind,source="$(pwd)",target=/opt/xcsoar \
      -it ghcr.io/xcsoar/xcsoar/xcsoar-build:latest /bin/bash

One-shot build via the wrapper script (``ANDROID``, ``DOCS``, ``KOBO``,
``UNIX``, ``UNIX-SDL``, ``WAYLAND``, ``WIN64OPENGL``, ``WIN32OPENGL``;
legacy GDI: ``PC``, ``WIN64``)::

  docker run \
      --mount type=bind,source="$(pwd)",target=/opt/xcsoar \
      ghcr.io/xcsoar/xcsoar/xcsoar-build:latest \
      xcsoar-compile UNIX USE_CCACHE=y

Windows OpenGL cross-compile example::

  docker run \
      --mount type=bind,source="$(pwd)",target=/opt/xcsoar \
      ghcr.io/xcsoar/xcsoar/xcsoar-build:latest \
      xcsoar-compile WIN64OPENGL USE_CCACHE=y everything

Add ``USE_CCACHE=y`` as needed; ccache data is stored in ``./.ccache/`` on
the host. For GUI testing with software rendering and X11 forwarding, see
:file:`ide/docker/README.md`.

**Raspberry Pi:** Pi 4 and 5 should be built **natively on the device**
(see above). The container runs on x86_64 and does not replace that
workflow.

For **legacy Pi 1–3 cross-compiles**, start a **privileged** container
(debootstrap and ``qemu-user-static`` need it), create the sysroot, then
build::

  docker run --privileged \
      --mount type=bind,source="$(pwd)",target=/opt/xcsoar \
      -it ghcr.io/xcsoar/xcsoar/xcsoar-build:latest /bin/bash

  sudo ./ide/provisioning/setup-pi-sysroot.sh
  make -j$(nproc) TARGET=PI2 PI=/opt/pi/root USE_CCACHE=y

**Android and iOS:** The image ships with the Android SDK and NDK. On
**Linux x86_64**, ``xcsoar-compile ANDROID`` is the usual Docker workflow
(see :file:`ide/docker/README.md`). Clone with ``--recurse-submodules``
(see *Getting the source* above) before mounting the tree into the
container.

**iOS** is not built inside Docker. Use a **Mac** with Xcode and the
native steps in *Compiling for iOS* above
(``ide/provisioning/install-darwin-packages.sh BASE IOS``, then
``make TARGET=IOS64 ipa``).

On **macOS** hosts, Docker-based Android builds have been reported to
hang during third-party downloads or fail under ``linux/amd64`` emulation
on Apple Silicon (`#892 <https://github.com/XCSoar/XCSoar/issues/892>`__).
Prefer a **native** Android build on the Mac (Android Studio SDK/NDK,
``make TARGET=ANDROID`` — see *Compiling for Android* above) or run the
Docker image on a Linux x86_64 machine.

Using a build VM with Vagrant
-----------------------------

Vagrant is an optional third choice: a VirtualBox VM with dependencies
for several targets. Prefer a native host install or Docker unless you
specifically want this workflow.

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
(``~/xcsoar-src``), which should be used instead. In this git clone, the
XCSoar source directory on the host is preconfigured as a git remote
named “host”, and the XCSoar master directory is preconfigured as a
remote named “master”.

To shutdown the VM, type::

  vagrant halt
