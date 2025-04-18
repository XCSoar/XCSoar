// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CSVLine.hpp"

#include <algorithm>

#include <cassert>

#include <stdlib.h>
#include <string.h>

[[gnu::pure]]
static const char *
EndOfLine(const char *line) noexcept
{
  return line + strlen(line);
}

CSVLine::CSVLine(const char *line) noexcept
  :data(line), end(EndOfLine(line)) {}

std::string_view
CSVLine::ReadView() noexcept
{
  const char* _separator = strchr(data, ',');

  const char *s = data;
  std::size_t length;
  if (_separator != nullptr && _separator < end) {
    length = _separator - data;
    data = _separator + 1;
  } else {
    length = end - data;
    data = end;
  }

  return {s, length};
}

char
CSVLine::ReadFirstChar() noexcept
{
  const auto s = ReadView();
  return s.empty() ? '\0' : s.front();
}

char
CSVLine::ReadOneChar() noexcept
{
  const auto s = ReadView();
  return s.size() == 1 ? s.front() : '\0';
}

void
CSVLine::Read(char *dest, size_t size) noexcept
{
  auto src = ReadView();
  if (src.size() >= size)
    src = src.substr(0, size - 1);
  *std::copy(src.begin(), src.end(), dest) = '\0';
}

bool
CSVLine::ReadCompare(std::string_view value) noexcept
{
  return ReadView() == value;
}

long
CSVLine::Read(long default_value) noexcept
{
  ReadChecked(default_value);
  return default_value;
}

unsigned
CSVLine::ReadHex(unsigned default_value) noexcept
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
CSVLine::Read(double default_value) noexcept
{
  ReadChecked(default_value);
  return default_value;
}

bool
CSVLine::ReadChecked(double &value_r) noexcept
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
CSVLine::ReadChecked(int &value_r) noexcept
{
  long lvalue;
  if (!ReadChecked(lvalue))
    return false;

  value_r = lvalue;
  return true;
}

bool
CSVLine::ReadChecked(long &value_r) noexcept
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
CSVLine::ReadHexChecked(unsigned &value_r) noexcept
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
CSVLine::ReadChecked(unsigned long &value_r) noexcept
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
CSVLine::ReadChecked(unsigned &value_r) noexcept
{
  unsigned long lvalue;
  if (!ReadChecked(lvalue))
    return false;

  value_r = lvalue;
  return true;
}

bool
CSVLine::ReadCheckedCompare(double &value_r, std::string_view string) noexcept
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
