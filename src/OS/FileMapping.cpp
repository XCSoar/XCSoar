/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "FileMapping.hpp"
#include "Path.hpp"

#ifdef HAVE_POSIX
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#else
#include <windows.h>
#endif

FileMapping::FileMapping(Path path)
  :m_data(nullptr)
#ifndef HAVE_POSIX
  , hMapping(nullptr)
#endif
{
#ifdef HAVE_POSIX
  int flags = O_RDONLY;
#ifdef O_NOCTTY
  flags |= O_NOCTTY;
#endif
#ifdef O_CLOEXEC
  flags |= O_CLOEXEC;
#endif

  int fd = open(path.c_str(), flags);
  if (fd < 0)
    return;

  struct stat st;
  if (fstat(fd, &st) < 0 ||
      /* mapping empty files can't be useful, let's make this a
         failure */
      st.st_size <= 0 ||
      /* file is too large */
      st.st_size > 1024 * 1024 * 1024) {
    close(fd);
    return;
  }

  m_size = (size_t)st.st_size;

  m_data = mmap(nullptr, m_size, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  if (m_data == nullptr)
    return;

  madvise(m_data, m_size, MADV_WILLNEED);
#else /* !HAVE_POSIX */
  hFile = ::CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                       nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (gcc_unlikely(hFile == INVALID_HANDLE_VALUE))
    return;

  BY_HANDLE_FILE_INFORMATION fi;
  if (!::GetFileInformationByHandle(hFile, &fi) ||
      fi.nFileSizeHigh > 0 ||
      fi.nFileSizeLow > 1024 * 1024 * 1024)
    return;

  m_size = fi.nFileSizeLow;

  hMapping = ::CreateFileMapping(hFile, nullptr, PAGE_READONLY,
                                 fi.nFileSizeHigh, fi.nFileSizeLow,
                                 nullptr);
  if (gcc_unlikely(hMapping == nullptr))
    return;

  m_data = ::MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, m_size);
#endif /* !HAVE_POSIX */
}

FileMapping::~FileMapping()
{
#ifdef HAVE_POSIX
  if (m_data != nullptr)
    munmap(m_data, m_size);
#else /* !HAVE_POSIX */
  if (m_data != nullptr)
    ::UnmapViewOfFile(m_data);

  if (hMapping != nullptr)
    ::CloseHandle(hMapping);

  if (hFile != INVALID_HANDLE_VALUE)
    ::CloseHandle(hFile);
#endif /* !HAVE_POSIX */
}
