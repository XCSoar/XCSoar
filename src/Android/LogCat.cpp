/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

bool
CheckLogCat()
{
  char path[1024];
  LocalPath(path, _T("crash"));
  mkdir(path, 0777);

  strcat(path, "/new");
  unlink(path);

  /* run logcat, save to a temporary file */

  LogFormat("Launching logcat");

  pid_t pid = fork();
  if (pid == 0) {
    /* in the child process */
    execl("/system/bin/logcat", "logcat", "-v", "threadtime",
          "-d", "-t", "1000", "-f", path, NULL);
    _exit(EXIT_FAILURE);
  }

  if (pid < 0) {
    LogFormat("Launching logcat has failed: %s", strerror(errno));
    unlink(path);
    return false;
  }

  int status;
  pid_t pid2 = waitpid(pid, &status, 0);
  if (pid2 < 0) {
    unlink(path);
    return false;
  }

  if (WIFSIGNALED(status)) {
    LogFormat("logcat was killed by signal %d", WTERMSIG(status));
    unlink(path);
    return false;
  }

  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    LogFormat("logcat has failed");
    unlink(path);
    return false;
  }

  /* see if there was a crash; this check is very simple, but I hope
     it's good enough to avoid false positives */

  int found_xcsoar = 0;
  bool found_crash = false;

  FileLineReaderA reader(path);
  char *line;
  while ((line = reader.read()) != NULL) {
    if (strstr(line, ">>> org.xcsoar") != NULL) {
      const char *p = strstr(line, "pid:");
      if (p != NULL)
        found_xcsoar = atoi(p + 4);
    } else if (found_xcsoar > 0 && strstr(line, "fault addr") != NULL)
      found_crash = true;
  }

  if (!found_crash) {
    /* no crash was found, delete the temporary file */
    unlink(path);
    return false;
  }

  /* crash found: build a file name and rename the temporary file */

  char final_path[1024];
  LocalPath(final_path, _T("crash"));
  strcat(final_path, "/crash-");

  time_t t = time(NULL);
  struct tm tm;
  strftime(final_path + strlen(final_path), 32,
           "%Y-%m-%d-%H-%M-%S", gmtime_r(&t, &tm));

  sprintf(final_path + strlen(final_path), "-%d.txt", found_xcsoar);
  unlink(final_path);
  rename(path, final_path);
  return true;
}
