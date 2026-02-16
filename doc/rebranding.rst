####################
Rebranding XCSoar
####################

This guide documents how to rebrand XCSoar to another name, making it easier to create and maintain forks.

.. contents:: Table of Contents
   :depth: 3
   :local:

Quick Start
===========

XCSoar provides :file:`src/ProductName.hpp` to centralize product name configuration.

Method 1: Build-Time Override
------------------------------

Override macros at build time::

  make PRODUCT_NAME=MyProduct PRODUCT_NAME_LC=myproduct TARGET=UNIX

Method 2: Permanent Changes
----------------------------

Modify :file:`src/ProductName.hpp` directly::

  #define PRODUCT_NAME "MyProduct"
  #define PRODUCT_NAME_LC "myproduct"
  #define ANDROID_PACKAGE "com.mycompany.myproduct"

Then build normally::

  make TARGET=UNIX


Product Name Macros
===================

The :file:`src/ProductName.hpp` header provides the following macros:

Core Product Name
-----------------

``PRODUCT_NAME``
  The main product name (e.g., ``"XCSoar"``).
  Can be overridden at build time.

``PRODUCT_NAME_LC``
  Lowercase version for file paths and Unix conventions (e.g., ``"xcsoar"``).

``PRODUCT_NAME_A``
  ASCII version for help text and logging (same as ``PRODUCT_NAME``).

Android Configuration
---------------------

``ANDROID_PACKAGE``
  Android package name in reverse domain notation (e.g., ``"org.xcsoar"``).

Data Directory Paths
--------------------

``PRODUCT_DATA_DIR``
  Data directory name for Windows/macOS (e.g., ``"XCSoarData"``).

``PRODUCT_UNIX_SYSCONF_DIR``
  System-wide configuration directory on Linux/Unix.
  Derived from: ``"/etc/" PRODUCT_NAME_LC`` → ``"/etc/xcsoar"``

``PRODUCT_UNIX_HOME_DIR``
  User home directory on Linux/Unix.
  Derived from: ``"." PRODUCT_NAME_LC`` → ``".xcsoar"``


Platform-Specific Paths
========================

The product name macros automatically generate appropriate paths for each platform:

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - Platform
     - Data Directory
     - Notes
   * - Linux/Unix
     - ``~/.xcsoar``
     - Hidden, follows Unix conventions
   * - Windows
     - ``%LOCALAPPDATA%\XCSoarData``
     - Visible folder
   * - macOS (desktop)
     - ``~/XCSoarData``
     - Visible, macOS users prefer this
   * - iOS
     - ``~/Documents/XCSoarData``
     - Visible via iTunes file sharing
   * - System-wide
     - ``/etc/xcsoar``
     - Global config on Linux/Unix


Files Using Product Name
=========================

The following files automatically use the product name macros:

Source Code Files
-----------------

- :file:`src/Version.cpp` - Version strings
- :file:`src/Version.hpp` - Version declarations
- :file:`src/LocalPath.cpp` - Data directory paths
- :file:`src/Gauge/LogoView.cpp` - Product token display
- :file:`src/XCSoar.cpp` - Help text
- :file:`src/MainWindow.hpp` - Window title

Build System Files
------------------

- :file:`build/main.mk` - Program name
- :file:`build/android.mk` - Android package
- :file:`build/version.mk` - Version defines

Android Files
-------------

- :file:`android/res/values/strings.xml` - App display name
- :file:`android/AndroidManifest.xml.in` - Android manifest


What Needs Manual Changes
==========================

For a complete rebrand, some files still require manual editing:

Version Symbol Names
--------------------

:file:`src/Version.cpp` and :file:`src/Version.hpp` use hardcoded ``XCSoar_*`` symbol names:

- ``XCSoar_Version``
- ``XCSoar_VersionLong``
- ``XCSoar_VersionString``
- ``XCSoar_ProductToken``

