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

#ifndef XCSOAR_IO_LINE_READER_HPP
#define XCSOAR_IO_LINE_READER_HPP

#include "Compiler.h"

#include <tchar.h>

template<class T>
class LineReader {
public:
  virtual ~LineReader() {}

  /**
   * Read one line, and returns a it as a null-terminated string
   * (without the trailing line feed character).  The returned buffer
   * is writable, and may be modified by the caller while parsing it.
   * It is invalidated by the next call.  After the last line has been
   * read, this method returns NULL.
   */
  virtual T *read() = 0;

  /**
   * Determins the size of the file.  Returns -1 if the size is
   * unknown.
   */
  gcc_pure
  virtual long size() const {
    return -1;
  }

  /**
   * Determins the current position within the file.  Returns -1 if
   * this is unknown.
   */
  gcc_pure
  virtual long tell() const {
    return -1;
  }
};

class TLineReader : public LineReader<TCHAR> {};

#ifdef _UNICODE
class NLineReader : public LineReader<char> {};
#else
class NLineReader : public TLineReader {};
#endif

#endif
