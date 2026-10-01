System tools
============

The optional ``Setup / System Tools`` page runs installed Linux maintenance
jobs.  It requires ``USE_POLL_EVENT=y`` (X11, Wayland or direct display);
SDL builds do not include it.  Jobs cannot start while flying.

Installation
------------

Install an executable ``<id>`` and descriptor ``<id>.tool`` together:

* System: ``/usr/libexec/xcsoar/system-tools``.
* Administrator: ``/usr/local/libexec/xcsoar/system-tools``.
* Personal: ``tools`` below XCSoar's active data directory.

The build variables ``SYSTEM_TOOLS_DIRECTORY`` and
``LOCAL_SYSTEM_TOOLS_DIRECTORY`` override the absolute system paths.
Personal descriptors may also use ``<id>.sh``; if both executable names
exist, the entry is rejected.  Scripts need an interpreter line such as
``#!/bin/sh``.  Commands run as XCSoar's user, including root when applicable.

System directories and their parents must be root-owned and not writable by
group/others.  Personal directories must belong to XCSoar's effective user;
their parents may also belong to root.  Personal paths may be group-writable
only when their group matches XCSoar's effective group: members of that group
are trusted to modify commands.  World write is rejected, except on sticky
parent directories such as ``/tmp``.  Descriptors and executables must be
regular files with the required owner.  Symlinks are rejected.  Executables
require owner execute permission and must not have setuid/setgid bits.
Duplicate system IDs are all rejected.  A trusted personal command overrides
a system command with the same ID.
IDs start with an ASCII letter or digit and contain only letters, digits,
``-``, ``_`` and ``.`` (maximum 64 characters).  Each directory and the merged
catalogue are limited to 64 tools.
An overflowing personal catalogue is ignored without hiding system tools.

Descriptor
----------

UTF-8, maximum 64 KiB, one ``key=value`` per line.  Blank lines and ``#``
comments are ignored.  The four required fields are:

.. code-block:: ini

   name=Calibrate sensors
   description=Calibrate the pressure sensors
   instructions=Place the device level and keep it still.
   mode=job

Optional fields:

* ``name[de]``, ``description[de]``, ``instructions[de]``: localized text;
  unqualified fields remain mandatory as fallback.
* ``confirmation=required`` (default) or ``none``.  Use ``none`` only for
  harmless information reports.
* ``input=none`` (default); other input kinds are rejected.

Unknown fields are ignored.  ``mode=job`` runs alongside XCSoar;
``mode=exclusive`` requires a packaged system tool and the launcher below.
Personal commands cannot use exclusive mode.

Execution and backup
--------------------

Jobs run directly without a shell or arguments, with stdin at ``/dev/null``.
Combined stdout/stderr appears as bounded UTF-8 output; exit status zero
means success.  ASCII controls other than newline, carriage return and tab
are removed from job output and exclusive result messages.  Cancellation
sends SIGTERM to the process group, then SIGKILL after ten seconds, including
descendants which outlive the helper.
Cancellation closes the dialog by this deadline, even if the child remains
blocked in the kernel; its exit status is collected asynchronously.  Output
is read until EOF or the deadline; unread output may then be discarded.
Helpers must clean up services and hardware on SIGTERM.  Background tasks
must detach and close inherited streams; their lifecycle is the helper's
responsibility.

Data backups include personal commands.  POSIX restore preserves ordinary
permissions but strips special and group/world write bits, creates safe
catalogue directories without broadening existing access, and leaves
ownership with the restoring user.
Restore as XCSoar's user and review commands before running them.
Other restored data retains normal non-executable permissions.

Exclusive launcher
------------------

1. Create a private runtime directory owned by XCSoar's user, without
   group/world write permission.  Parents must belong to root or XCSoar's
   user and reject group/world write, except sticky parents such as ``/tmp``.
   Export its absolute path as
   ``XCSOAR_EXCLUSIVE_HANDOFF_DIR``; otherwise exclusive tools stay hidden.
2. XCSoar atomically writes ``request`` containing ``<id>`` and a newline,
   then shuts down normally, saving its flight log, profile and task.
3. Wait for complete exit.  Consume the request before execution; map its
   ID to a known packaged executable, never a shell command.  Remove stale
   requests at boot.
4. Run the helper with exclusive display/input access.  It must provide
   cancellation or a timeout.  Launch errors, signals and nonzero exits
   must produce a failure result.
5. Atomically write ``result``: ID and ``success`` or ``failure`` on the
   first two newline-terminated lines, followed by optional pilot-facing
   UTF-8 text.  Maximum 8192 bytes; regular file, owned by XCSoar's user,
   without group/world write permission.
6. Restart XCSoar with the same arguments, data path, terminal/display setup
   and handoff environment.  XCSoar consumes and deletes the result, then
   displays its message and completion state once.
