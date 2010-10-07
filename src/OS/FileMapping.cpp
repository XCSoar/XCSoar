/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

  M Roberts (original release)
  Robin Birch <robinb@ruffnready.co.uk>
  Samuel Gisiger <samuel.gisiger@triadis.ch>
  Jeff Goodenough <jeff@enborne.f2s.com>
  Alastair Harrison <aharrison@magic.force9.co.uk>
  Scott Penrose <scottp@dd.com.au>
  John Wharington <jwharington@gmail.com>
  Lars H <lars_hn@hotmail.com>
  Rob Dunning <rob@raspberryridgesheepfarm.com>
  Russell King <rmk@arm.linux.org.uk>
  Paolo Ventafridda <coolwind@email.it>
  Tobias Lohner <tobias@lohner-net.de>
  Mirek Jezek <mjezek@ipplc.cz>
  Max Kellermann <max@duempel.org>
  Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "OS/FileMapping.hpp"
#include "Compiler.h"

#ifdef HAVE_POSIX
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#else
#include <windows.h>
#endif

FileMapping::FileMapping(const TCHAR *path)
  :m_data(NULL)
#ifndef HAVE_POSIX
  , hMapping(NULL)
#endif
{
#ifdef HAVE_POSIX
  int fd = open(path, O_RDONLY);
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

  m_data = mmap(NULL, m_size, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  if (m_data == NULL)
    return;

  madvise(m_data, m_size, MADV_WILLNEED);
#else /* !HAVE_POSIX */
  hFile = ::CreateFile(path, GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (gcc_unlikely(hFile == INVALID_HANDLE_VALUE))
    return;

  struct {
    BY_HANDLE_FILE_INFORMATION fi;

#ifdef _WIN32_WCE
    /* on Windows CE, GetFileInformationByHandle() seems to overflow
       the BY_HANDLE_FILE_INFORMATION variable by 4 bytes
       (undocumented on MSDN); adding the following DWORD gives it
       enough buffer to play with */
    DWORD dummy;
#endif
  } i;

  if (!::GetFileInformationByHandle(hFile, &i.fi) ||
      i.fi.nFileSizeHigh > 0 ||
      i.fi.nFileSizeLow > 1024 * 1024 * 1024)
    return;

  m_size = i.fi.nFileSizeLow;

  hMapping = ::CreateFileMapping(hFile, NULL, PAGE_READONLY,
                                 i.fi.nFileSizeHigh, i.fi.nFileSizeLow,
                                 NULL);
  if (gcc_unlikely(hMapping == NULL))
    return;

  m_data = ::MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, m_size);
#endif /* !HAVE_POSIX */
}

FileMapping::~FileMapping()
{
#ifdef HAVE_POSIX
  if (m_data != NULL)
    munmap(m_data, m_size);
#else /* !HAVE_POSIX */
  if (m_data != NULL)
    ::UnmapViewOfFile(m_data);

  if (hMapping != NULL)
    ::CloseHandle(hMapping);

  if (hFile != INVALID_HANDLE_VALUE)
    ::CloseHandle(hFile);
#endif /* !HAVE_POSIX */
}
