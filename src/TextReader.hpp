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

#ifndef XCSOAR_TEXT_READER_HPP
#define XCSOAR_TEXT_READER_HPP

#include "ReusableArray.hpp"

#include <stdio.h>

#ifdef _UNICODE
#include <tchar.h>
#endif

/**
 * Line-based reader for text files.  Currently, it is limited to
 * reading UTF-8 characters.
 */
class TextReader {
private:
  FILE *file;

  ReusableArray<char> buffer;
#ifdef _UNICODE
  ReusableArray<TCHAR> tbuffer;
#endif

public:
  TextReader(const char *path);

#ifdef _UNICODE
  TextReader(const TCHAR *path);
#endif

  ~TextReader();

  /**
   * Returns true if opening the file has failed.  This must be
   * checked before calling any other method.
   */
  bool error() const {
    return file == NULL;
  }

protected:
  char *read_raw_line();

public:
  /**
   * Reads one line from the input file.  The returned buffer is
   * writable, but is invalidated by the next call.  The end-of-line
   * characters are stripped, but not other leading/trailing
   * whitespace.  Returns NULL on end of file or on error.
   */
  char *read_utf8_line() {
    return read_raw_line();
  }

  /**
   * Like read_raw_line(), but convert it to a TCHAR string.
   */
#ifdef _UNICODE
  TCHAR *read_tchar_line();
#else
  char *read_tchar_line() {
    return read_raw_line();
  }
#endif
};

#endif
