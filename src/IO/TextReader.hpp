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

#ifndef XCSOAR_IO_TEXT_READER_HPP
#define XCSOAR_IO_TEXT_READER_HPP

#include "ReusableArray.hpp"

#include <stdio.h>

#ifdef _UNICODE
#include <tchar.h>
#endif

class TextReader {
private:
#ifdef _UNICODE
  ReusableArray<TCHAR> tbuffer;
#endif

public:
  virtual ~TextReader();

protected:
  virtual char *read_raw_line() = 0;

public:
  /**
   * Determins the size of the file.  Returns -1 if the size is
   * unknown.
   */
  virtual long size() const;

  /**
   * Determins the current position within the file.  Returns -1 if
   * this is unknown.
   */
  virtual long tell() const;

#ifdef _UNICODE
  TCHAR *read_tchar_line();
#else
  char *read_tchar_line() {
    return read_raw_line();
  }
#endif
};

/**
 * Line-based reader for text files.  Currently, it is limited to
 * reading UTF-8 characters.
 */
class FileTextReader : public TextReader {
private:
  FILE *file;

  long the_size;

  ReusableArray<char> buffer;

public:
  FileTextReader(const char *path);

#ifdef _UNICODE
  FileTextReader(const TCHAR *path);
#endif

  virtual ~FileTextReader();

  /**
   * Returns true if opening the file has failed.  This must be
   * checked before calling any other method.
   */
  bool error() const {
    return file == NULL;
  }

protected:
  virtual char *read_raw_line();

public:
  virtual long size() const;
  virtual long tell() const;
};

#endif
