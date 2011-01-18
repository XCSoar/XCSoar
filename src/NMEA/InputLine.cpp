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
  const char *asterisk = strchr(line, '*');
  return asterisk != NULL
    ? asterisk
    : line + strlen(line);
}

NMEAInputLine::NMEAInputLine(const char *line)
  :data(line), end(end_of_line(line)) {
}

size_t
NMEAInputLine::skip()
{
  const char *comma = strchr(data, ',');
  if (comma != NULL && comma < end) {
    size_t length = comma - data;
    data = comma + 1;
    return length;
  } else {
    size_t length = end - data;
    data = end;
    return length;
  }
}

char
NMEAInputLine::read_first_char()
{
  char ch = *data;
  return skip() > 0 ? ch : '\0';
}

void
NMEAInputLine::read(char *dest, size_t size)
{
  const char *src = data;
  size_t length = skip();
  if (length >= size)
    length = size - 1;
  strncpy(dest, src, length);
  dest[length] = '\0';
}

bool
NMEAInputLine::read_compare(const char *value)
{
  size_t length = strlen(value);
  char buffer[length + 2];
  read(buffer, length + 2);
  return strcmp(buffer, value) == 0;
}

static bool
is_end_of_line(char ch)
{
  return ch == '\0' || ch == '*';
}

long
NMEAInputLine::read(long default_value)
{
  char *endptr;
  long value = strtol(data, &endptr, 10);
  assert(endptr >= data && endptr <= end);
  if (is_end_of_line(*endptr)) {
    data = "";
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
NMEAInputLine::read_hex(long default_value)
{
  char *endptr;
  long value = strtol(data, &endptr, 16);
  assert(endptr >= data && endptr <= end);
  if (is_end_of_line(*endptr)) {
    data = "";
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
NMEAInputLine::read(double default_value)
{
  char *endptr;
  double value = strtod(data, &endptr);
  assert(endptr >= data && endptr <= end);
  if (is_end_of_line(*endptr)) {
    data = "";
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
NMEAInputLine::read_checked(double &value_r)
{
  char *endptr;
  double value = strtod(data, &endptr);
  assert(endptr >= data && endptr <= end);

  bool success = endptr > data;
  if (is_end_of_line(*endptr)) {
    data = "";
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
NMEAInputLine::read(fixed default_value)
{
  double value;
  return read_checked(value)
    ? fixed(value)
    : default_value;
}

bool
NMEAInputLine::read_checked(fixed &value_r)
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
NMEAInputLine::read_checked(int &value_r)
{
  char *endptr;
  long value = strtol(data, &endptr, 10);
  assert(endptr >= data && endptr <= end);

  bool success = endptr > data;
  if (is_end_of_line(*endptr)) {
    data = "";
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
NMEAInputLine::read_checked_compare(fixed &value_r, const char *string)
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
