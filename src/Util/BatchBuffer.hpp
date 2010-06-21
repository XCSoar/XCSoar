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

#ifndef XCSOAR_BATCH_BUFFER_HPP
#define XCSOAR_BATCH_BUFFER_HPP

#include <algorithm>

/**
 * A fixed-size buffer which gets flushed by the caller after it
 * becomes full.
 *
 * Not thread safe.
 */
template<class T, unsigned size>
class BatchBuffer {
protected:
  T data[size];
  unsigned tail;

public:
  BatchBuffer()
    :tail(0) {}

  BatchBuffer(const BatchBuffer<T,size> &other)
    :tail(other.tail) {
    std::copy(other.data, other.data + tail, data);
  }

protected:
  static unsigned next(unsigned i) {
    return (i + 1) % size;
  }

public:
  bool empty() const {
    return tail == 0;
  }

  bool full() const {
    return tail == size;
  }

  unsigned length() const {
    return tail;
  }

  const T &operator[](unsigned i) const {
    return data[i];
  }

  T &append() {
    return data[tail++];
  }

  void clear() {
    tail = 0;
  }
};

#endif
