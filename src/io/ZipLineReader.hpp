// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ZipReader.hpp"
#include "BufferedLineReader.hpp"
#include "ConvertLineReader.hpp"

class WithZipReader {
protected:
  ZipReader zip;

  WithZipReader(struct zzip_dir *dir, const char *path)
    :zip(dir, path) {}
};

/**
 * Glue class which combines ZipReader and BufferedReader, and provides
 * a public NLineReader interface.
 */
class ZipLineReaderA : WithZipReader, public BufferedLineReader {
public:
  ZipLineReaderA(struct zzip_dir *dir, const char *path)
    :WithZipReader(dir, path), BufferedLineReader(zip) {}

public:
  /* virtual methods from class NLineReader */
  long GetSize() const override;
  long Tell() const override;
};

class ZipLineReader : public ConvertLineReader {
public:
  /**
   * Throws std::runtime_errror on error.
   */
  ZipLineReader(struct zzip_dir *dir, const char *path,
                Charset cs=Charset::UTF8)
    :ConvertLineReader(std::make_unique<ZipLineReaderA>(dir, path), cs) {}
};
