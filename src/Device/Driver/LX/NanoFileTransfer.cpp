// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NanoFileTransfer.hpp"
#include "util/NumberParser.hxx"
#include "util/StringSplit.hxx"

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

std::optional<FileDataBlock>
ParseFileDataBlock(std::string_view line) noexcept
{
  /* "0,<block>,<crc>,<base64>"; the leading column is always 0 */
  const auto [status, rest1] = Split(line, ',');
  const auto [number, rest2] = Split(rest1, ',');
  const auto [crc, base64] = Split(rest2, ',');

  FileDataBlock block;
  if (status != "0" ||
      !ParseIntegerTo(number, block.number) ||
      !ParseIntegerTo(crc, block.crc) ||
      base64.empty())
    return std::nullopt;

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
