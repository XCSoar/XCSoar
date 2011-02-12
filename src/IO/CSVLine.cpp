/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "NMEA/InputLine.hpp"

#include <assert.h>
#include <string.h>

static const char *
end_of_line(const char *line)
{
  return line + strlen(line);
}

CSVLine::CSVLine(const char *line):
  data(line), end(end_of_line(line)) {}

size_t
CSVLine::skip()
{
  const char* _seperator = strchr(data, ',');
  if (_seperator != NULL && _seperator < end) {
    size_t length = _seperator - data;
    data = _seperator + 1;
    return length;
  } else {
    size_t length = end - data;
    data = end;
    return length;
  }
}

char
CSVLine::read_first_char()
{
  char ch = *data;
  return skip() > 0 ? ch : '\0';
}

void
CSVLine::read(char *dest, size_t size)
{
  const char *src = data;
  size_t length = skip();
  if (length >= size)
    length = size - 1;
  strncpy(dest, src, length);
  dest[length] = '\0';
}

bool
CSVLine::read_compare(const char *value)
{
  size_t length = strlen(value);
  char buffer[length + 2];
  read(buffer, length + 2);
  return strcmp(buffer, value) == 0;
}

long
CSVLine::read(long default_value)
{
  char *endptr;
  long value = strtol(data, &endptr, 10);
  assert(endptr >= data && endptr <= end);
  if (endptr == data)
    /* nothing was parsed */
    value = default_value;

  if (endptr >= end) {
    data = end;
    return value;
  } else if (*endptr == ',') {
    data = endptr + 1;
    return value;
  } else {
    data = endptr;
    skip();
    return default_value;
  }
}

long
CSVLine::read_hex(long default_value)
{
  char *endptr;
  long value = strtol(data, &endptr, 16);
  assert(endptr >= data && endptr <= end);
  if (endptr == data)
    /* nothing was parsed */
    value = default_value;

  if (endptr >= end) {
    data = end;
    return value;
  } else if (*endptr == ',') {
    data = endptr + 1;
    return value;
  } else {
    data = endptr;
    skip();
    return default_value;
  }
}

double
CSVLine::read(double default_value)
{
  char *endptr;
  double value = strtod(data, &endptr);
  assert(endptr >= data && endptr <= end);
  if (endptr == data)
    /* nothing was parsed */
    value = default_value;

  if (endptr >= end) {
    data = end;
    return value;
  } else if (*endptr == ',') {
    data = endptr + 1;
    return value;
  } else {
    data = endptr;
    skip();
    return default_value;
  }
}

bool
CSVLine::read_checked(double &value_r)
{
  char *endptr;
  double value = strtod(data, &endptr);
  assert(endptr >= data && endptr <= end);

  bool success = endptr > data;
  if (endptr >= end) {
    data = end;
  } else if (*endptr == ',') {
    data = endptr + 1;
  } else {
    data = endptr;
    skip();
    return false;
  }

  if (success)
    value_r = value;
  return success;
}

#ifdef FIXED_MATH

fixed
CSVLine::read(fixed default_value)
{
  double value;
  return read_checked(value)
    ? fixed(value)
    : default_value;
}

bool
CSVLine::read_checked(fixed &value_r)
{
  double value;
  if (read_checked(value)) {
    value_r = fixed(value);
    return true;
  } else
    return false;
}

#endif /* FIXED_MATH */

bool
CSVLine::read_checked(int &value_r)
{
  char *endptr;
  int value = strtol(data, &endptr, 10);
  assert(endptr >= data && endptr <= end);

  bool success = endptr > data;
  if (endptr >= end) {
    data = end;
  } else if (*endptr == ',') {
    data = endptr + 1;
  } else {
    data = endptr;
    skip();
    return false;
  }

  if (success)
    value_r = value;
  return success;
}

bool
CSVLine::read_checked(long &value_r)
{
  char *endptr;
  long value = strtol(data, &endptr, 10);
  assert(endptr >= data && endptr <= end);

  bool success = endptr > data;
  if (endptr >= end) {
    data = end;
  } else if (*endptr == ',') {
    data = endptr + 1;
  } else {
    data = endptr;
    skip();
    return false;
  }

  if (success)
    value_r = value;
  return success;
}

bool
CSVLine::read_checked_compare(fixed &value_r, const char *string)
{
  fixed value;
  if (read_checked(value)) {
    if (read_compare(string)) {
      value_r = value;
      return true;
    } else
      return false;
  } else {
    skip();
    return false;
  }
}
