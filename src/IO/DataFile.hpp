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

#ifndef XCSOAR_IO_DATA_FILE_HPP
#define XCSOAR_IO_DATA_FILE_HPP

#include "Charset.hpp"

#include <memory>

#include <tchar.h>

template<class T> class Source;
class TLineReader;
class NLineReader;
class TextWriter;

/**
 * Opens a file from the data directory.
 *
 * Throws exception on error.
 *
 * @param name the file name relative to the data directory
 */
std::unique_ptr<Source<char>>
OpenDataFile(const TCHAR *name);

/**
 * Opens a text file from the data directory.
 *
 * Throws exception on error.
 *
 * @param name the file name relative to the data directory
 * @param cs the character set of the input file
 */
std::unique_ptr<TLineReader>
OpenDataTextFile(const TCHAR *name, Charset cs=Charset::UTF8);

/**
 * Opens a text file from the data directory.
 *
 * Throws exception on error.
 *
 * @param name the file name relative to the data directory
 */
std::unique_ptr<NLineReader>
OpenDataTextFileA(const TCHAR *name);

/**
 * Creates a text file in the data directory.  If the file already
 * exists, it is truncated, unless "append" is true.
 *
 * Throws exception on error.
 */
std::unique_ptr<TextWriter>
CreateDataTextFile(const TCHAR *name, bool append=false);

#endif
