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

#ifndef XCSOAR_FIFO_BUFFER_HPP
#define XCSOAR_FIFO_BUFFER_HPP

#include <utility>
#include <algorithm>

/**
 * A first-in-first-out buffer: you can append data at the end, and
 * read data from the beginning.  This class automatically shifts the
 * buffer as needed.  It is not thread safe.
 */
template<class T>
class FifoBuffer {
public:
  typedef std::pair<T*, unsigned> Range;

protected:
  T *data;
  unsigned size, head, tail;

public:
  FifoBuffer(unsigned _size)
    :data(new T[_size]), size(_size), head(0), tail(0) {}

  ~FifoBuffer() {
    if (data != NULL)
      delete[] data;
  }

protected:
  void shift() {
    if (head == 0)
      return;

    std::copy(data + head, data + tail, data);

    tail -= head;
    head = 0;
  }

public:
  /**
   * Prepares writing.  Returns a buffer range which may be written.
   * When you are finished, call append().
   */
  Range write() {
    shift();
    return Range(data + tail, size - tail);
  }

  /**
   * Expands the tail of the buffer, after data has been written to
   * the buffer returned by write().
   */
  void append(unsigned n) {
    tail += n;
  }

  /**
   * Return a buffer range which may be read.  The buffer pointer is
   * writable, to allow modifications while parsing.
   */
  Range read() {
    return Range(data + head, tail - head);
  }

  /**
   * Marks a chunk as consumed.
   */
  void consume(unsigned n) {
    head += n;
  }
};

#endif
