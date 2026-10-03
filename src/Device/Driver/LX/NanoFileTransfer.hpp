// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

/*
 * The LXNAV file transfer protocol, used by LXNAV's own Nano
 * configuration app to download flights in verified blocks:
 *
 *   -> PLXVC,FILE_INFO,R,/<file>,<window>,<payload>
 *   <- PLXVC,FILE_DATA,A,0,<block>,<crc>,<base64>   (one per block)
 *   -> PLXVC,FILE_OK,R,<next block>                 (after each window)
 *   -> PLXVC,FILE_DATA_LOST,R,<block>               (resend from block)
 *   <- PLXVC,FILE_CRC32,A,<crc>                     (after the last block)
 *   -> PLXVC,FILE_CRC_OK,R
 */
namespace Nano {

/**
 * The CRC-32 the LXNAV file transfer uses.  It is the reflected
 * CRC-32 (polynomial 0xedb88320), except that every right shift
 * copies the sign bit in, as a signed 32 bit shift does; the logger
 * computes it that way.  Values are printed as signed decimals.
 */
class FileTransferCrc {
  uint32_t value = 0xffffffff;

public:
  void Update(std::span<const std::byte> data) noexcept;

  void Update(std::string_view data) noexcept {
    Update(std::as_bytes(std::span{data}));
  }

  [[gnu::pure]]
  int32_t Get() const noexcept {
    return static_cast<int32_t>(~value);
  }
};

/**
 * One parsed "FILE_DATA,A" sentence, the part after that prefix.
 */
struct FileDataBlock {
  unsigned number;

  int32_t crc;

  /** the payload, still base64 encoded */
  std::string_view base64;
};

[[gnu::pure]]
std::optional<FileDataBlock>
ParseFileDataBlock(std::string_view line) noexcept;

/**
 * Decode standard base64 (with "=" padding) into @p dest.
 *
 * @return the number of bytes, or std::nullopt if @p src is not
 * valid base64 or does not fit
 */
std::optional<std::size_t>
DecodeBase64(std::string_view src, std::span<std::byte> dest) noexcept;

} // namespace Nano
