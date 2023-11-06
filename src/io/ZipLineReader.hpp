// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ZipReader.hpp"
#include "BufferedLineReader.hpp"

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
};
