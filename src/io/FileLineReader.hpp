// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FileReader.hxx"
#include "LineReader.hpp"
#include "BufferedReader.hxx"
#include "system/Path.hpp"

/**
 * Glue class which combines FileReader and BufferedReader, and provides
 * a public NLineReader interface.
 */
class FileLineReaderA : public NLineReader {
  FileReader file;
  BufferedReader buffered;

public:
  /**
   * Throws std::runtime_errror on error.
   */
  explicit FileLineReaderA(Path path)
    :file(path), buffered(file) {}

  /**
   * Rewind the file to the beginning.
   */
  void Rewind() {
    file.Rewind();
    buffered.Reset();
  }

public:
  /* virtual methods from class NLineReader */
  char *ReadLine() override;
};
