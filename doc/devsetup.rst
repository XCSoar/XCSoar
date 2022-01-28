###################################################
Setting up a development environment based on linux
###################################################

This describes the setup of a development environment suitable to
compile XCSoar for most supported platforms. The manual focuses on recent
releases of Debian-based flavors of GNU/Linux (including Ubuntu).

In the following instructions, ``sudo`` is used to execute commands with
root privileges. This is not enabled by default in Debian (but on some
Debian based distributions, like Ubuntu).

To install a virtual machine with the required, you can use ``Vagrant``.

Download source code
====================

To download the XCSoar source code, make sure you have git installed::

 sudo apt-get update
 sudo apt-get install git

Download the source code of XCSoar by executing ``git`` in the
following way in your project directory::

 git clone --recurse-submodules git://github.com/XCSoar/XCSoar

Use provisioning scripts
========================

If you are not using Vagrant, but an existing standard installation of a
Debian-based Linux distribution, you can run the scripts from
``ide/provisioning`` subfolder of the XCSoar source to install the build
dependencies for various XCSoar target platforms.

::

   cd ide/provisioning
   sudo ./add-debian-unstable.sh
   sudo ./install-debian-packages.sh
   ./install-android-tools.sh

Optional: Eclipse IDE
=====================

One of the most widespread IDEs is eclipse. It is not limited to
Android, and can be used for all targets. It is not required for XCSoar, but
its installation is described here as an example. Eclipse is quite
heavyweight, and many developers prefer other IDEs for XCSoar development.

To install, download the eclipse installer (Sometimes called
“*Ooomph!*” for some reason) from here:
``https://www.eclipse.org/downloads/``

Important: Install the CDT version of eclipse for C development, not the
Android/Java package, even if you plan developing for Android. In
addition, it is very convenient to install the git support (egit).

The current stable version is *eclipse mars* (4.5) and works with
OpenJDK 7 or 8, the new *eclipse neon* 4.6, currently RC2, is also quite
stable, and requires OpenJDK 8. Both can be installed with the
installer.

You can also install the ADT (Android development tools) package for
better integration with Android.

Next, create a new project, by generating a make project from existing
sources files. Choose your xcsoar source directory which contains the
makefile.

Important: After you have added the sources, eclipse will start
indexing all files. If you have already started ``make`` before this
time, then a lot of files have been downloaded for the various
libraries which are exctracted/built within the XCSoar directory (most
notably the boost libraries). Indexing all these takes a very long
time, and a lot of heap space, so you should probably stop the indexer
right away. In addition you should probably exclude these directories
from the indexer for the future.

For this, in the C/C++ scope, right-click on the “output” directory in
the file tree on the left side, select “Properties”, then
“Resource/Resource Filters” and add a filter. In the “add filter”
dialog, choose “exclude all”, “files and folders”, “all children
(recursive)” and set the Filter details to “Name matches \*”. This will
exclude the output tree from the indexer, leading to a minimal index.

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
