************************************
Setting up a development environment
************************************

This describes the setup of a development environment suitable to
build XCSoar for the various target platforms (such as Android, KOBO, 
UNIX, Windows, ...). 

When Linux operating systems (OS) are mentioned in this manual, recent
releases of Debian-based flavors of GNU/Linux (including Ubuntu) are
assumed. ``sudo`` is used to execute commands with
root privileges. This is not enabled by default in Debian (but is on some
Debian-based distributions, like Ubuntu).


In the following, there is a short overview of how to set up a standard 
environment with the tools provided in the repository. More detailed,
step-by-step instructions are provided for typical scenarios in the appendix.

Build on Linux, Standard setup
==============================

Clone the git repository to your machine (for example from the official repository, 
with ``git clone --recursive https://github.com/XCSoar/XCSoar``).
On a fresh install (default: Debian), you can then run the provisioning scripts provided
in the repository in ``ide/provisioning/`` to install all necessary tools and libraries
for building XCSoar for all supported platforms.

For more detailed step-by-step instructions, see section "`Detailed Setup guides: Linux systems`_"

Build on a Windows computer (WSL2)
==================================

Windows Subsystem for Linux 2 (WSL2) works well as a tool to run the Linux build
environment without booting the entire machine into Linux. 
Set up a WSL2 instance with Debian as guest OS. Once it is running,
you can follow the instructions for Linux inside the WSL2 instance to
install all the tools and libraries.

For best native Windows user experience, you can use a Windows IDE or code editor 
with remote editing capabilities 
(VSCodium, Visual Studio Code, CLion...). In this way, you can edit the code that 
is located inside the WSL instance with an editor that runs directly in Windows.
You can also use an IDE with remote building capabilities (CLion, ...) to edit 
the code that is located on the Windows host, but is automatically 
sent to the Linux instance for building.

It is highly recommended to use the WSL / virtual machine file system for 
building the code. While it is also possible to run the build on the 
automatically mounted Windows drives directly
(such as in ``/mnt/c/Users/Username/Documents/XCSoar-Code/``), 
this is much slower, ten times or more, depending on your configuration and circumstances.

You should therefore clone or copy the modified code into the WSL virtual machine, 
or use a remote editing tool as outlined above.

For more details and step-by-step instructions, see section "`Detailed Setup guides: Windows systems`_"

Vagrant build system setup 
==========================

Vagrant is a tool to set up and manage virtual machines (VMs) that can be used 
for building the code. In the repository, a Vagrantfile is provided which sets up a VM
with all necessary tools and libraries.
This is a very convenient way to create and destroy build environments, particularly 
if you need to create new "fresh" virtual machines regularly. It is available for
Linux, Windows and MacOS.

To use it, install Vagrant on your machine. A Vagrantfile is provided in the 
``ide/vagrant`` directory. Running the ``vagrant up`` command in this directory
will create a VM with all necessary tools and libraries.

The Vagrantfile also mounts the XCSoar source directory on the host into 
the VM at ``/xcsoar-host-src`` and then uses git internally to clone that 
repository into the VM at ``$HOME/xcsoar-src``.

Building with the default commands will then build the code in that directory, 
within the VM file system, and you need to extract the result of the build afterwards
if needed. If you remote edit the code inside the VM, you need to push the changes
back to the host (or to another remote) or copy the files before destroying the instance.

*Note for Windows users*: This works very well in Windows, but Vagrant will use VirtualBox as the 
default VM manager in the current configuration, which can interfere with Hyper-V.
Oracle (maintainer of VirtualBox) actually recommends 
disabling Hyper-V in the Windows features. However, Hyper-V is required for WSL2, so you might have to
choose between using Vagrant with VirtualBox or using WSL2 for development, or edit the Vagrantfile 
to use Hyper-V as the VM manager.

Currently (Oct 2024), testing shows that WSL2 builds much faster than 
VirtualBox on Windows.

For more details and step-by-step instructions on using Vagrant, see "`Vagrant for automatic virtual machine setup`_",
and particularly on Windows, see section "`Windows: automatic build system setup with Vagrant`_"

Build in Docker container
=========================

Within the repository are also tools to set up a Docker container for building XCSoar.
This is the way that is used for the official builds, so this will get you the most
exact replication of the building and runtime environment.

With Docker installed (here: assuming Linux as main OS), there are two ways to
set up an environment for building and running XCSoar in a Docker container. You
can use the container prepared on ``ghcr.io``, or you can build the container yourself,
using the provided Dockerfile.

To download and start the prepared container::

   cd ide/docker
   docker run --mount type=bind,source="$(pwd)",target=/opt/xcsoar \
    -it ghcr.io/xcsoar/xcsoar/xcsoar-build:latest /bin/bash

You can then start a build process inside the container, or run XCSoar directly.
(For running XCSoar, you need to set up X11 forwarding. This is described in 
``ide/docker/README.md``).

For building the container locally, use ``docker build`` with the Dockerfile
in the ``ide/docker`` directory::

   docker build --file ide/docker/Dockerfile -t xcsoar/xcsoar-build:latest ./ide/

You can find more details in the ``README.md`` file in the ``ide/docker`` directory.
For more information on how to use the container on Windows, see 
section `Windows with docker: use docker containers in WSL`_.


Detailed Setup guides: Linux systems
====================================

In the following, we are assuming that your Linux distribution
is a Debian-like setup, such as Debian 12 or Ubuntu.

Install git and get source code
-------------------------------

To download the XCSoar source code, make sure you have ``git`` installed. 
In Debian, for this you typically do::

 sudo apt-get update
 sudo apt-get install git

To download the source code of XCSoar, execute ``git`` in the
following way in your project directory::

 git clone --recurse-submodules https://github.com/XCSoar/XCSoar

Remember to check that your name and contact are 
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

Also, *in Windows*, if you did not change the global ``autocrlf`` setting,
you can now also change it locally (only for this new repository), with::

   git config core.autocrlf input
   git config core.filemode false

Set up standard build environment in Linux
------------------------------------------

This will set up the environment for building XCSoar in a typical Linux
environment. This can either be the computer directly running Linux as the OS
or a virtual machine (VM) running Linux.

In the repository, provisioning scripts are provided to set up 
all tools and libraries needed for the environment. They are designed
to install everything necessary, from a freshly installed Debian-style Linux.

For this, simply run all the scripts from the
``ide/provisioning`` subfolder of the XCSoar source to install the build
dependencies for various XCSoar target platforms (you can omit the Android 
script if you do not plan to build for Android).

::

   cd ide/provisioning
   sudo ./install-debian-packages.sh
   ./install-android-tools.sh

If all went well and there was no conflict with any other libraries or tools 
already existing on the system, you can now try to build the source, for example for Android::

   make TARGET=ANDROID

Vagrant for automatic virtual machine setup
-------------------------------------------

To avoid any compatibility or version issues, you can set up a virtual machine with
exactly the correct configuration for building XCSoar, even if your main OS is not Linux,
or you cannot install the tools necessary on your main Linux OS due to 
compatibility issues.

The tool *Vagrant* together with the ''Vagrantfile'' provided in the XCSoar repository
makes the process run entirely automatically.

For this, you need to

- Install ``VirtualBox`` on your machine
- Install ``Vagrant``
- Enter the directory ``ide/vagrant`` and execute::
  
   vagrant up

- Vagrant will now create a VM, install all required tools, and start the VM
  to run in the background
- To connect to the VM using an SSH connection, simply run::

   vagrant ssh

- You can now build the source, for example the UNIX version::

   cd xcsoar-src/
   make TARGET=UNIX

Importantly, *with this method a separate code directory and code git 
repository (the "VM repository") exists within the virtual machine.* This is in addition to
the repository (the "host repository") on the host machine,
which you created in the beginning. The host repository is configured as 
the upstream repository for the VM repository.

Therefore, if you edit the code that is stored in the **host**
machine native file system, you will first have to *commit your changes into
the host repository, and use git pull inside the
VM to pull the changes* from the host repository to the VM repository. Only 
after this can you build the modified source inside the VM.

Conversely, if you change the source code inside the **VM file system** (including
via remote editing from a host IDE connecting into the VM), then you will need
to *git commit and git push inside the VM* in order to commit the changes
into the VM repository and then push the changes out of
the virtual machine into the host repository. Otherwise, you will lose the 
changes once the VM is deleted or re-created for some reason.

This is different from the approach taken in the Docker container 
configuration that is provided with XCSoar. In that configuration,
the build inside the Docker container directly accesses the host
source directory, so only one copy of the code exists.

Docker containers for building XCSoar, Step-by-step
---------------------------------------------------

Instead of running a full virtual machine, you can also run your build 
process and execute XCSoar in a Docker container. To help you set this
up, the source code provides a Dockerfile.

First, make sure you have Docker installed. We are assuming that you use
Docker Engine here; for Docker Desktop everything will be similar.

The fastest way is to use the official install script. You can get it
like this::

   curl -fsSL https://get.docker.com -o get-docker.sh

Next, execute the script as root::

   sudo sh get-docker.sh

Once Docker Engine is up and running, you can create the container by 
entering the directory ``ide/docker`` and running the command::

   docker build \
    --file ide/docker/Dockerfile \
    -t xcsoar/xcsoar-build:latest ./ide/

After this, you can start a build process inside the container, or
open a shell inside the container. All of this is described in the 
``README.md`` file in the ``ide/docker`` directory

Detailed setup guides: Windows systems
======================================

Since XCSoar can only be built with Linux tools, it is necessary to set up
some type of Linux environment on a Windows machine. There are several ways
to do this. The typical one is to use a VM manager software 
(such as *VMware*, *VirtualBox* or *Hyper-V*)
to create a virtual machine into which a Linux variant is installed.
This system can then be booted like a regular computer, with the screen output
in a window on the Windows desktop (or full screen, of course). You can
then use the Linux interface directly, and all the installation 
guides for Linux, described above, apply.

The small downside of using the virtual Linux machine is that 
you have to switch between Windows and Linux usage, 
and that you have to 
maintain a complete Linux including the graphical user interface componenents. 
It also requires using tools with different look-and-feel compared to
your Windows programs. But if you are happy with 
the Unix environment, and the user interface (UI) on the Linux desktop works well 
for you, this is a good and robust way to do it.

While that is basically the same as using a Linux machine, and therefore described by 
the Linux part of this manual, it is also possible to stay completely within the Windows
user interface, and use the Linux tools only for the build process.

The following methods instead describe how to set up such "headless" build environments
in Windows, which are more lightweight and do not require a full Linux desktop.
These VMs are only used for the building process itself (and potentially for running 
the build result in case of a UNIX build), and not for editing the code.

In those cases, a ``git`` client is also needed (or at least highly recommended) 
in the Windows OS,
to be able to easily download and manage the source code files in the Windows 
filesystem. The code can then be forwarded to, or accessed from, the Linux VM or container.

Install git and get source code
-------------------------------

Download and install ``git`` from https://git-scm.com/download/win.
Some graphical git clients such as ``Sourcetree`` already include a version of git,
which is also fine.

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
from within your Windows environment, and not solely from within 
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

Also, if you did not change the global ``autocrlf`` setting,
you can now also change it locally (only for this new repository), with::

   git config core.autocrlf input
   git config core.filemode false


Set up build environment in WSL2 (recommended)
----------------------------------------------

The Windows Subsystem for Linux 2 (WSL2) is a very good way to run the Linux build. It is
basically a simpler way to run a Linux VM based on the Hyper-V platform that comes 
with Windows, with particularly good integration into Windows. WSL version 2 is necessary,
which is available for Windows from Win 10 build number 18917 or later.

Note: Hyper-V is required for WSL2, but not compatible with VirtualBox. According to Oracle 
(maker of VirtualBox), you should not enable Hyper-V if you want to use VirtualBox.

To set up the system for building XCSoar, you can follow these steps:

- Enable Hyper-V and WSL 2 in "Windows Features".
- In the Windows command shell, update WSL2 with ``wsl --update``. Reboot your computer if a WSL update was necessary.
- Create a Debian machine in WSL2 with ``wsl --install -d Debian``.
- Check if Debian is running properly: start ``wsl`` (or ``wsl -d Debian`` to select the Debian machine if you have more than one in your WSL).
  You should see a Debian command prompt; the Linux command ``lsb_release -a`` should show the Debian version.

Now install the necessary tools and libraries for building XCSoar, using the scripts provided::

   sudo ide/provisioning/install-debian-packages.sh

- Now create a directory for the source code inside the WSL filesystem. Do not use the mounted Windows drives (this will create problems and will be extremely slow).

Now you can add the source code. You can either clone the repository from the source again, particularly if 
you do not plan to make any changes, or you can clone it from the existing repository on your Windows host.
Using your local version is recommended if you plan to make changes to the code, as you can then push
the changes back to the Windows host repository, where they will be safe in case you decide to re-create the Debian
virtual machine. This can often be useful if you want to try out different configurations or setups, or simply
if XCSoar needs to be updated substantially.

To clone from the Windows host, you can use the following command::

   git clone --recursive /mnt/c/path/to/your/XCSoar/Code

(Assuming that your XCSoar code is located in the directory ``C:\path\to\your\XCSoar\Code`` on your Windows host machine.)

To clone the repository from XCSoar directly, use the same command as before, just inside the WSL2 instance::

   git clone --recursive https://github.com/XCSoar/XCSoar

Do not forget to set ``user.name`` and ``user.email`` in the WSL2 instance, as described above, if you want to
contribute your changes back to the main XCSoar code base.

Now you can build the code, for example for UNIX, with the following command::

   make TARGET=UNIX

This will build the UNIX version of XCSoar, which you should then be able to run inside the WSL2 instance directly, with::

   ./output/UNIX/bin/xcsoar


Windows: automatic build system setup with Vagrant
--------------------------------------------------

For a basically fully automatic setup of a virtual machine in Linux, MacOS or Windows,
the Vagrant tool can be used very easily, as the necessary ``Vagrantfile`` for XCSoar is provided in the
XCSoar repository.
This uses the free Vagrant tool and the free VirtualBox VM
software from Oracle. (Other VM managers are also possible, but require 
manual configuration of Vagrant and the Vagrantfile.)

Prerequisites
^^^^^^^^^^^^^

Make sure the following components are installed on your computer (the "host machine"):

 - VirtualBox (Oracle)
 - Vagrant (HashiCorp)
 - A command line SSH client (for example OpenSSH)

Create and run your virtual machine
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Open a command prompt and enter the xcsoar code base directory; then tell Vagrant
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
   make TARGET=ANDROID

Transfer your code changes out of the VM
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Importantly, *with this method a separate code directory / repository 
(the "VM repository") exists within the virtual machine.* This is in addition to
the first repository (the "host repository") on the host machine,
which you created in the beginning. The host repository is configured as 
the upstream repository for the VM repository.

You therefore need to make sure that you transfer your code changes between
these two locations as necessary. You can copy the files manually, or use git.

If you edit the code that is stored in the **host**
machine native file system, you can transfer using git by committing your changes into
the host repository, and then use ``git pull`` inside the
VM to pull the changes from the host repository to the VM repository. Only 
after this can you build the modified source inside the VM.

Conversely, if you change the source code inside the **VM file system** (including
via a remote editing tool on the host but acting on the code inside the VM), you will need
to *git commit* your code and then *git push* it there in order to push the changes out of
the virtual machine into the host repository.

This is different from the approach taken in the *Docker container* 
configuration that is provided with XCSoar. In that configuration,
the build inside the Docker container directly accesses the host
source directory, so only one copy of the code exists.

Run xcsoar inside VM
^^^^^^^^^^^^^^^^^^^^

In order to run the UNIX version of xcsoar inside the VM,
you need to configure X11 forwarding from the VM to the host computer,
and have an X11 server running.

In the particular case of Windows, first install and run the ``VcXsrv`` X11-Server 
(or another X11-Server of your choice). It should show an 
"X"-Symbol in the taskbar.
Next, inside your VM, make sure that X11 forwarding is configured correctly.
Open the SSHd configuration file with::

   sudo nano /etc/ssh/sshd_config

And make sure the following settings are set::

   AddressFamily inet
   X11Forwarding yes

Then, re-start the SSH service::

   sudo systemctl restart ssh

Now you can close the SSH session.
In Windows, set the environment variable ``DISPLAY=localhost:0.0``, for
example with::

   set DISPLAY=localhost:0.0

(Or set it globally in your Windows setup).
Finally, you can start the SSH session into the VM again, with::

   vagrant ssh -- -X 

Build and start xcsoar (after successful build)::

   cd xcsoar-src
   make -j 8 TARGET=UNIX
   ./output/UNIX/bin/xcsoar

(Here, the ``-j 8`` option tells ``make`` to run eight processes in parallel).


Windows with docker: use docker containers in WSL
-------------------------------------------------

The Docker container solution provided for XCSoar can also be used in Windows
in order to create an exact replica of the build environment used for the
official builds.

For this, you need to make sure that WSL2 is installed in your Windows setup 
(requires Windows 11 or a recent enough version of Windows 10).
In addition, you have to download and install *Docker Desktop for Windows*.

Enter the directory ``ide/docker`` and start a container, which gets configured 
by the Dockerfile.


Which method should I use?
==========================

**If you use Linux as OS on your computer:** For highest performance 
when building, directly configure your OS with the tools needed to 
build XCSoar. 

You can  still consider using Vagrant to
create a dedicated VM for building XCSoar, to make it easier to ensure all 
library versions, Android tools and dependencies are set up to match
the current version of the source code, without worrying about what
else is installed on your computer.


Alternatively, you can use the preconfigured container for this purpose, 
particularly if you are already running a docker engine for other purposes. 
This is the closest to the build environments used in the official builds, 
and probably also the quickest to get running *if* you have a lot of disk space and 
a fast internet connection.


**If you use Windows as fundamental OS:** The default way to build XCSoar is to
use WSL2 for the Linux build environment, and then use a Windows IDE or Code editor
with remote editing capabilities to edit the code that is located inside the WSL instance.

Using Vagrant is the simplest and most convenient way to create (and manage)
build environments, at the cost of the actual build being substantially slower.

For maximum compatibility with the official version and to really be sure to have
a correctly configured runtime environment, you can use the docker container method.

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
