************************************
Setting up a development environment
************************************

This describes the setup of a development environment suitable to
compile XCSoar for most supported platforms (such as Android, KOBO, 
UNIX, etc.), on computers running GNU/Linux, Windows or MacOS as the
main operating system.

When linux environments are used in this manual, recent
releases of Debian-based flavors of GNU/Linux (including Ubuntu) are
assumed. In the following instructions, ``sudo`` is used to execute commands with
root privileges. This is not enabled by default in Debian (but on some
Debian based distributions, like Ubuntu).


There are many different ways to achieve a working development setup for each
platform / operating system (OS). If you are an experienced developer, just
do what you like best. if you are less experienced, then you find a setup guide
in the following which describes several methods which are particularly
supported by setup tools within the repository. If you do not
know what is best for you, just go with the recommended way. Also, see 
"which method should I choose" below.

Setup guide for Linux systems
=============================

In the following, we are assuming that your Linux distributions
is a Debian-like setup, such as Debian 12 or Ubuntu.

First: Install git and get source code
--------------------------------------

To download the XCSoar source code, make sure you have ``git`` installed. 
In Debian, you typically do::

 sudo apt-get update
 sudo apt-get install git

To download the source code of XCSoar,  execute ``git`` in the
following way in your project directory::

 git clone --recurse-submodules https://github.com/XCSoar/XCSoar

Remember to make sure to check that your name and contact are 
set up correctly if you later want to provide your contributions
to the main repository.

You can enter::

   git config --get-all user.email
   git config --get-all user.name

To check the currently configured user and contact address. If they
are not correct, you can change them with::

   git config user.name "My Name"
   git config user.email "my@email.com"

This will change the setting only for this repository. 
By adding the ``--global`` flag, you can set it for your
global git installation.

Also, *in Windows*, if you did not change the global ``autcrlf`` setting,
you can now also change it locally (only for this new repository), with::

   git config core.autocrlf input
   git config core.filemode false

Method 1: set up development environment directly
-------------------------------------------------

This is only recommended if you know what you are doing with respect to 
setting up your own environments and maintaining it yourself.

You can use the provisioning scripts to set up the environment, which
are designed to be run on a *freshly installed* Debian-style Linux.

For this, run all the scripts from the
``ide/provisioning`` subfolder of the XCSoar source to install the build
dependencies for various XCSoar target platforms.

::

   cd ide/provisioning
   sudo ./install-debian-packages.sh
   ./install-android-tools.sh

If all went well and there was no conflict with any other libraries or tools 
already existing on the system, you can now try to build the source
with::

   make TARGET=ANDROID

If your code does not build, you might have to check the existence and versions of 
various components of your Linux installation, such as the C++ compiler version etc.


Method 2: use Vagrant to create a virtual machine (recommended)
---------------------------------------------------------------

To avoid any compatibility or version issues, you can set up a virtual machine with
exactly the correct configuration for building XCSoar. To achieve this very easily,
you can use the tool ``Vagrant``, which makes the process run entirely automatic.

For this, you need to
- Install ``VirtualBox`` on your machine
- Install ``Vagrant``
- Enter the directory ``ide/vagrant`` and execute::
  
  vagrant up

- Vagrant will now create a VM, install all required tools, and start the VM
  to run in the background
- To connect to the VM using an ssh connection, simply run::

   vagrant ssh

- you can now build the source, for example the unix version:

   cd xcsoar-src/
   make TARGET=ANDROID

Importantly, *with this method a separate code directory and code git 
repository (the "VM repository") exists within the virtual machine.* This is in addition to
the repository (the "host repository") on the host machine,
which you created in the beginning. The host repository is configured as 
the upstream repository for the VM repository.

Therefore, if you edit the code that is stored in the **host**
machine native file system, you will first have to *push your changes into
the host repository, and use git pull inside the
VM to pull the changes* from the host repository to the VM repository. Only 
after this can you build the modified source inside the VM.

Conversely, if you change the source code inside the **VM file system** (including
via remote editing from a host IDE connecting into the VM), then you will need
to *git commit and git push inside the VM* in order to commit the changes
into the VM repository and then push the changes out of
the virtual machine into the host repository. Otherwise you will lose the 
changes once the VM is deleted or re-created for some reason.

This is different from the approach taken in the docker container 
configuration that is provided with xcsoar. In that configuration,
the build inside the docker container directly accesses the host
source directory, so only one copy of the code exists.

Method 3: use docker containers
-------------------------------

