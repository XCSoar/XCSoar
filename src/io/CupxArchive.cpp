// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CupxArchive.hpp"
#include "Open.hxx"
#include "UniqueFileDescriptor.hxx"
#include "system/Path.hpp"
#include "util/PackedLittleEndian.hxx"
#include "util/StringCompare.hxx"

#include <zlib.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>

static constexpr uint32_t CUPX_MAGIC = 0x58505543;
static constexpr std::size_t CUPX_HEADER_SIZE = 256;

static constexpr uint32_t ZIP_LOCAL_FILE_SIG = 0x04034b50;
static constexpr std::size_t ZIP_LOCAL_FILE_SIZE = 30;

static constexpr uint32_t ZIP_CENTRAL_DIR_SIG = 0x02014b50;
static constexpr std::size_t ZIP_CENTRAL_DIR_SIZE = 46;

static constexpr uint32_t ZIP_EOCD_SIG = 0x06054b50;
static constexpr std::size_t ZIP_EOCD_SIZE = 22;

/** Max distance from EOF to search for an EOCD (ZIP comment limit). */
static constexpr off_t ZIP_EOCD_SEARCH = 65557;

static constexpr uint32_t MAX_ENTRY_SIZE = 120u * 1024 * 1024;

static constexpr uint16_t ZIP_FLAG_DATA_DESCRIPTOR = 0x0008;

static const PackedLE16 &
LE16(const std::byte *p) noexcept
{
  return *reinterpret_cast<const PackedLE16 *>(p);
}

static const PackedLE32 &
LE32(const std::byte *p) noexcept
{
  return *reinterpret_cast<const PackedLE32 *>(p);
}

static bool
ReadFull(FileDescriptor fd, void *buf, std::size_t len) noexcept
{
  auto *p = static_cast<uint8_t *>(buf);
  while (len > 0) {
    const auto n = fd.Read(p, len);
    if (n <= 0)
      return false;
    p += n;
    len -= static_cast<std::size_t>(n);
  }
  return true;
}

static UniqueFileDescriptor
OpenCupx(Path path) noexcept
try {
  return OpenReadOnly(path.c_str());
} catch (...) {
  return {};
}

/**
 * Skip past the optional 256-byte CUPX header.  On return the file
 * position is at the start of pics.zip (or at offset 0 if no header).
 */
static bool
SkipCupxHeader(FileDescriptor fd) noexcept
{
  std::byte magic[4];
  if (!ReadFull(fd, magic, 4))
    return false;

  if (LE32(magic) == CUPX_MAGIC)
    return fd.Seek(CUPX_HEADER_SIZE) >= 0;

  return fd.Rewind();
}

/**
 * Decompress a stored or deflated ZIP entry from the current file
 * position, given known sizes.
 */
static std::vector<std::byte>
DecompressEntry(FileDescriptor fd, uint16_t compression,
                uint32_t compressed_size,
                uint32_t uncompressed_size)
{
  if (compressed_size > MAX_ENTRY_SIZE ||
      uncompressed_size > MAX_ENTRY_SIZE)
    return {};

  if (compression == 0) {
    std::vector<std::byte> result(compressed_size);
    if (!ReadFull(fd, result.data(), compressed_size))
      return {};
    return result;
  }

  if (compression != 8)
    return {};

  std::vector<std::byte> compressed(compressed_size);
  if (!ReadFull(fd, compressed.data(), compressed_size))
    return {};

  std::vector<std::byte> result(uncompressed_size);

  z_stream strm{};
  strm.next_in = reinterpret_cast<Bytef *>(compressed.data());
  strm.avail_in = compressed_size;
  strm.next_out = reinterpret_cast<Bytef *>(result.data());
  strm.avail_out = uncompressed_size;

  if (inflateInit2(&strm, -MAX_WBITS) != Z_OK)
    return {};

  const int ret = inflate(&strm, Z_FINISH);
  inflateEnd(&strm);

  if (ret != Z_STREAM_END)
    return {};

  result.resize(uncompressed_size - strm.avail_out);
  return result;
}

struct ZipEocd {
  /** Absolute start of this ZIP inside the .cupx file (SFX delta). */
  off_t zip_start;
  uint32_t cd_size;
  uint32_t cd_offset;
};

/**
 * Locate an End-of-Central-Directory record near EOF.
 *
 * @param zip_start  if set, require SFX concat == this (pics.zip);
 *                   if nullopt, take the last valid EOCD (points.zip).
 */
static std::optional<ZipEocd>
FindEocd(FileDescriptor fd, off_t filesize,
         std::optional<off_t> zip_start) noexcept
{
  const off_t search_start = std::max(off_t{0}, filesize - ZIP_EOCD_SEARCH);
  const auto search_len = static_cast<std::size_t>(filesize - search_start);

  std::vector<std::byte> tail(search_len);
  if (fd.Seek(search_start) < 0)
    return std::nullopt;
  if (!ReadFull(fd, tail.data(), search_len))
    return std::nullopt;

  for (ssize_t i = static_cast<ssize_t>(search_len) - ZIP_EOCD_SIZE;
       i >= 0; --i) {
    if (LE32(tail.data() + i) != ZIP_EOCD_SIG)
      continue;

    const std::byte *eocd = tail.data() + i;
    const uint32_t cd_size = LE32(eocd + 12);
    const uint32_t cd_offset = LE32(eocd + 16);
    if (cd_size > MAX_ENTRY_SIZE)
      continue;

    const off_t eocd_offset = search_start + i;
    const off_t concat = eocd_offset
      - static_cast<off_t>(cd_size)
      - static_cast<off_t>(cd_offset);
    if (concat < 0)
      continue;

    const off_t real_cd = static_cast<off_t>(cd_offset) + concat;
    if (real_cd < 0 ||
        real_cd + static_cast<off_t>(cd_size) > filesize)
      continue;

    /* Confirm a central-directory signature at the computed offset. */
    std::byte sig[4];
    if (fd.Seek(real_cd) < 0 || !ReadFull(fd, sig, 4))
      continue;
    if (LE32(sig) != ZIP_CENTRAL_DIR_SIG)
      continue;

    ZipEocd found{concat, cd_size, cd_offset};

    if (!zip_start.has_value())
      /* First hit scanning backward = last EOCD in the file
         (points.zip). */
      return found;

    if (concat == *zip_start)
      return found;
  }

  return std::nullopt;
}

