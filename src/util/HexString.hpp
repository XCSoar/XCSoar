// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <array>
#include <string>
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

/**
* Parses a hex string into binary data stored as a string
*
* @param hex_str A string in hexadecimal form.
* @return A string containing the decoded bytes (may include non-printable data).
* @throws std::invalid_argument if parsing failed or the hex string has
*         the wrong length.
*/
inline
std::string
ParseHexString(const std::string_view hex_str) {
  std::string result;

  if (hex_str.length() % 2 != 0)
    throw std::invalid_argument("Hex string has wrong length.");

  result.reserve(hex_str.length() / 2);

  for (std::size_t i = 0; i < hex_str.length(); i += 2) {
    const unsigned char upper = ParseHexDigit(hex_str[i]);
    const unsigned char lower = ParseHexDigit(hex_str[i + 1]);
    const unsigned char value = (upper << 4) | lower;
    result.push_back(static_cast<char>(value));
  }

  return result;
}
