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

static constexpr uint32_t CUPX_MAGIC = 0x58505543;
static constexpr std::size_t CUPX_HEADER_SIZE = 256;

static constexpr uint32_t ZIP_LOCAL_FILE_SIG = 0x04034b50;
static constexpr std::size_t ZIP_LOCAL_FILE_SIZE = 30;

static constexpr uint32_t ZIP_CENTRAL_DIR_SIG = 0x02014b50;
static constexpr std::size_t ZIP_CENTRAL_DIR_SIZE = 46;

static constexpr uint32_t ZIP_EOCD_SIG = 0x06054b50;
static constexpr std::size_t ZIP_EOCD_SIZE = 22;

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

static constexpr uint32_t MAX_ENTRY_SIZE = 120u * 1024 * 1024;

/**
 * Read exactly @p len bytes, looping for short reads (which POSIX
 * allows even on regular files, e.g. after a signal).
 */
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

/**
 * Decompress a stored or deflated ZIP entry from the current file
 * position.
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

  if (compression == 8) {
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

    int ret = inflate(&strm, Z_FINISH);
    inflateEnd(&strm);

    if (ret != Z_STREAM_END)
      return {};

    result.resize(uncompressed_size - strm.avail_out);
    return result;
  }

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
 * Walk local file headers forward from the current position and
 * extract the first entry whose name matches (case-insensitive).
 */
static std::vector<std::byte>
ExtractByForwardScan(FileDescriptor fd,
                     std::string_view entry_name)
{
  std::byte header[ZIP_LOCAL_FILE_SIZE];

  while (true) {
    if (!ReadFull(fd, header, ZIP_LOCAL_FILE_SIZE))
      return {};

    if (LE32(header) != ZIP_LOCAL_FILE_SIG)
      return {};

    const uint16_t compression = LE16(header + 8);
    const uint32_t compressed_size = LE32(header + 18);
    const uint32_t uncompressed_size = LE32(header + 22);
    const uint16_t name_len = LE16(header + 26);
    const uint16_t extra_len = LE16(header + 28);

    /* data-descriptor entries (bit 3) not supported */
    if (LE16(header + 6) & 0x08)
      return {};

    char name_buf[512];
    if (name_len == 0 || name_len >= sizeof(name_buf)) {
      if (fd.Skip(static_cast<off_t>(name_len) + extra_len + compressed_size) < 0)
        return {};
      continue;
    }

    if (!ReadFull(fd, name_buf, name_len))
      return {};

    if (extra_len > 0 && fd.Skip(extra_len) < 0)
      return {};

    if (StringIsEqualIgnoreCase({name_buf, name_len}, entry_name))
      return DecompressEntry(fd, compression, compressed_size,
                             uncompressed_size);

    if (compressed_size > 0 && fd.Skip(compressed_size) < 0)
      return {};
  }
}

/**
 * Find a named entry via the central directory of the last ZIP in
 * the file (using the Python-style SFX offset adjustment).
 */
static std::vector<std::byte>
ExtractByEocdScan(FileDescriptor fd, off_t filesize,
                  std::string_view entry_name)
{
  const off_t search_start = std::max(off_t{0}, filesize - 65557);
  const auto search_len = static_cast<std::size_t>(filesize - search_start);

  std::vector<std::byte> tail(search_len);
  if (fd.Seek(search_start) < 0)
    return {};
  if (!ReadFull(fd, tail.data(), search_len))
    return {};

  off_t eocd_offset = -1;
  for (ssize_t i = static_cast<ssize_t>(search_len) - ZIP_EOCD_SIZE;
       i >= 0; --i) {
    if (LE32(tail.data() + i) == ZIP_EOCD_SIG) {
      eocd_offset = search_start + i;
      break;
    }
  }

  if (eocd_offset < 0)
    return {};

  const std::byte *eocd = tail.data() + (eocd_offset - search_start);
  const uint32_t cd_size = LE32(eocd + 12);
  const uint32_t cd_offset = LE32(eocd + 16);

  if (cd_size > MAX_ENTRY_SIZE)
    return {};

  /*
   * SFX adjustment: the CD offset in the EOCD is relative to the
   * embedded ZIP start, not the overall file.
   */
  const off_t concat = eocd_offset
    - static_cast<off_t>(cd_size)
    - static_cast<off_t>(cd_offset);
  if (concat < 0)
    return {};

  const off_t real_cd_pos = static_cast<off_t>(cd_offset) + concat;
  if (real_cd_pos < 0 || real_cd_pos + static_cast<off_t>(cd_size) > filesize)
    return {};

  std::vector<std::byte> cd(cd_size);
  if (fd.Seek(real_cd_pos) < 0)
    return {};
  if (!ReadFull(fd, cd.data(), cd_size))
    return {};

  std::size_t pos = 0;
  while (pos + ZIP_CENTRAL_DIR_SIZE <= cd_size) {
    if (LE32(cd.data() + pos) != ZIP_CENTRAL_DIR_SIG)
      break;

    const uint16_t name_len = LE16(cd.data() + pos + 28);
    const uint16_t extra_len = LE16(cd.data() + pos + 30);
    const uint16_t comment_len = LE16(cd.data() + pos + 32);

    if (pos + ZIP_CENTRAL_DIR_SIZE + name_len > cd_size)
      break;

    const std::string_view name{
      reinterpret_cast<const char *>(cd.data() + pos + ZIP_CENTRAL_DIR_SIZE),
      name_len};

    if (StringIsEqualIgnoreCase(name, entry_name)) {
      const uint32_t local_offset = LE32(cd.data() + pos + 42);
      const off_t real_local = static_cast<off_t>(local_offset) + concat;
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
      const uint32_t compressed_size = LE32(lh + 18);
      const uint32_t uncompressed_size = LE32(lh + 22);
      const uint16_t lh_name_len = LE16(lh + 26);
      const uint16_t lh_extra_len = LE16(lh + 28);

      if (fd.Skip(lh_name_len + lh_extra_len) < 0)
        return {};

      return DecompressEntry(fd, compression, compressed_size,
                             uncompressed_size);
    }

    pos += ZIP_CENTRAL_DIR_SIZE + name_len + extra_len + comment_len;
  }

  return {};
}

std::vector<std::byte>
CupxArchive::ExtractImage(Path cupx_path, std::string_view image_name)
{
  UniqueFileDescriptor fd;
  try {
    fd = OpenReadOnly(cupx_path.c_str());
  } catch (...) {
    return {};
  }

  if (!SkipCupxHeader(fd))
    return {};

  char entry[520];
  if (image_name.size() + 5 >= sizeof(entry))
    return {};

  std::memcpy(entry, "Pics/", 5);
  std::memcpy(entry + 5, image_name.data(), image_name.size());

  return ExtractByForwardScan(fd, {entry, 5 + image_name.size()});
}

std::vector<std::byte>
CupxArchive::ExtractPointsCup(Path cupx_path)
{
  UniqueFileDescriptor fd;
  try {
    fd = OpenReadOnly(cupx_path.c_str());
  } catch (...) {
    return {};
  }

  const off_t filesize = fd.GetSize();
  if (filesize < 0)
    return {};

  return ExtractByEocdScan(fd, filesize, "POINTS.CUP");
}
