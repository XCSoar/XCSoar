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

#include "FileMapping.hpp"
#include "Path.hpp"
#include "Error.hxx"
#include "util/RuntimeError.hxx"

#ifdef HAVE_POSIX
#include "io/UniqueFileDescriptor.hxx"
#include "io/Open.hxx"

#include <sys/mman.h>
#include <sys/stat.h>
#else
#include "ConvertPathName.hpp"

#include <windows.h>
#endif

FileMapping::FileMapping(Path path)
{
#ifdef HAVE_POSIX
  auto fd = OpenReadOnly(path.c_str());

  struct stat st;
  if (fstat(fd.Get(), &st) < 0)
    throw FormatErrno("Failed to stat %s", path.c_str());

  /* mapping empty files can't be useful, let's make this a failure */
  if (st.st_size <= 0)
    throw FormatRuntimeError("File empty: %s", path.c_str());

  /* file is too large */
  if (st.st_size > 1024 * 1024 * 1024)
    throw FormatRuntimeError("File too large: %s", path.c_str());

  m_size = (size_t)st.st_size;

  m_data = mmap(nullptr, m_size, PROT_READ, MAP_SHARED, fd.Get(), 0);
  if (m_data == nullptr)
    throw FormatErrno("Failed to map %s", path.c_str());

  madvise(m_data, m_size, MADV_WILLNEED);
#else /* !HAVE_POSIX */
  hFile = ::CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                       nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (gcc_unlikely(hFile == INVALID_HANDLE_VALUE))
    throw FormatLastError("Failed to open %s",
                          (const char *)NarrowPathName(path));

  BY_HANDLE_FILE_INFORMATION fi;
  if (!::GetFileInformationByHandle(hFile, &fi)) {
    const auto e = GetLastError();
    ::CloseHandle(hFile);
    throw FormatLastError(e, "Failed to open %s",
                          (const char *)NarrowPathName(path));
  }

  if (fi.nFileSizeHigh > 0 ||
      fi.nFileSizeLow > 1024 * 1024 * 1024) {
    ::CloseHandle(hFile);
    throw FormatRuntimeError("File too large: %s", path.c_str());
  }

  m_size = fi.nFileSizeLow;

  hMapping = ::CreateFileMapping(hFile, nullptr, PAGE_READONLY,
                                 fi.nFileSizeHigh, fi.nFileSizeLow,
                                 nullptr);
  if (gcc_unlikely(hMapping == nullptr)) {
    const auto e = GetLastError();
    ::CloseHandle(hFile);
    throw FormatLastError(e, "Failed to map %s",
                          (const char *)NarrowPathName(path));
  }

  m_data = ::MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, m_size);
  if (m_data == nullptr) {
    const auto e = GetLastError();
    ::CloseHandle(hMapping);
    ::CloseHandle(hFile);
    throw FormatLastError(e, "Failed to map %s",
                          (const char *)NarrowPathName(path));
  }
#endif /* !HAVE_POSIX */
}

FileMapping::~FileMapping() noexcept
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
