// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
};

class NLineReader : public LineReader<char> {};
