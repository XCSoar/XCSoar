// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileMapping.hpp"
#include "lib/fmt/PathFormatter.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "lib/fmt/SystemError.hxx"
#include "system/Path.hpp"

#ifdef HAVE_POSIX
#include "UniqueFileDescriptor.hxx"
#include "Open.hxx"

#include <sys/mman.h>
#include <sys/stat.h>
#else
#include <fileapi.h>
#include <handleapi.h> // for INVALID_HANDLE_VALUE
#include <winbase.h> // for CreateFileMapping(), UnmapViewOfFile()
#endif

FileMapping::FileMapping(Path path)
{
#ifdef HAVE_POSIX
  auto fd = OpenReadOnly(path.c_str());

  struct stat st;
  if (fstat(fd.Get(), &st) < 0)
    throw FmtErrno("Failed to stat {}", path);

  /* mapping empty files can't be useful, let's make this a failure */
  if (st.st_size <= 0)
    throw FmtRuntimeError("File empty: {}", path);

  /* file is too large */
  if (st.st_size > 1024 * 1024 * 1024)
    throw FmtRuntimeError("File too large: {}", path);

  const std::size_t size = (std::size_t)st.st_size;

  void *data = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd.Get(), 0);
  if (data == (void *)-1)
    throw FmtErrno("Failed to map {}", path);

  madvise(data, size, MADV_WILLNEED);
#else /* !HAVE_POSIX */
  hFile = ::CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                       nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) [[unlikely]]
    throw FmtLastError("Failed to open {}", path);

  BY_HANDLE_FILE_INFORMATION fi;
  if (!::GetFileInformationByHandle(hFile, &fi)) {
    const auto e = GetLastError();
    ::CloseHandle(hFile);
    throw FmtLastError(e, "Failed to open {}", path);
  }

  if (fi.nFileSizeHigh > 0 ||
      fi.nFileSizeLow > 1024 * 1024 * 1024) {
    ::CloseHandle(hFile);
    throw FmtRuntimeError("File too large: {}", path);
  }

  const std::size_t size = (std::size_t)fi.nFileSizeLow;

  hMapping = ::CreateFileMapping(hFile, nullptr, PAGE_READONLY,
                                 fi.nFileSizeHigh, fi.nFileSizeLow,
                                 nullptr);
  if (hMapping == nullptr) [[unlikely]] {
    const auto e = GetLastError();
    ::CloseHandle(hFile);
    throw FmtLastError(e, "Failed to map {}", path);
  }

  void *data = ::MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, size);
  if (data == nullptr) {
    const auto e = GetLastError();
    ::CloseHandle(hMapping);
    ::CloseHandle(hFile);
    throw FmtLastError(e, "Failed to map {}", path);
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
