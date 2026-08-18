// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "QRDecoder.hpp"

#include <ZXing/ImageView.h>
#include <ZXing/ReadBarcode.h>

#include <limits>

std::string
DecodeQRCode(const uint8_t *luminance, std::size_t size,
             unsigned width, unsigned height,
             unsigned row_stride, unsigned pixel_stride) noexcept
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
  const ZXing::ImageView image{luminance, static_cast<int>(size),
                               static_cast<int>(width),
                               static_cast<int>(height),
                               ZXing::ImageFormat::Lum,
                               static_cast<int>(row_stride),
                               static_cast<int>(pixel_stride)};

  /* tryHarder and tryRotate are off: this runs on every preview frame,
     and the pilot can simply hold the camera still for another one.
     The library is compiled with QR support only, but the format is
     restricted here too so that a build with more formats enabled
     cannot start returning barcodes we have no use for. */
  const auto options = ZXing::ReaderOptions{}
    .setFormats(ZXing::BarcodeFormat::QRCode)
    .setTryHarder(false)
    .setTryRotate(false)
    .setTryInvert(false)
    .setMaxNumberOfSymbols(1);

  const auto barcode = ZXing::ReadBarcode(image, options);
  if (!barcode.isValid())
    return {};

  return barcode.text();
} catch (...) {
  /* not expected, but a decoder failure must never take down the
     camera thread */
  return {};
}
