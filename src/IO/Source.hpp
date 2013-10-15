/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_IO_SOURCE_HPP
#define XCSOAR_IO_SOURCE_HPP

#include "Util/WritableBuffer.hpp"
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
  typedef WritableBuffer<T> Range;

public:
  virtual ~Source() {}

  /**
   * Returns words from this source.  After you have used them, call
   * the method consume().  This method Invalidates any range returned
   * by a previous call.
   *
   * This object will always return as many words as possible; calling
   * read() subsequently (without consume()) will not get you more
   * data.  An empty range marks the end of the stream.
   */
  virtual Range Read() = 0;

  /**
   * Marks words as "consumed".  The buffer returned by read() is not
   * yet Invalidated, and may be used until the next read() call is
   * issued.
   */
  virtual void Consume(unsigned n) = 0;

  /**
   * Determins the size of the file.  Returns -1 if the size is
   * unknown.
   */
  gcc_pure
  virtual long GetSize() const {
    return -1;
  }

  /**
   * Determins the current position within the file.  Returns -1 if
   * this is unknown.
   */
  gcc_pure
  virtual long Tell() const {
    return -1;
  }
};

#endif