Instead of running a full virtual machine, you can also run your build 
process and execute XCSoar in a Docker container. To help you set this
up, the source code provides a dockerfile.

First, make sure you have Docker installed. We are assuming that you use
Docker Engine here, for Docker Desktop everything will be similar.

The fastest way is to use the official install script. You can get it
like this::

   curl -fsSL https://get.docker.com -o get-docker.sh

Next, execute the script as root::

   sudo sh get-docker.sh

Once Docker engine is up and running, you can create the container by 
entering the directory ``ide/docker`` and run the command::

   docker build \
    --file ide/docker/Dockerfile \
    -t xcsoar/xcsoar-build:latest ./ide/

After this, you can start a build process inside the container, or
open a shell inside the container. All of this is descirbed in the 
``README.md`` file in the ``ide/docker`` directory

Setup guide for Windows systems
===============================

Unless you want to work entirely in a virtual machine (method 3),
you will want to have ``git`` installed in Windows in order to
obtain (and keep updated) the source code on your machine in an efficient
way.


If you do not have it, download and install ``git`` from https://git-scm.com/download/win.
Some graphical git clients such as ``Sourcetree`` already include a version of git.


Also, at this point, in Windows, if this is your only use of git, you should consider 
setting the option ``autocrlf`` to "input", and the option ``filemode`` to "false"::

   git config --global core.autocrlf input
   git config --global core.filemode false

To download the source code of XCSoar, you create the code tree and the host 
repository for the code by executing ``git`` in the
following way in your project directory::

 git clone --recursive https://github.com/XCSoar/XCSoar

Next, consider making sure that your name and contact are 
set up correctly if you are considering to later provide your 
work as contributions to the community by pushing to ``github``
using the *Windows*-installed version of ``git``.
(This is only necessary if you work on source files
from within your windows environment, and not solely from within 
the VM: 
Inside the 
VM or container there will be another installation of ``git``,
for which you should definitely set username and email. If you are
unsure, just do it for both).

You can enter::

   git config --get-all user.email
   git config --get-all user.name

To check the currently configured user and contact address. If they
are not correct, you can change them with::

   git config user.name "My Name"
   git config user.email "my@email.com"

This example will change the setting only for this repository. 
By adding the ``--global`` flag, you can set it for your
global git installation.

Also, if you did not change the global ``autcrlf`` setting,
you can now also change it locally (only for this new repository), with::

   git config core.autocrlf input
   git config core.filemode false



Windows method 1: virtual machine setup with Vagrant (recommended)
---------------------------------------------------------------------------

Here we use the Vagrant tool, to automatically set up and configure a 
virtual machine (VM) in VirtualBox (Other VM managers are also possible,
but require more configuration of Vagrant). 

Prerequisites
^^^^^^^^^^^^^

Make sure the following components are installed on your computer (the "host machine")

 - VirtualBox (Oracle)
 - Vagrant (HashiCorp)
 - A command line SSH client (for example openSSH)

Create and run your virtual machine
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Open a command prompt and enter the xcsoar code base directory; then tell vagrant
to set up and start your VM::
   
   cd ide/vagrant
   vagrant up

This will take a while, and needs at least 12 GB of disk space (building the
source for all possible targets will increase this by several GB).

To access your new VM and get to a command prompt inside, type::

   vagrant ssh

Now, as mentioned earlier, you should set your username and contact address inside
the VM if you want to be able to contribute to the community by pushing code upstream.
For this, run the config commands again, now inside the VM::

   git config --global user.name "My Name"
   git config --global user.email "my@email.com"

Now, you are ready to build the code! (But not yet ready to *run* xcsoar on your computer, 
that requires more configuration, see below).

To build the code e.g. for Android, run:

   cd xcsoar-src/
   make -j 8 TARGET=ANDROID

Here, the ``-j 8`` option tells ``make`` to run eight processes in parallel
to speed up building the code.

Do not lose your work!
^^^^^^^^^^^^^^^^^^^^^^

Importantly, *with this method a separate code directory and code git 
repository (the "VM repository") exists within the virtual machine.* This is in addition to
the first repository (the "host repository") on the host machine,
which you created in the beginning. The host repository is configured as 
the upstream repository for the VM repository.

Therefore, if you edit the code that is stored in the **host**
machine native file system, you will first have to *push your changes into
the host repository, and use git pull inside the
VM to pull the changes* from the host repository to the VM repository. Only 
after this can you build the modified source inside the VM.

