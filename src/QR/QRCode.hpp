// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

/**
 * A square QR symbol, one byte per module.
 *
 * A non-zero module is black.  The quiet zone is part of the grid, so
 * a caller can draw the bytes and nothing else.
 */
struct QRSymbol
{
  unsigned width  = 0;
  unsigned height = 0;

  /** Row-major.  0 is white, anything else is black. */
  std::vector<uint8_t> modules;

  [[nodiscard]]
  bool IsValid() const noexcept
  {
    return width > 0 && height > 0 &&
           modules.size() == static_cast<std::size_t>(width) * height;
  }

  [[nodiscard]]
  bool IsBlack(unsigned x, unsigned y) const noexcept
  {
    return modules[static_cast<std::size_t>(y) * width + x] != 0;
  }
};

/**
 * Square of modules a centre mark may cover.
 *
 * The three finder patterns stay clear.  The size is zero when the
 * symbol is too small for a mark.
 */
struct QRCentre
{
  unsigned left = 0;
  unsigned top  = 0;
  unsigned size = 0;
};

/**
 * The largest centre square that leaves the finders alone.
 *
 * About a quarter of the symbol, and never the corner finders.  The
 * encoder uses the quartile error correction so this square can be
 * painted over and the code still scans.
 */
[[nodiscard]]
QRCentre QRCodeCentreMark(const QRSymbol &symbol) noexcept;

/**
 * Encode text as a QR symbol.
 *
 * Error correction is the quartile level, so a centre mark can cover
 * part of the symbol.  @return an empty symbol if the text is empty
 * or does not fit in a QR code
 */
QRSymbol EncodeQRCode(std::string_view text);

/**
 * Decode a QR code from a greyscale image.
 *
 * Free of platform types: Android hands over the Y plane of a Camera2
 * frame, and any other port can pass an 8-bit luminance buffer.
 *
 * @param luminance pixel bytes; not necessarily one byte per pixel
 * when @p pixel_stride is greater than one
 * @param size the buffer's size in bytes, so a buffer too small for
 * the geometry is rejected instead of being read past its end
 * @param row_stride bytes between the starts of two rows
 * @param pixel_stride bytes between two horizontally adjacent pixels
 *
 * @return the decoded text, or an empty string if the image contains
 * no readable QR code
 */
std::string DecodeQRCode(const uint8_t *luminance,
                         std::size_t size,
                         unsigned width,
                         unsigned height,
                         unsigned row_stride,
                         unsigned pixel_stride) noexcept;
