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

#include "CSVLine.hpp"
#include "Util/StringAPI.hxx"

#include <algorithm>

#include <assert.h>
#include <stdlib.h>

static const char *
EndOfLine(const char *line)
{
  return line + strlen(line);
}

CSVLine::CSVLine(const char *line):
  data(line), end(EndOfLine(line)) {}

size_t
CSVLine::Skip()
{
  const char* _seperator = strchr(data, ',');
  if (_seperator != nullptr && _seperator < end) {
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
CSVLine::ReadFirstChar()
{
  char ch = *data;
  return Skip() > 0 ? ch : '\0';
}

char
CSVLine::ReadOneChar()
{
  char ch = *data;
  return Skip() == 1 ? ch : '\0';
}

void
CSVLine::Read(char *dest, size_t size)
{
  const char *src = data;
  size_t length = Skip();
  if (length >= size)
    length = size - 1;
  *std::copy_n(src, length, dest) = '\0';
}

bool
CSVLine::ReadCompare(const char *value)
{
  size_t length = strlen(value);
  char buffer[length + 2];
  Read(buffer, length + 2);
  return StringIsEqual(buffer, value);
}

long
CSVLine::Read(long default_value)
{
  ReadChecked(default_value);
  return default_value;
}

unsigned
CSVLine::ReadHex(unsigned default_value)
{
  char *endptr;
  unsigned long value = strtoul(data, &endptr, 16);
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
    Skip();
    return default_value;
  }
}

double
CSVLine::Read(double default_value)
{
  ReadChecked(default_value);
  return default_value;
}

bool
CSVLine::ReadChecked(double &value_r)
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
    Skip();
    return false;
  }

  if (success)
    value_r = value;
  return success;
}

bool
CSVLine::ReadChecked(int &value_r)
{
  long lvalue;
  if (!ReadChecked(lvalue))
    return false;

  value_r = lvalue;
  return true;
}

bool
CSVLine::ReadChecked(long &value_r)
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
    Skip();
    return false;
  }

  if (success)
    value_r = value;
  return success;
}

bool
CSVLine::ReadHexChecked(unsigned &value_r)
{
  char *endptr;
  unsigned long value = strtoul(data, &endptr, 16);
  assert(endptr >= data && endptr <= end);

  bool success = endptr > data;
  if (endptr >= end) {
    data = end;
  } else if (*endptr == ',') {
    data = endptr + 1;
  } else {
    data = endptr;
    Skip();
    return false;
  }

  if (success)
    value_r = value;
  return success;
}

bool
CSVLine::ReadChecked(unsigned long &value_r)
{
  char *endptr;
  unsigned long value = strtoul(data, &endptr, 10);
  assert(endptr >= data && endptr <= end);

  bool success = endptr > data;
  if (endptr >= end) {
    data = end;
  } else if (*endptr == ',') {
    data = endptr + 1;
  } else {
    data = endptr;
    Skip();
    return false;
  }

  if (success)
    value_r = value;
  return success;
}

bool
CSVLine::ReadChecked(unsigned &value_r)
{
  unsigned long lvalue;
  if (!ReadChecked(lvalue))
    return false;

  value_r = lvalue;
  return true;
}

bool
CSVLine::ReadCheckedCompare(double &value_r, const char *string)
{
  double value;
  if (ReadChecked(value)) {
    if (ReadCompare(string)) {
      value_r = value;
      return true;
    } else
      return false;
  } else {
    Skip();
    return false;
  }
}
