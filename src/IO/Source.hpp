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

#ifndef XCSOAR_IO_SOURCE_HPP
#define XCSOAR_IO_SOURCE_HPP

#include "Compiler.h"

#include <utility>

/**
 * A data source.  This object provides a buffer, which may be
 * modified in-place by the caller, to do zero-copy parsing.
 */
template<class T>
class Source {
public:
  /**
   * A portion of this object's buffer.  The first item is the start
   * of the buffer, and the second item is the number of available
   * words.
   */
  typedef std::pair<T*, unsigned> Range;

public:
  virtual ~Source() {}

  /**
   * Returns words from this source.  After you have used them, call
   * the method consume().  This method invalidates any range returned
   * by a previous call.
   *
   * This object will always return as many words as possible; calling
   * read() subsequently (without consume()) will not get you more
   * data.  An empty range marks the end of the stream.
   */
  virtual Range read() = 0;

  /**
   * Marks words as "consumed".  The buffer returned by read() is not
   * yet invalidated, and may be used until the next read() call is
   * issued.
   */
  virtual void consume(unsigned n) = 0;

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

#endif
