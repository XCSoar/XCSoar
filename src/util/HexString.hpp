/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#ifndef HEX_STRING_HPP
#define HEX_STRING_HPP

/** \file */

#include <array>
#include <string_view>
#include <stdexcept>

static unsigned char
ParseHexDigit(const unsigned char c) {
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  // Invalid character: parsing should fail
  throw std::invalid_argument("Invalid hex character encountered.");
}

/**
* Parses a string of hex data into a byte array.
*
* @param hex_str A string containing the data in hexadecimal form.
* @return An array filled with the specified data.
* @throws std::invalid_argument if parsing failed or the hex string has
*         the wrong size for the array.
*/
template<std::size_t len>
std::array<std::byte, len>
ParseHexString(const std::string_view hex_str)
{
  std::array<std::byte, len> raw_hash;

  if (hex_str.length() != len * 2)
    throw std::invalid_argument("Hex string has wrong length.");

  for (std::size_t i = 0; i < len; i++) {
    const unsigned char upper_nibble = ParseHexDigit(hex_str[2*i]);
    const unsigned char lower_nibble = ParseHexDigit(hex_str[2*i + 1]);

    raw_hash[i] = std::byte((upper_nibble << 4) | lower_nibble);
  }

  return raw_hash;
}
#endif
