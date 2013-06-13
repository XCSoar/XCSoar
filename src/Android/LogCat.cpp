/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "LogCat.hpp"
#include "LogFile.hpp"
#include "LocalPath.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/Async/IOThread.hpp"
#include "OS/FileDescriptor.hpp"

#include <atomic>

#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

class LogCatReader final : private FileEventHandler {
  IOThread &io_thread;
  FileDescriptor fd;
  std::atomic<pid_t> pid;

  std::string data;

public:
  LogCatReader(IOThread &_io_thread, FileDescriptor &&_fd, pid_t _pid)
    :io_thread(_io_thread), fd(std::move(_fd)), pid(_pid) {
    io_thread.LockAdd(fd.Get(), IOThread::READ, *this);
  }

  ~LogCatReader() {
    io_thread.LockRemove(fd.Get());
    fd.Close();

    Kill(pid.exchange(0));
  }

private:
  void Wait(pid_t pid);
  void Kill(pid_t pid);
  void Save(int pid) const;
  void EndOfFile();

  virtual bool OnFileEvent(int fd, unsigned mask) override;
};

static void
SaveCrash(int pid, const char *data, size_t length)
{
  char name[64];
  time_t t = time(NULL);
  struct tm tm;
  strftime(name, sizeof(name),
           "crash/crash-%Y-%m-%d-%H-%M-%S", gmtime_r(&t, &tm));

  char path[1024];
  LocalPath(path, name);
  unlink(path);

  LogFormat("Saving logcat to %s", path);

  const int fd = open(path, O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW|O_WRONLY,
                      0777);
  if (fd < 0)
    return;

  write(fd, data, length);
  close(fd);
}

void
LogCatReader::Wait(pid_t pid)
{
  if (pid <= 0)
    return;

  int status;
  pid_t pid2 = ::waitpid(pid, &status, 0);
  if (pid2 <= 0)
    return;

  if (WIFSIGNALED(status))
    LogFormat("logcat was killed by signal %d", WTERMSIG(status));

  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
    LogFormat("logcat has failed");
}

void
LogCatReader::Kill(pid_t pid)
{
  if (pid <= 0)
    return;

  LogFormat("Kill logcat");

  kill(pid, SIGTERM);
  Wait(pid);
}

inline void
LogCatReader::Save(int pid) const
{
  ::SaveCrash(pid, data.data(), data.length());
}

gcc_pure
static const char *
FindLine(const char *start, const char *p)
{
  while (p > start && p[-1] != '\n')
    --p;

  return p;
}

gcc_pure
static int
FindCrash(const char *p)
{
  /* see if there was a crash; this check is very simple, but I hope
     it's good enough to avoid false positives */

  const char *q = strstr(p, ">>> org.xcsoarte");
  if (q == nullptr || strstr(q, "fault addr") == nullptr)
    return 0;

  const char *r = strstr(FindLine(p, q), "pid:");
  if (r == nullptr)
    return 0;

  return atoi(r + 4);
}

inline void
LogCatReader::EndOfFile()
{
  Wait(pid.exchange(0));

  const int pid = FindCrash(data.c_str());
  if (pid > 0)
    Save(pid);
  else
    LogFormat("No crash found in logcat");

  StopLogCat();
  OnLogCatFinished(pid > 0);
}

bool
LogCatReader::OnFileEvent(int _fd, unsigned mask)
{
  assert(_fd == fd.Get());

  char buffer[1024];
  ssize_t nbytes = fd.Read(buffer, sizeof(buffer));
  if (nbytes > 0) {
    if (data.length() < 1024 * 1024)
      data.append(buffer, nbytes);
    return true;
  } else {
    EndOfFile();
    return false;
  }
}

static std::atomic<LogCatReader *> log_cat_reader;

void
CheckLogCat(IOThread &io_thread)
{
  LogFormat("Launching logcat");

  FileDescriptor r, w;
  if (!FileDescriptor::CreatePipe(r, w))
    return;

  pid_t pid = fork();
  if (pid == 0) {
    /* in the child process */
    w.Duplicate(1);

    execl("/system/bin/logcat", "logcat", "-v", "threadtime",
          "-d", "-t", "1000", NULL);
    _exit(EXIT_FAILURE);
  }

  if (pid < 0) {
    LogFormat("Launching logcat has failed: %s", strerror(errno));
    return;
  }

  log_cat_reader = new LogCatReader(io_thread, std::move(r), pid);
}

void
StopLogCat()
{
  LogCatReader *lcr = log_cat_reader.exchange(nullptr);
  delete lcr;
}
