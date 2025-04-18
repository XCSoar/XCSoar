// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <string_view>

/**
 * A helper class which can dissect a NMEA input line.
 */
class CSVLine {
protected:
  const char *data, *end;

public:
  explicit CSVLine(const char *line) noexcept;

  std::string_view Rest() const noexcept {
    return {data, std::size_t(end - data)};
  }

  bool IsEmpty() const noexcept {
    return data >= end;
  }

  /**
   * Read one column and return its contents as a std::string_view.
   */
  std::string_view ReadView() noexcept;

  void Skip() noexcept {
    ReadView();
  }

  /**
   * Skip a number of columns.
   */
  void Skip(unsigned n) noexcept {
    while (n-- > 0)
      Skip();
  }

  char ReadFirstChar() noexcept;

  /**
   * Read a column, expect it to be exactly one character.  Returns 0
   * on failure.
   */
  char ReadOneChar() noexcept;

  void Read(char *dest, size_t size) noexcept;
  bool ReadCompare(std::string_view value) noexcept;

  long Read(long default_value) noexcept;
  unsigned ReadHex(unsigned default_value) noexcept;

  int Read(int default_value) noexcept {
    return (int)Read((long)default_value);
  }

  int Read(bool default_value) noexcept {
    return Read((long)default_value) != 0;
  }

  double Read(double default_value) noexcept;
  bool ReadChecked(double &value_r) noexcept;

  bool ReadChecked(int &value_r) noexcept;
  bool ReadChecked(long &value_r) noexcept;
  bool ReadChecked(unsigned long &value_r) noexcept;
  bool ReadChecked(unsigned &value_r) noexcept;

  /**
   * Tries to read a hexadecimal number from the next field
   * @param value_r A reference to the variable that the
   * number should be written into
   * @return True if number was read successfully, False otherwise
   */
  bool ReadHexChecked(unsigned &value_r) noexcept;

  /**
   * Read a #double only if the unit string which follows matches.
   */
  bool ReadCheckedCompare(double &value_r, std::string_view string) noexcept;
};
