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

#ifndef XCSOAR_IO_ZIP_LINE_READER_HPP
#define XCSOAR_IO_ZIP_LINE_READER_HPP

#include "ZipSource.hpp"
#include "LineSplitter.hpp"
#include "ConvertLineReader.hpp"

/**
 * Glue class which combines ZipSource, LineSplitter and
 * ConvertLineReader, and provides a public TLineReader interface.
 */
class ZipLineReaderA : public NLineReader {
protected:
  ZipSource zip;
  LineSplitter splitter;

public:
  ZipLineReaderA(const char *path)
    :zip(path), splitter(zip) {}
#ifdef _UNICODE
  ZipLineReaderA(const TCHAR *path)
    :zip(path), splitter(zip) {}
#endif

  bool error() const {
    return zip.error();
  }

public:
  virtual char *read();
  virtual long size() const;
  virtual long tell() const;
};

#ifdef _UNICODE

/**
 * Glue class which combines ZipSource, LineSplitter and
 * ConvertLineReader, and provides a public TLineReader interface.
 */
class ZipLineReader : public TLineReader {
protected:
  ZipSource zip;
  LineSplitter splitter;
  ConvertLineReader convert;

public:
  ZipLineReader(const char *path,
                ConvertLineReader::charset cs=ConvertLineReader::UTF8)
    :zip(path), splitter(zip), convert(splitter, cs) {}
#ifdef _UNICODE
  ZipLineReader(const TCHAR *path,
                ConvertLineReader::charset cs=ConvertLineReader::UTF8)
    :zip(path), splitter(zip), convert(splitter, cs) {}
#endif

  bool error() const {
    return zip.error();
  }

public:
  virtual TCHAR *read();
  virtual long size() const;
  virtual long tell() const;
};

#else

class ZipLineReader : public ZipLineReaderA {
public:
  ZipLineReader(const char *path,
                 ConvertLineReader::charset cs=ConvertLineReader::UTF8)
    :ZipLineReaderA(path) {}
};

#endif

#endif
