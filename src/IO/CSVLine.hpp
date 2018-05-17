/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_CSV_LINE_HPP
#define XCSOAR_CSV_LINE_HPP

#include "Util/Range.hpp"

#include <stddef.h>

/**
 * A helper class which can dissect a NMEA input line.
 */
class CSVLine {
protected:
  const char *data, *end;

public:
  CSVLine(const char *line);

  Range<const char *> Rest() const {
    return Range<const char *>(data, end);
  }

  bool IsEmpty() const {
    return data >= end;
  }

  /**
   * Skip the next column.
   *
   * @return the length of the column
   */
  size_t Skip();

  /**
   * Skip a number of columns.
   */
  void Skip(unsigned n) {
    while (n-- > 0)
      Skip();
  }

  char ReadFirstChar();

  /**
   * Read a column, expect it to be exactly one character.  Returns 0
   * on failure.
   */
  char ReadOneChar();

  void Read(char *dest, size_t size);
  bool ReadCompare(const char *value);

  long Read(long default_value);
  unsigned ReadHex(unsigned default_value);

  int Read(int default_value) {
    return (int)Read((long)default_value);
  }

  int Read(bool default_value) {
    return Read((long)default_value) != 0;
  }

  double Read(double default_value);
  bool ReadChecked(double &value_r);

  bool ReadChecked(int &value_r);
  bool ReadChecked(long &value_r);
  bool ReadChecked(unsigned long &value_r);
  bool ReadChecked(unsigned &value_r);

  /**
   * Tries to read a hexadecimal number from the next field
   * @param value_r A reference to the variable that the
   * number should be written into
   * @return True if number was read successfully, False otherwise
   */
  bool ReadHexChecked(unsigned &value_r);

  /**
   * Read a #double only if the unit string which follows matches.
   */
  bool ReadCheckedCompare(double &value_r, const char *string);
};

#endif
