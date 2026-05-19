// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TarArchive.hpp"

#include "io/OutputStream.hxx"
#include "io/Reader.hxx"
#include "system/Path.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <span>
#include <stdexcept>

namespace {

constexpr size_t TAR_BLOCK_SIZE = 512;
constexpr size_t COPY_BUFFER_SIZE = 262144;

using TarHeader = std::array<char, TAR_BLOCK_SIZE>;
using TarBlock = std::array<std::byte, TAR_BLOCK_SIZE>;

[[nodiscard]]
bool
IsZeroHeader(const TarHeader &header) noexcept
{
  for (char c : header)
    if (c != '\0')
      return false;

  return true;
}

[[nodiscard]]
uint64_t
ReadSizeFromHeader(const TarHeader &header) noexcept
{
  char buf[13] = {};
  std::memcpy(buf, header.data() + 124, 12);
  return buf[0] != '\0' ? std::strtoull(buf, nullptr, 8) : 0;
}

[[nodiscard]]
size_t
PadToBlockSize(uint64_t size) noexcept
{
  return (TAR_BLOCK_SIZE - (size % TAR_BLOCK_SIZE)) % TAR_BLOCK_SIZE;
}

bool
ReadHeaderBlock(Reader &in, TarHeader &header)
{
  const size_t n = in.Read(std::span<std::byte>(
    reinterpret_cast<std::byte *>(header.data()), header.size()));
  return n == header.size();
}

void
SkipBytes(Reader &in, uint64_t size)
{
  std::array<std::byte, COPY_BUFFER_SIZE> buffer;

  while (size > 0) {
    const size_t to_read =
      std::min<size_t>(buffer.size(), static_cast<size_t>(size));
    const size_t n = in.Read(std::span<std::byte>(buffer.data(), to_read));
    if (n == 0)
      break;

    size -= n;
  }
}

void
CopyBytes(Reader &in, OutputStream &out, uint64_t size)
{
  std::array<std::byte, COPY_BUFFER_SIZE> buffer;
  uint64_t remaining = size;

  while (remaining > 0) {
    const size_t to_read = std::min<size_t>(buffer.size(),
                                            static_cast<size_t>(remaining));
    const size_t n = in.Read(std::span<std::byte>(buffer.data(), to_read));
    if (n == 0)
      throw std::runtime_error("short read in tar data");

    out.Write(std::span<const std::byte>(buffer.data(), n));
    remaining -= n;
  }
}

void
WriteString(char *dest, std::string_view src, size_t max) noexcept
{
  if (max == 0)
    return;

  std::memset(dest, 0, max);
  std::memcpy(dest, src.data(), std::min(max - 1, src.size()));
}

void
WriteOctal(char *dest, size_t size, uint64_t value) noexcept
{
  if (size == 0)
    return;

  std::snprintf(dest, size, "%0*llo ", int(size - 1),
                (unsigned long long)value);
}

void
WriteChecksumField(TarHeader &header) noexcept
{
  std::memset(header.data() + 148, ' ', 8);

  unsigned sum = 0;
  for (char c : header)
    sum += static_cast<unsigned>(static_cast<unsigned char>(c));

  char checksum[16] = {};
  std::snprintf(checksum, sizeof(checksum), "%06o ", sum);
  std::memcpy(header.data() + 148, checksum, 8);
}

void
WriteHeaderBlock(OutputStream &out, std::string_view name, uint64_t size)
{
  TarHeader header{};
  WriteString(header.data() + 0, name, 100);
  WriteOctal(header.data() + 100, 8, 0644);
  WriteOctal(header.data() + 108, 8, 0);
  WriteOctal(header.data() + 116, 8, 0);
  WriteOctal(header.data() + 124, 12, size);
  WriteOctal(header.data() + 136, 12,
             static_cast<uint64_t>(std::time(nullptr)));
  WriteString(header.data() + 257, "ustar", 6);
  header[263] = '0';
  header[264] = '0';
  WriteChecksumField(header);

  out.Write(std::span<const std::byte>(
    reinterpret_cast<const std::byte *>(header.data()), header.size()));
}

void
WritePaddingBytes(OutputStream &out, uint64_t size)
{
  const size_t pad = PadToBlockSize(size);
  if (pad == 0)
    return;

  TarBlock zero{};
  out.Write(std::span<const std::byte>(zero.data(), pad));
}

void
WriteEndMarker(OutputStream &out)
{
  TarBlock zero{};
  out.Write(std::span<const std::byte>(zero.data(), zero.size()));
  out.Write(std::span<const std::byte>(zero.data(), zero.size()));
}

} // namespace

bool
TarReader::Next(std::string &name, uint64_t &size)
{
  for (;;) {
    TarHeader header{};
    if (!ReadHeaderBlock(input, header))
      return false;

    if (IsZeroHeader(header))
      return false;

    name.assign(header.data(), strnlen(header.data(), 100));
    std::replace(name.begin(), name.end(), '\\', '/');

    /* strip leading slashes to prevent absolute-path extraction */
    auto first = name.find_first_not_of('/');
    if (first == std::string::npos)
      first = name.size();
    if (first > 0)
      name.erase(0, first);

    size = ReadSizeFromHeader(header);
    remaining = size;

    /* skip entries with path-traversal sequences or empty names */
    if (name.empty() || Path(name.c_str()).HasPathTraversal()) {
      Skip();
      continue;
    }

    return true;
  }
}

void
TarReader::ReadData(OutputStream &out)
{
  CopyBytes(input, out, remaining);
  SkipBytes(input, PadToBlockSize(remaining));
  remaining = 0;
}

void
TarReader::Skip()
{
  SkipBytes(input, remaining + PadToBlockSize(remaining));
  remaining = 0;
}

void
TarWriter::Add(std::string_view name, Reader &in, uint64_t size)
{
  WriteHeaderBlock(output, name, size);
  CopyBytes(in, output, size);
  WritePaddingBytes(output, size);
}

void
TarWriter::Finish()
{
  WriteEndMarker(output);
}
