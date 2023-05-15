// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Process.hpp"
#include "util/Compiler.h"

#ifdef HAVE_POSIX

#include <cassert>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

static bool
UnblockAllSignals() noexcept
{
  sigset_t ss;
  sigemptyset(&ss);
  return sigprocmask(SIG_SETMASK, &ss, nullptr) == 0;
}

static pid_t
ForkExec(const char *const*argv) noexcept
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
Wait(pid_t pid) noexcept
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
Start(const char *const*argv) noexcept
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
Run(const char *const*argv) noexcept
{
  const pid_t pid = ForkExec(argv);
  return pid > 0 && Wait(pid);
}

#endif
