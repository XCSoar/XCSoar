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

#include "OS/FileUtil.hpp"

#include "OS/PathName.hpp"

#include <sys/types.h>
#include <sys/stat.h>

void
Directory::Create(const TCHAR* path)
{
#ifdef HAVE_POSIX
  mkdir(path, 0777);
#else /* !HAVE_POSIX */
  CreateDirectory(path, NULL);
#endif /* !HAVE_POSIX */
}

bool
Directory::Exists(const TCHAR* path)
{
#ifdef HAVE_POSIX
  struct stat st;
  if (stat(NarrowPathName(path), &st) != 0)
    return false;

  return (st.st_mode & S_IFDIR);
#else
  DWORD attributes = GetFileAttributes(path);
  return attributes != INVALID_FILE_ATTRIBUTES &&
    (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#endif
}

bool
File::Exists(const TCHAR* path)
{
#ifdef HAVE_POSIX
  struct stat st;
  if (stat(NarrowPathName(path), &st) != 0)
    return false;

  return (st.st_mode & S_IFREG);
#else
  DWORD attributes = GetFileAttributes(path);
  return attributes != INVALID_FILE_ATTRIBUTES &&
    (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
#endif
}
