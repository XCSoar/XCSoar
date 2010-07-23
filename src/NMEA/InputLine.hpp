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

#ifndef XCSOAR_NMEA_INPUT_LINE_HPP
#define XCSOAR_NMEA_INPUT_LINE_HPP

#include "Math/fixed.hpp"

/**
 * A helper class which can dissect a NMEA input line.
 */
class NMEAInputLine {
  const char *data, *end;

public:
  NMEAInputLine(const char *line);

  const char *rest() const {
    return data;
  }

  /**
   * Skip the next column.
   *
   * @return the length of the column
   */
  size_t skip();

  /**
   * Skip a number of columns.
   */
  void skip(unsigned n) {
    while (n-- > 0)
      skip();
  }

  char read_first_char();

  void read(char *dest, size_t size);
  bool read_compare(const char *value);

  long read(long default_value);
  long read_hex(long default_value);

  int read(int default_value) {
    return (int)read((long)default_value);
  }

  int read(bool default_value) {
    return read((long)default_value) != 0;
  }

  double read(double default_value);
  bool read_checked(double &value_r);

#ifdef FIXED_MATH
  fixed read(fixed default_value);
  bool read_checked(fixed &value_r);
#endif

  /**
   * Read a #fixed only if the unit string which follows matches.
   */
  bool read_checked_compare(fixed &value_r, const char *string);
};

#endif
