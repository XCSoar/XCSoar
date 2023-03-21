// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

template<class T>
class LineReader {
public:
  virtual ~LineReader() {}

  /**
   * Read one line, and returns a it as a null-terminated string
   * (without the trailing line feed character).  The returned buffer
   * is writable, and may be modified by the caller while parsing it.
   * It is Invalidated by the next call.  After the last line has been
   * read, this method returns nullptr.
   *
   * Throws on error.
   */
  virtual T *ReadLine() = 0;

  /**
   * Determins the size of the file.  Returns -1 if the size is
   * unknown.
   */
  [[gnu::pure]]
  virtual long GetSize() const {
    return -1;
  }

  /**
   * Determins the current position within the file.  Returns -1 if
   * this is unknown.
   */
  [[gnu::pure]]
  virtual long Tell() const {
    return -1;
  }
};

class TLineReader : public LineReader<TCHAR> {};

#ifdef _UNICODE
class NLineReader : public LineReader<char> {};
#else
class NLineReader : public TLineReader {};
#endif
