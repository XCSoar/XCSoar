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

#ifndef XCSOAR_REUSABLE_ARRAY_HPP
#define XCSOAR_REUSABLE_ARRAY_HPP

#include "Util/NonCopyable.hpp"

#include <algorithm>
#include <stddef.h>

template<class T>
class ReusableArray : private NonCopyable {
protected:
  T *data;
  unsigned length;

public:
  ReusableArray():data(NULL), length(0) {}
  ReusableArray(unsigned _length):data(new T[_length]), length(_length) {}

  ~ReusableArray() {
    if (data != NULL)
      delete[] data;
  }

  /**
   * Obtains an array.  Its values are undefined.
   */
  T *get(unsigned _length) {
    if (_length > length) {
      delete[] data;
      length = _length;
      data = new T[length];
    }

    return data;
  }

  /**
   * Grows an existing array, preserving data.
   */
  T *grow(unsigned old_length, unsigned new_length) {
    if (new_length >= length) {
      T *new_data = new T[new_length];
      if (new_data == NULL)
        return NULL;

      std::copy(data, data + old_length, new_data);
      delete[] data;
      data = new_data;
      length = new_length;
    }

    return data;
  }
};

#endif
