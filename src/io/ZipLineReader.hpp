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

#ifndef XCSOAR_IO_ZIP_LINE_READER_HPP
#define XCSOAR_IO_ZIP_LINE_READER_HPP

#include "ZipReader.hpp"
#include "BufferedReader.hxx"
#include "ConvertLineReader.hpp"

/**
 * Glue class which combines ZipReader and BufferedReader, and provides
 * a public NLineReader interface.
 */
class ZipLineReaderA : public NLineReader {
  ZipReader zip;
  BufferedReader buffered;

public:
  ZipLineReaderA(struct zzip_dir *dir, const char *path)
    :zip(dir, path), buffered(zip) {}

public:
  /* virtual methods from class NLineReader */
  char *ReadLine() override;
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

#endif