/**
 * Extract one named entry from a ZIP located via @p eocd.
 * Uses central-directory sizes when the local header has bit 3 set
 * (or zero sizes), which newer SeeYou .cupx files rely on.
 */
static std::vector<std::byte>
ExtractZipEntry(FileDescriptor fd, off_t filesize,
                const ZipEocd &eocd, std::string_view entry_name)
{
  std::vector<std::byte> cd(eocd.cd_size);
  const off_t real_cd = static_cast<off_t>(eocd.cd_offset) + eocd.zip_start;
  if (fd.Seek(real_cd) < 0 || !ReadFull(fd, cd.data(), eocd.cd_size))
    return {};

  std::size_t pos = 0;
  while (pos + ZIP_CENTRAL_DIR_SIZE <= eocd.cd_size) {
    if (LE32(cd.data() + pos) != ZIP_CENTRAL_DIR_SIG)
      break;

    const uint16_t name_len = LE16(cd.data() + pos + 28);
    const uint16_t extra_len = LE16(cd.data() + pos + 30);
    const uint16_t comment_len = LE16(cd.data() + pos + 32);

    if (pos + ZIP_CENTRAL_DIR_SIZE + name_len > eocd.cd_size)
      break;

    const std::string_view name{
      reinterpret_cast<const char *>(cd.data() + pos + ZIP_CENTRAL_DIR_SIZE),
      name_len};

    if (!StringIsEqualIgnoreCase(name, entry_name)) {
      pos += ZIP_CENTRAL_DIR_SIZE + name_len + extra_len + comment_len;
      continue;
    }

    const uint32_t cd_compressed_size = LE32(cd.data() + pos + 20);
    const uint32_t cd_uncompressed_size = LE32(cd.data() + pos + 24);
    const uint32_t local_offset = LE32(cd.data() + pos + 42);
    const off_t real_local =
      static_cast<off_t>(local_offset) + eocd.zip_start;
    if (real_local < 0 ||
        real_local + static_cast<off_t>(ZIP_LOCAL_FILE_SIZE) > filesize)
      return {};

    if (fd.Seek(real_local) < 0)
      return {};

    std::byte lh[ZIP_LOCAL_FILE_SIZE];
    if (!ReadFull(fd, lh, ZIP_LOCAL_FILE_SIZE))
      return {};
    if (LE32(lh) != ZIP_LOCAL_FILE_SIG)
      return {};

    const uint16_t compression = LE16(lh + 8);
    uint32_t compressed_size = LE32(lh + 18);
    uint32_t uncompressed_size = LE32(lh + 22);
    const uint16_t lh_name_len = LE16(lh + 26);
    const uint16_t lh_extra_len = LE16(lh + 28);

    if ((LE16(lh + 6) & ZIP_FLAG_DATA_DESCRIPTOR) != 0 ||
        (compressed_size == 0 && uncompressed_size == 0 &&
         cd_compressed_size != 0)) {
      compressed_size = cd_compressed_size;
      uncompressed_size = cd_uncompressed_size;
    }

    if (fd.Skip(lh_name_len + lh_extra_len) < 0)
      return {};

    return DecompressEntry(fd, compression, compressed_size,
                           uncompressed_size);
  }

  return {};
}

static std::vector<std::byte>
ExtractNamedEntry(FileDescriptor fd, off_t filesize,
                  std::string_view entry_name,
                  std::optional<off_t> zip_start)
{
  const auto eocd = FindEocd(fd, filesize, zip_start);
  if (!eocd)
    return {};
  return ExtractZipEntry(fd, filesize, *eocd, entry_name);
}

std::vector<std::byte>
CupxArchive::ExtractImage(Path cupx_path, std::string_view image_name)
{
  auto fd = OpenCupx(cupx_path);
  if (!fd.IsDefined())
    return {};

  if (!SkipCupxHeader(fd))
    return {};

  const off_t zip_start = fd.Tell();
  if (zip_start < 0)
    return {};

  const off_t filesize = fd.GetSize();
  if (filesize < 0)
    return {};

  char entry[520];
  if (image_name.size() + 5 >= sizeof(entry))
    return {};

  std::memcpy(entry, "Pics/", 5);
  std::memcpy(entry + 5, image_name.data(), image_name.size());

  return ExtractNamedEntry(fd, filesize,
                           {entry, 5 + image_name.size()},
                           zip_start);
}

std::vector<std::byte>
CupxArchive::ExtractPointsCup(Path cupx_path)
{
  auto fd = OpenCupx(cupx_path);
  if (!fd.IsDefined())
    return {};

  const off_t filesize = fd.GetSize();
  if (filesize < 0)
    return {};

  return ExtractNamedEntry(fd, filesize, "POINTS.CUP", std::nullopt);
}
