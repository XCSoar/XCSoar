// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FileReader.hxx"
#include "BufferedReader.hxx"
#include "ConvertLineReader.hpp"

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
  long GetSize() const override;
  long Tell() const override;
};

class FileLineReader : public ConvertLineReader {
public:
  /**
   * Throws std::runtime_errror on error.
   */
  FileLineReader(Path path, Charset cs=Charset::UTF8)
    :ConvertLineReader(std::make_unique<FileLineReaderA>(path), cs) {}

  /**
   * Rewind the file to the beginning.
   */
  void Rewind() {
    ((FileLineReaderA &)GetSource()).Rewind();
  }
};