Conversely, if you change the source code inside the **VM file system** (including
via remote editing from a host IDE connecting into the VM), then you will need
to *git commit and git push inside the VM* in order to commit the changes
into the VM repository and then push the changes out of
the virtual machine into the host repository. Otherwise you will lose the 
changes once the VM is deleted or re-created for some reason.

This is different from the approach taken in the docker container 
configuration that is provided with xcsoar. In that configuration,
the build inside the docker container directly accesses the host
source directory, so only one copy of the code exists.

Run xcsoar inside VM
^^^^^^^^^^^^^^^^^^^^

In order to run the UNIX version of xcsoar inside the container,
you need to configure X11 forwarding from the VM to the host computer,
and have an X11 server running.

For this, install and run the ``VcXsrv`` X11-Server (or another X11-Server of your choice). It should show an 
"X"-Symbol in the taskbar.
Next, inside your VM, make sure that X11 forwarding is configured correctly.
Open the configuration file with::

   sudo nano /etc/ssh/sshd_config

And make sure the following settings are set::

   AddressFamily inet
   X11Forwarding yes

Then, re-start the ssh components::

   sudo systemctl restart ssh
   sudo systemctl restart ssh

Now you can close the ssh session.
In Windows, set the environment variable ``DISPLAY=localhost:0.0``, for
example with::

   set DISPLAY=localhost:0.0

(Or set it globally in your windows setup).
Finally, you can start the ssh session into the VM again, with::

   vagrant ssh -- -X 

Build and start xcsoar (after successful build)::

   cd xcsoar-src
   make -j 8 TARGET=UNIX
   ./output/UNIX/bin/xcsoar

(Here, the ``-j 8`` option tells ``make`` to run eight processes in parallel).



Windows method 2: use a VM directly
-----------------------------------

In this concept, simply use any VM manger such as VMware, VirtualBox
or Hyper-V in order to create a virtual machine into which you install
a Linux variant of your choice - preferably Debian or one of its many
variants such as Ubuntu. In this case, you do not even need git or
any other tool on the Windows host, only inside the VM.

Once the OS is installed, start it and clone the xcsoar repository inside the VM.
Then proceed as described before with the "direct installation" for Linux.

After this, you use the Linux user interface as displayed by the VM software directly, 
working on your code using Linux tools and commands.


Windows method 3: use docker containers in WSL
------------------------------------------------

In principle, Windows provides the Windows Subsystem for Linux (WSL & WSL2),
but it is not very straightforward to directly build XCSoar inside WSL.

However, the docker container solution provided for xcsoar can be used using
Docker Desktop for Windows in connection with WSL.

For this, you need to make sure that WSL in installed in your Windows setup. 
In addition you have to download and install Docker Desktop for Windows.

Enter the directory ide/docker and start a container, which gets configured 
by the dockerfile.


Which method should I use?
==========================

**If you use Linux as OS on your computer:** Consider still using Vagrant to
create a dedicated VM for building XCSoar, to ensure all 
library versions, Android tools and dependencies are set up to matche
the current version of the source code (method 3), 
or, alternatively, to use the preconfigured container for the same reason 
(method 2).

To obtain maximum performance in building the code and when working on the 
code, you should use Linux directly (method 1). 


**If you use Windows as fundamental OS:** Unless you have special
requirements or an unusual setup, using a virtual machine with
Vagrant is probably the fastest and easiest to set up (Windows method 1).

For maximum comptibility with additional (Linux-based) tools, or if you have 
a lot of Linux knowledge, consider creating and using a VM 
directly (method 2).

If you are already using docker for other purposes, then using the docker 
method might be most straightforward for you (method 3)


Optional: Eclipse IDE
=====================

Another very widespread IDEs is eclipse. It is not limited to
Android, can be used for all targets, and will support C++ and Java 
simultaneously. It is not required for XCSoar, but
its installation is described here as an example. Eclipse is quite
heavyweight, based on Java and has many functions and extensions
not discussed here.

To install, download the eclipse installer (Sometimes called
“*Ooomph!*” for some reason) from here:
``https://www.eclipse.org/downloads/``

Important: Install the CDT version of eclipse for C development, not the
Android/Java package, even if you plan developing for Android. In
addition, it is very convenient to install the git support (egit).

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
----------------------------------------------------

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

Inside TeXStudio, open the file ``XCSoar-manual.tex`` (or one of
the other root files) and right-click on this file to “set as explicit
root document”, in the structure view on the left. Now you are good to
go.  Make changes and press F5 to see the result immediately.
