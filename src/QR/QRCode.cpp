// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "QRCode.hpp"

#include <ZXing/CreateBarcode.h>
#include <ZXing/ImageView.h>
#include <ZXing/ReadBarcode.h>
#include <limits>

QRCentre
QRCodeCentreMark(const QRSymbol &symbol) noexcept
{
  QRCentre mark;

  /* Quiet zone, finder, and the white separator beside the finder. */
  constexpr unsigned finder = 4 + 7 + 1;
  if (!symbol.IsValid() || symbol.width != symbol.height ||
      symbol.width <= 2 * finder)
    return mark;

  const unsigned max_span = symbol.width - 2 * finder;
  unsigned span           = symbol.width * 28 / 100;
  if (span > max_span) span = max_span;
  if (span < 5) return mark;

  mark.size = span;
  mark.left = (symbol.width - span) / 2;
  mark.top  = (symbol.height - span) / 2;
  return mark;
}

QRSymbol
EncodeQRCode(std::string_view text)
try {
  QRSymbol symbol;
  if (text.empty()) return symbol;

  /* The old writer is asked for the module grid (no scaled bitmap).
     It does not add a quiet zone, so four white modules are added
     here.  That zone is part of the symbol a caller draws.
     ecLevel 5 is quartile correction (~25%): the centre mark painted
     over the symbol has to be recoverable. */
  const auto barcode = ZXing::CreateBarcodeFromText(
    text, ZXing::CreatorOptions{ZXing::BarcodeFormat::QRCode, "ecLevel=5"});
  const auto image = barcode.symbol();
  if (!barcode.isValid() || image.data() == nullptr || image.width() <= 0 ||
      image.height() <= 0)
    return symbol;

  constexpr unsigned quiet = 4;
  symbol.width             = static_cast<unsigned>(image.width()) + 2 * quiet;
  symbol.height            = static_cast<unsigned>(image.height()) + 2 * quiet;
  symbol.modules.assign(static_cast<std::size_t>(symbol.width) * symbol.height,
                        0);

  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      /* symbol() is a luminance image: a 0 byte is a black module. */
      if (*image.data(x, y) != 0) continue;

      const auto dx = static_cast<unsigned>(x) + quiet;
      const auto dy = static_cast<unsigned>(y) + quiet;
      symbol.modules[static_cast<std::size_t>(dy) * symbol.width + dx] = 1;
    }
  }

  return symbol;
} catch (...) {
  return {};
}

std::string
DecodeQRCode(const uint8_t *luminance,
             std::size_t size,
             unsigned width,
             unsigned height,
             unsigned row_stride,
             unsigned pixel_stride) noexcept
try {
  if (luminance == nullptr || size == 0 || width == 0 || height == 0 ||
      row_stride == 0 || pixel_stride == 0 ||
      size > size_t(std::numeric_limits<int>::max()))
    /* ImageView would throw, and the catch below cannot tell a bad
       caller from a frame with no QR code in it */
    return {};

  /* The strides are passed through, so a padded camera plane needs no
     repacking before it gets here.  This is the bounds-checking
     constructor: it rejects a buffer too small for the geometry, so we
     do not have to reproduce that arithmetic at every call site. */
  const ZXing::ImageView image{luminance,
                               static_cast<int>(size),
                               static_cast<int>(width),
                               static_cast<int>(height),
                               ZXing::ImageFormat::Lum,
                               static_cast<int>(row_stride),
                               static_cast<int>(pixel_stride)};

  /* tryHarder and tryRotate are off: a camera caller runs this on
     every preview frame, and the pilot can hold the camera still for
     another one.  The library is compiled with QR support only, but
     the format is restricted here too so that a build with more
     formats enabled cannot start returning barcodes we have no use
     for. */
  const auto options = ZXing::ReaderOptions{}
                         .setFormats(ZXing::BarcodeFormat::QRCode)
                         .setTryHarder(false)
                         .setTryRotate(false)
                         .setTryInvert(false)
                         .setMaxNumberOfSymbols(1);

  const auto barcode = ZXing::ReadBarcode(image, options);
  if (!barcode.isValid()) return {};

  return barcode.text();
} catch (...) {
  /* not expected, but a decoder failure must never take down the
     camera thread */
  return {};
}
