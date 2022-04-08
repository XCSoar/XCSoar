##############
Best Practises
##############

This chapter describes how to set up a development environment as simple as possible.
It is just one option to get a running XCSoar development environment.


Hardware/Operating System
=========================

Use a linux debian 11 host (such as Ubuntu 21).

Performance Considerations
--------------------------
 - Use a desktop PC rather than a notebook.
 - Don't use a virtual machine if you have performance issues (docker is no problem).
 - Example: On a middle class windows notebook using Oracle Virtual Machine, the complete XCSoar 
   build (``make TARGET=UNIX`` ) took more than one hour. The same build on a strong i7 based Desktop 
   took ~2 minutes.  

Setting it Up
=============

 * Software

   The easiest way to set up a working build environment is to use Docker_.
   
.. _Docker: docker.html

 
 * Use eclipse_ as your IDE.

.. _eclipse: devsetup.html#optional-eclipse-ide

 * Downloading the source code.
   Use git for downloading the software as described here_

.. _here: devsetup.html#download-source-code

 * For debugging you will need a working local build, because the debugger needs all the include files.
   Proceed as described in `Setting up a development environment based on linux`__


.. _Setup: devsetup.html

__ Setup_

Build/Run for other Platforms
=============================
Use Docker_ for other platform builds.
   
.. _Docker: docker.html


 * Run on Windows
 
   - Configure samba for your XCSoar folder
   - configure ssh_ to run without password
   - Map the shared folder as a drive (E.g. X:)::

       net use X: \\ubuntuhost\XCSoar -P

   - On the windows machine run ``output/PC/bin/XCSoar.exe`` from the shared folder


 * Debug on Windows
 
   The easiest approach ist to use the docker image on the linux host to build the app.
 
 
     - Install Microsoft Visual Code
     - Install the C_ ++ Addon

.. _C: https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools
   
     - Install MingW_ 
     - Configure a launch.json in XCSoars .vscode directory::
   
         {
           // Verwendet IntelliSense zum Ermitteln möglicher Attribute.
           // Zeigen Sie auf vorhandene Attribute, um die zugehörigen Beschreibungen anzuzeigen.
           // Weitere Informationen finden Sie unter https://go.microsoft.com/fwlink/?linkid=830387
           "version": "0.2.0",
           "configurations": [
            {
              "name": "gcc.exe build and debug active file",
              "type": "cppdbg",
              "request": "launch",
              "program": "X:/output/PC/bin/XCSoar-ns.exe",
              "args": [],
              "stopAtEntry": false,
              "cwd": "X:/",
              "environment": [],
              "externalConsole": false,
              "MIMode": "gdb",
              "miDebuggerPath": "C:/MinGW/bin/gdb.exe",
              "miDebuggerArgs": "--directory=X:/src",
              "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
              ],
              "preLaunchTask": "build"
            }
           ]
         }


     - Configure a tasks.json::

         {
         // See https://go.microsoft.com/fwlink/?LinkId=733558
         // for the documentation about the tasks.json format
          "version": "2.0.0",
          "tasks": [
                {
                    "label": "build",
                    "type": "process",
                    "group": "build",
                    "options": {
                        "cwd": "C:\\Users\\${yourUser}\\"
                    },
                    "command": "C:\\Users\\${yourUser}\\.vscode\\make.bat",
                    "args": ["PC"],
                    "problemMatcher": []
                }]
         }

       
   - Create make.bat::

       
         ssh yourUser@yourUbuntuhost cd /opt/XCSoar;docker run ^
         --mount type=bind,source="$(pwd)",target=/opt/xcsoar ^
         ghcr.io/xcsoar/xcsoar/xcsoar-build:latest xcsoar-compile %*


.. _MingW: https://sourceforge.net/projects/mingw/
.. _ssh: https://howchoo.com/linux/ssh-login-without-password


   


 