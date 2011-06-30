/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "FileSource.hpp"

#ifdef HAVE_POSIX

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

PosixFileSource::PosixFileSource(const char *path)
{
  fd = ::open(path, O_RDONLY);
}

PosixFileSource::~PosixFileSource()
{
  if (fd >= 0)
    ::close(fd);
}

long
PosixFileSource::size() const
{
  struct stat st;
  return ::fstat(fd, &st) >= 0
    ? (long)st.st_size
    : -1;
}

unsigned
PosixFileSource::read(char *p, unsigned n)
{
  ssize_t nbytes = ::read(fd, p, n);
  return nbytes > 0 ? nbytes : 0;
}

#endif /* HAVE_POSIX */

#ifdef WIN32

#ifdef _WIN32_WCE
#include <syslimits.h> /* for PATH_MAX */
#endif

WindowsFileSource::WindowsFileSource(const char *path)
{
#ifdef _WIN32_WCE
  /* Windows Mobile doesn't provide narrow API functions */
  TCHAR tpath[PATH_MAX];

  int length = ::MultiByteToWideChar(CP_ACP, 0, path, -1, tpath, PATH_MAX);
  if (length == 0) {
    handle = INVALID_HANDLE_VALUE;
    return;
  }

  handle = ::CreateFile(tpath, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#else
  handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#endif
}

#ifdef _UNICODE
WindowsFileSource::WindowsFileSource(const TCHAR *path)
{
  handle = ::CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}
#endif

WindowsFileSource::~WindowsFileSource()
{
  if (handle != INVALID_HANDLE_VALUE)
    ::CloseHandle(handle);
}

long
WindowsFileSource::size() const
{
  struct {
    BY_HANDLE_FILE_INFORMATION i;

#ifdef _WIN32_WCE
    /* on Windows CE, GetFileInformationByHandle() seems to overflow
       the BY_HANDLE_FILE_INFORMATION variable by 4 bytes
       (undocumented on MSDN); adding the following DWORD gives it
       enough buffer to play with */
    DWORD dummy;
#endif
  } i;

  return ::GetFileInformationByHandle(handle, &i.i)
    ? i.i.nFileSizeLow
    : -1;
}

unsigned
WindowsFileSource::read(char *p, unsigned n)
{
  DWORD nbytes;
  if (!::ReadFile(handle, p, n, &nbytes, NULL))
    return 0;
  return nbytes;
}

#endif /* WIN32 */
