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

#ifndef XCSOAR_IO_FILE_HANDLE_HPP
#define XCSOAR_IO_FILE_HANDLE_HPP

#include "OS/Path.hpp"

#include <algorithm>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include <tchar.h>

class FileHandle {
private:
  FILE *file;

public:
  FileHandle(Path path, const TCHAR *mode) {
    file = _tfopen(path.c_str(), mode);
  }

  FileHandle(const FileHandle &other) = delete;

  FileHandle(FileHandle &&other):file(other.file) {
    other.file = nullptr;
  }

  ~FileHandle() {
    if (IsOpen())
      fclose(file);
  }

  FileHandle &operator=(const FileHandle &other) = delete;

  FileHandle &operator=(FileHandle &&other) {
    std::swap(file, other.file);
    return *this;
  }

  /**
   * Returns true if the file is open and waiting for further actions.
   * This must be checked before calling any other method.
   */
  bool IsOpen() const {
    return file != nullptr;
  }

  /**
   * Ensure that all pending writes have been passed to the operating
   * system.  This does not guarantee that these have been written to
   * the physical device; they might still reside in the filesystem
   * cache.
   */
  bool Flush() {
    assert(file != nullptr);
    return fflush(file) == 0;
  }

  bool Seek(long offset, int whence) {
    assert(file != nullptr);
    return fseek(file, offset, whence) == 0;
  }

  long Tell() {
    assert(file != nullptr);
    return ftell(file);
  }

  size_t Read(void *ptr, size_t size, size_t nmemb) {
    assert(file != nullptr);
    return fread(ptr, size, nmemb, file);
  }

  /** Writes a character to the file */
  int Write(int ch) {
    assert(file != nullptr);
    return fputc(ch, file);
  }

  /** Writes a NULL-terminated string to the file */
  int Write(const char *s) {
    assert(s != nullptr);
    assert(file != nullptr);
    return fputs(s, file);
  }

  /** Writes a block of data to the file */
  size_t Write(const void *s, size_t size, size_t length) {
    assert(s != nullptr);
    assert(file != nullptr);
    return fwrite(s, size, length, file);
  }

  template<typename... Args>
  void WriteFormatted(const char *format, Args&&... args) {
    assert(format != nullptr);
    assert(file != nullptr);

    ::fprintf(file, format, args...);
  }
};

#endif
