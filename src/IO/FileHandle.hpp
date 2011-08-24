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

#ifndef XCSOAR_IO_FILE_HANDLE_HPP
#define XCSOAR_IO_FILE_HANDLE_HPP

#include <assert.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef _UNICODE
#include <tchar.h>
#endif

class FileHandle {
private:
  FILE *file;

public:
  FileHandle(const char *path, const char *mode) {
    file = fopen(path, mode);
  }

#ifdef _UNICODE
  FileHandle(const TCHAR *path, const TCHAR *mode) {
    file = _tfopen(path, mode);
  }
#endif

  ~FileHandle() {
    if (IsOpen())
      fclose(file);
  }

  /**
   * Returns true if the file is open and waiting for further actions.
   * This must be checked before calling any other method.
   */
  bool IsOpen() const {
    return file != NULL;
  }

  /**
   * Ensure that all pending writes have been passed to the operating
   * system.  This does not guarantee that these have been written to
   * the physical device; they might still reside in the filesystem
   * cache.
   */
  bool Flush() {
    assert(file != NULL);
    return fflush(file) == 0;
  }

  /** Writes a character to the file */
  int Write(int ch) {
    assert(file != NULL);
    return fputc(ch, file);
  }

  /** Writes a NULL-terminated string to the file */
  int Write(const char *s) {
    assert(s != NULL);
    assert(file != NULL);
    return fputs(s, file);
  }

#ifdef _UNICODE
  int Write(const TCHAR *s) {
    assert(s != NULL);
    assert(file != NULL);
    return _fputts(s, file);
  }
#endif

  /** Writes a block of data to the file */
  size_t Write(const void *s, size_t size, size_t length) {
    assert(s != NULL);
    assert(file != NULL);
    return fwrite(s, size, length, file);
  }

  void WriteFormatted(const char *format, va_list ap) {
    assert(format != NULL);
    assert(file != NULL);
    vfprintf(file, format, ap);
  }

  void WriteFormatted(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    WriteFormatted(format, ap);
    va_end(ap);
  }

#ifdef _UNICODE
  void WriteFormatted(const TCHAR *format, va_list ap) {
    assert(format != NULL);
    assert(file != NULL);
    _vftprintf(file, format, ap);
  }

  void WriteFormatted(const TCHAR *format, ...) {
    va_list ap;

    va_start(ap, format);
    WriteFormatted(format, ap);
    va_end(ap);
  }
#endif
};

#endif