These can be kept as-is (they're internal symbols) or changed manually if desired.

Android Display Name
--------------------

:file:`android/res/values/strings.xml` contains::

  <string name="app_name">XCSoar</string>
  <string name="app_name_testing">XCS-test</string>

These could be changed manually or via build-time substitution.

Logo and Graphics
-----------------

Replace graphics files in:

- :file:`Data/graphics/` - Logo and splash screen images
- :file:`Data/iOS/Assets.xcassets/` - iOS app icons


Usage Examples
==============

Example 1: Quick Test Build
----------------------------

Build XCSoar as "MyGlider" for testing::

  make PRODUCT_NAME=MyGlider PRODUCT_NAME_LC=myglider TARGET=UNIX

**Result:**

- Binary name: ``myglider`` (Linux) or ``MyGlider`` (Windows)
- Data directory: ``%LOCALAPPDATA%\MyGliderData`` (Windows), ``~/MyGliderData`` (macOS), or ``~/.myglider`` (Linux)
- System config: ``/etc/myglider``

Example 2: Permanent Fork
--------------------------

For a permanent fork, edit :file:`src/ProductName.hpp`:

.. code-block:: cpp

  #ifndef PRODUCT_NAME
  #define PRODUCT_NAME "OpenSoar"
  #endif

  #ifndef PRODUCT_NAME_LC
  #define PRODUCT_NAME_LC "opensoar"
  #endif

  #ifndef ANDROID_PACKAGE
  #define ANDROID_PACKAGE "de.opensoar"
  #endif

Then build normally::

  make TARGET=UNIX

Example 3: Android Build
-------------------------

Build an Android APK with custom branding::

  make PRODUCT_NAME=MyGlider PRODUCT_NAME_LC=myglider \
       ANDROID_PACKAGE=com.mycompany.myglider \
       TARGET=ANDROID


Comparison with Other Forks
============================

OpenSoar Rebranding Analysis
-----------------------------

The `OpenSoar <https://github.com/OpenSoaring/OpenSoar>`__ fork demonstrates
a complete rebranding. Here's what they changed:

Configuration File
~~~~~~~~~~~~~~~~~~

Created :file:`OpenSoar.config` in the root directory::

  PROGRAM_NAME=OpenSoar
  PROGRAM_VERSION=7.43-3.24.1
  ANDROID_VERSIONCODE=24
  ANDROID_PACKAGE=de.opensoar

Key Changes
~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 30 30

   * - Category
     - XCSoar
     - OpenSoar
   * - Data Directory (Windows)
     - ``XCSoarData``
     - ``OpenSoarData``
   * - Home Directory (Linux)
     - ``~/.xcsoar``
     - ``~/.opensoar``
   * - Version Symbols
     - ``XCSoar_Version``
     - ``OpenSoar_Version``
   * - Android Package
     - ``org.xcsoar``
     - ``de.opensoar``
   * - App Display Name
     - "XCSoar"
     - "OpenSoar"
   * - System Config Dir
     - ``/etc/xcsoar``
     - ``/etc/opensoar``

XCSoar vs OpenSoar Approach
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**OpenSoar Approach:**

- ❌ Hardcoded ``OpenSoar_*`` symbols in :file:`Version.cpp`
- ❌ Direct string replacements throughout codebase
- ✅ Centralized config file (:file:`OpenSoar.config`)
- ⚠️ Changed from ``TCHAR`` to ``char`` (breaks Windows Unicode)

**XCSoar Approach:**

- ✅ Single header file (:file:`ProductName.hpp`)
- ✅ Build-time override support
- ✅ Smart macro composition (e.g., ``"/etc/" + PRODUCT_NAME_LC``)
- ⚠️ Version symbols still use ``XCSoar_*`` (acceptable for internal use)
- ⚠️ Changed from ``TCHAR`` to ``char`` (breaks Windows Unicode)

Advantages
~~~~~~~~~~

XCSoar's approach offers several advantages:

1. **Easier to maintain** - Single header file vs. scattered changes
2. **Preserves compatibility** - Keeps cross-platform features like ``TCHAR``
3. **Build-time flexibility** - Can test different names without committing
4. **Less invasive** - Doesn't require changing internal symbol names


Testing Checklist
=================

When rebranding XCSoar, verify:

.. |check| unicode:: U+2611 .. BALLOT BOX WITH CHECK

User Interface
--------------

- |check| Program name appears correctly in title bar
- |check| About dialog shows correct product name
- |check| Help text references correct product name
- |check| Version display shows correct product name

File System
-----------

- |check| Data directory location (check ``~/.productname`` on Linux)
- |check| Config file location (``/etc/productname`` on Linux)
- |check| Log files use correct product name

Android
-------

- |check| Android package name is correct
- |check| App installs with correct name
- |check| App icon displays correctly
- |check| App appears correctly in Android launcher

Flight Data
-----------

- |check| IGC files use correct product name (if applicable)
- |check| Flight logs reference correct product name


Advanced Customization
======================

For forks requiring deeper customization beyond the product name:

Custom Versioning
-----------------

XCSoar's version system uses:

- :file:`VERSION.txt` - Base version number
- :file:`build/version.mk` - Version processing
- ``XCSOAR_VERSION`` macro - Version string
- ``GIT_COMMIT_ID`` macro - Git commit hash

Custom Build System
-------------------

For forks using CMake (like OpenSoar), see their :file:`CMakeLists.txt`
for integration with build systems other than GNU Make.

Custom Features
---------------

When adding fork-specific features:

1. Use ``#ifdef HAVE_FEATURE_NAME`` for conditional compilation
2. Add feature detection in :file:`build/options.mk`
3. Keep core XCSoar code unmodified when possible
4. Document your changes in your fork's documentation


Contributing Improvements
=========================

If you develop improvements to XCSoar's rebranding system:

1. Ensure changes maintain backward compatibility
2. Test on multiple platforms (Linux, Windows, Android)
3. Update this documentation
4. Submit a pull request to upstream XCSoar

See :doc:`policy` for contribution guidelines.


Frequently Asked Questions
==========================

Do I need to change the version symbols (``XCSoar_Version``, etc.)?
--------------------------------------------------------------------

**No.** These are internal C++ symbols and don't affect the user-visible
product name. Most forks keep these as-is.

If you want to change them, you'll need to:

1. Modify :file:`src/Version.cpp` and :file:`src/Version.hpp`
2. Update all references to these symbols throughout the codebase

Will upstream merges break my rebranding?
-----------------------------------------

**No.** By keeping your changes in :file:`src/ProductName.hpp`, you can
merge upstream XCSoar changes without conflicts. The header file is designed
to be fork-friendly.

Can I use the same data directory as XCSoar?
---------------------------------------------

**Yes**, but not recommended. If you set::

  #define PRODUCT_NAME_LC "xcsoar"

Your fork will use ``~/.xcsoar``, sharing the same config with XCSoar.
This can be useful for testing but may confuse users.

How do I change the splash screen and logo?
--------------------------------------------

Replace the image files in:

- :file:`Data/graphics/logo.svg` - Main logo
- :file:`Data/graphics/logo_*.svg` - Platform-specific logos
- :file:`Data/graphics/splash_*.bmp` - Splash screens

The build system will automatically process these during compilation.

Can I distribute my rebranded fork?
------------------------------------

**Yes**, under the terms of XCSoar's GPL-2.0-or-later license. You must:

1. Keep the GPL license
2. Provide source code to your users
3. Credit the XCSoar Project
4. Document your changes

See :file:`COPYING` for full license terms.
