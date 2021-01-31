/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Process.hpp"
#include "util/Compiler.h"

#ifdef HAVE_POSIX

#include <cassert>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

static bool
UnblockAllSignals()
{
  sigset_t ss;
  sigemptyset(&ss);
  return sigprocmask(SIG_SETMASK, &ss, nullptr) == 0;
}

static pid_t
ForkExec(const char *const*argv)
{
  const pid_t pid = fork();
  if (pid == 0) {
    UnblockAllSignals();
    execv(argv[0], const_cast<char **>(argv));
    _exit(1);
  }

  return pid;

}

static bool
Wait(pid_t pid)
{
  assert(pid > 0);

  int status;
  pid_t pid2 = waitpid(pid, &status, 0);
  if (pid2 <= 0)
    return false;

  if (WIFSIGNALED(status) || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
    return false;

  return true;
}

bool
Start(const char *const*argv)
{
  /* double fork to detach from this process */
  const pid_t pid = fork();
  if (gcc_unlikely(pid < 0))
    return false;

  if (pid == 0)
    _exit(ForkExec(argv) ? 0 : 1);

  return Wait(pid);
}

bool
Run(const char *const*argv)
{
  const pid_t pid = ForkExec(argv);
  return pid > 0 && Wait(pid);
}

#endif
