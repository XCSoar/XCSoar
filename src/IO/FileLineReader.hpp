/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef XCSOAR_IO_FILE_LINE_READER_HPP
#define XCSOAR_IO_FILE_LINE_READER_HPP

#include "FileSource.hpp"
#include "LineSplitter.hpp"
#include "ConvertLineReader.hpp"

/**
 * Glue class which combines FileSource and LineSplitter, and provides
 * a public NLineReader interface.
 */
class FileLineReaderA : public NLineReader {
protected:
  FileSource file;
  LineSplitter splitter;

public:
  FileLineReaderA(const char *path)
    :file(path), splitter(file) {}
#ifdef _UNICODE
  FileLineReaderA(const TCHAR *path)
    :file(path), splitter(file) {}
#endif

  bool error() const {
    return file.error();
  }

public:
  virtual char *read();
  virtual long size() const;
  virtual long tell() const;
};

#ifdef _UNICODE

/**
 * Glue class which combines FileSource, LineSplitter and
 * ConvertLineReader, and provides a public TLineReader interface.
 */
class FileLineReader : public TLineReader {
protected:
  FileSource file;
  LineSplitter splitter;
  ConvertLineReader convert;

public:
  FileLineReader(const char *path,
                 ConvertLineReader::charset cs=ConvertLineReader::UTF8)
    :file(path), splitter(file), convert(splitter, cs) {}
#ifdef _UNICODE
  FileLineReader(const TCHAR *path,
                 ConvertLineReader::charset cs=ConvertLineReader::UTF8)
    :file(path), splitter(file), convert(splitter, cs) {}
#endif

  bool error() const {
    return file.error();
  }

public:
  virtual TCHAR *read();
  virtual long size() const;
  virtual long tell() const;
};

#else

class FileLineReader : public FileLineReaderA {
public:
  FileLineReader(const char *path,
                 ConvertLineReader::charset cs=ConvertLineReader::UTF8)
    :FileLineReaderA(path) {}
};

#endif

#endif
