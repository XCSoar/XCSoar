// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NanoFileTransfer.hpp"
#include "util/NumberParser.hxx"
#include "util/StringSplit.hxx"

#include <limits>

namespace Nano {

/**
 * Shift right by @p n bits, copying the sign bit in.
 */
static constexpr uint32_t
ShiftRightSigned(uint32_t x, unsigned n) noexcept
{
  const uint32_t sign = (x & 0x80000000u) ? ~(~uint32_t{0} >> n) : 0;
  return (x >> n) | sign;
}

void
FileTransferCrc::Update(std::span<const std::byte> data) noexcept
{
  for (const std::byte b : data) {
    uint32_t v = (value ^ static_cast<uint32_t>(b)) & 0xff;
    for (unsigned bit = 0; bit < 8; ++bit)
      v = (v & 1)
        ? ShiftRightSigned(v, 1) ^ 0xedb88320u
        : ShiftRightSigned(v, 1);

    value = v ^ ShiftRightSigned(value, 8);
  }
}

std::optional<int32_t>
ParseFileTransferCrc(std::string_view s) noexcept
{
  int64_t value;
  if (!ParseIntegerTo(s, value) ||
      value < std::numeric_limits<int32_t>::min() ||
      value > std::numeric_limits<uint32_t>::max())
    return std::nullopt;

  return static_cast<int32_t>(static_cast<uint32_t>(value));
}

std::optional<FileDataBlock>
ParseFileDataBlock(std::string_view line) noexcept
{
  /* "<status>,<block>,<crc>,<base64>"; the status is 0, or 1 for a
     block a Nano 3 (firmware 3.02) sends again after FILE_DATA_LOST */
  const auto [status, rest1] = Split(line, ',');
  const auto [number, rest2] = Split(rest1, ',');
  const auto [crc, base64] = Split(rest2, ',');

  FileDataBlock block;
  const auto crc_value = ParseFileTransferCrc(crc);
  if ((status != "0" && status != "1") ||
      !ParseIntegerTo(number, block.number) ||
      !crc_value ||
      base64.empty())
    return std::nullopt;

  block.crc = *crc_value;
  block.base64 = base64;
  return block;
}

static constexpr int
DecodeBase64Char(char ch) noexcept
{
  if (ch >= 'A' && ch <= 'Z')
    return ch - 'A';
  if (ch >= 'a' && ch <= 'z')
    return ch - 'a' + 26;
  if (ch >= '0' && ch <= '9')
    return ch - '0' + 52;
  if (ch == '+')
    return 62;
  if (ch == '/')
    return 63;
  return -1;
}

std::optional<std::size_t>
DecodeBase64(std::string_view src, std::span<std::byte> dest) noexcept
{
  if (src.size() % 4 != 0)
    return std::nullopt;

  std::size_t n = 0;
  while (!src.empty()) {
    const auto quad = src.substr(0, 4);
    src.remove_prefix(4);

    /* "=" may only pad the last group */
    const unsigned padding = quad[3] != '=' ? 0 : quad[2] != '=' ? 1 : 2;
    if (padding > 0 && !src.empty())
      return std::nullopt;

    uint32_t bits = 0;
    for (unsigned i = 0; i < 4 - padding; ++i) {
      const int v = DecodeBase64Char(quad[i]);
      if (v < 0)
        return std::nullopt;
      bits |= uint32_t(v) << (18 - 6 * i);
    }

    const unsigned count = 3 - padding;
    if (n + count > dest.size())
      return std::nullopt;

    for (unsigned i = 0; i < count; ++i)
      dest[n++] = static_cast<std::byte>(bits >> (16 - 8 * i));
  }

  return n;
}

} // namespace Nano
