// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileMapping.hpp"
#include "system/Path.hpp"
#include "system/Error.hxx"
#include "util/RuntimeError.hxx"

#ifdef HAVE_POSIX
#include "UniqueFileDescriptor.hxx"
#include "Open.hxx"

#include <sys/mman.h>
#include <sys/stat.h>
#else
#include "system/ConvertPathName.hpp"

#include <fileapi.h>
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

  const std::size_t size = (std::size_t)st.st_size;

  void *data = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd.Get(), 0);
  if (data == (void *)-1)
    throw FormatErrno("Failed to map %s", path.c_str());

  madvise(data, size, MADV_WILLNEED);
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

  const std::size_t size = (std::size_t)fi.nFileSizeLow;

  hMapping = ::CreateFileMapping(hFile, nullptr, PAGE_READONLY,
                                 fi.nFileSizeHigh, fi.nFileSizeLow,
                                 nullptr);
  if (gcc_unlikely(hMapping == nullptr)) {
    const auto e = GetLastError();
    ::CloseHandle(hFile);
    throw FormatLastError(e, "Failed to map %s",
                          (const char *)NarrowPathName(path));
  }

  void *data = ::MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, size);
  if (data == nullptr) {
    const auto e = GetLastError();
    ::CloseHandle(hMapping);
    ::CloseHandle(hFile);
    throw FormatLastError(e, "Failed to map %s",
                          (const char *)NarrowPathName(path));
  }
#endif /* !HAVE_POSIX */

  span = {(std::byte *)data, size};
}

FileMapping::~FileMapping() noexcept
{
#ifdef HAVE_POSIX
  munmap(span.data(), span.size());
#else /* !HAVE_POSIX */
  ::UnmapViewOfFile(span.data());
  ::CloseHandle(hMapping);
  ::CloseHandle(hFile);
#endif /* !HAVE_POSIX */
}
