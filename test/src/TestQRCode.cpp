// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "QR/QRCode.hpp"
#include "TestUtil.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

/* The camera decoder looks for finder patterns in a photograph, so a
   one-pixel module is not a readable image.  Scale the way the dialog
   does: several pixels per module, black as 0 and white as 255. */
static std::string
RoundTrip(const QRSymbol &symbol, unsigned scale)
{
  if (!symbol.IsValid() || scale == 0) return {};

  const unsigned width  = symbol.width * scale;
  const unsigned height = symbol.height * scale;
  std::vector<uint8_t> luminance(static_cast<std::size_t>(width) * height,
                                 255);

  for (unsigned y = 0; y < symbol.height; ++y) {
    for (unsigned x = 0; x < symbol.width; ++x) {
      if (!symbol.IsBlack(x, y)) continue;

      for (unsigned dy = 0; dy < scale; ++dy) {
        auto *row = luminance.data() +
                    (static_cast<std::size_t>(y) * scale + dy) * width +
                    static_cast<std::size_t>(x) * scale;
        std::fill_n(row, scale, static_cast<uint8_t>(0));
      }
    }
  }

  return DecodeQRCode(
    luminance.data(), luminance.size(), width, height, width, 1);
}

/* Paint the centre mark white, which is what the swift covers. */
static std::string
RoundTripWithCentre(const QRSymbol &symbol, unsigned scale)
{
  if (!symbol.IsValid() || scale == 0) return {};

  const QRCentre mark = QRCodeCentreMark(symbol);
  if (mark.size == 0) return {};

  const unsigned width  = symbol.width * scale;
  const unsigned height = symbol.height * scale;
  std::vector<uint8_t> luminance(static_cast<std::size_t>(width) * height,
                                 255);

  for (unsigned y = 0; y < symbol.height; ++y) {
    for (unsigned x = 0; x < symbol.width; ++x) {
      if (!symbol.IsBlack(x, y)) continue;
      if (x >= mark.left && x < mark.left + mark.size && y >= mark.top &&
          y < mark.top + mark.size)
        continue;

      for (unsigned dy = 0; dy < scale; ++dy) {
        auto *row = luminance.data() +
                    (static_cast<std::size_t>(y) * scale + dy) * width +
                    static_cast<std::size_t>(x) * scale;
        std::fill_n(row, scale, static_cast<uint8_t>(0));
      }
    }
  }

  return DecodeQRCode(
    luminance.data(), luminance.size(), width, height, width, 1);
}

int
main()
{
  plan_tests(4);

  const auto empty = EncodeQRCode({});
  ok1(!empty.IsValid());

  static constexpr auto payload = "XCTSK:ping";
  const auto symbol             = EncodeQRCode(payload);
  ok1(symbol.IsValid());
  ok1(RoundTrip(symbol, 8) == payload);
  ok1(RoundTripWithCentre(symbol, 8) == payload);

  return exit_status();
}
