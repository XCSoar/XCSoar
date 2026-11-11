// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/ColorGlyph.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/custom/UncompressedImage.hpp"
#include "util/ScopeExit.hxx"

#include <CoreText/CoreText.h>

#include <algorithm>
#include <memory>

#include <string.h>

/**
 * The color which Core Text has drawn is premultiplied with the
 * alpha channel, while the canvas expects it separated.
 */
static void
Unpremultiply(uint8_t *data, std::size_t n) noexcept
{
  for (std::size_t i = 0; i < n; i += 4) {
    const unsigned alpha = data[i + 3];
    if (alpha == 0 || alpha == 0xff)
      continue;

    for (unsigned j = 0; j < 3; ++j)
      data[i + j] = (data[i + j] * 0xff + alpha / 2) / alpha;
  }
}

bool
RenderColorGlyph(const char *text, unsigned size, Bitmap &bitmap) noexcept
{
  if (text == nullptr || *text == '\0' || size == 0)
    return false;

  CFStringRef string = CFStringCreateWithCString(nullptr, text,
                                                 kCFStringEncodingUTF8);
  if (string == nullptr)
    return false;

  AtScopeExit(string) { CFRelease(string); };

  const CFRange range = CFRangeMake(0, CFStringGetLength(string));
  if (range.length == 0)
    return false;

  /* which font would the system use for this text?  The cascade
     begins at the font of the user interface, not at a name which
     happens to exist; only a font with color glyphs, i.e. Apple Color
     Emoji, is worth this path */
  CTFontRef base = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem,
                                                 size, nullptr);
  if (base == nullptr)
    return false;

  CTFontRef font = CTFontCreateForString(base, string, range);
  CFRelease(base);
  if (font == nullptr)
    return false;

  AtScopeExit(font) { CFRelease(font); };

  if ((CTFontGetSymbolicTraits(font) & kCTFontTraitColorGlyphs) == 0)
    /* a plain outline font: let the caller draw the character as
       text, which is cheaper and can be colored */
    return false;

  const std::size_t pitch = size * 4;
  auto data = std::make_unique<uint8_t[]>(pitch * size);
  memset(data.get(), 0, pitch * size);

  CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
  if (rgb == nullptr)
    return false;

  const auto bitmap_info =
    CGBitmapInfo(uint32_t(kCGImageAlphaPremultipliedLast) |
                 uint32_t(kCGBitmapByteOrder32Big));

  CGContextRef ctx =
    CGBitmapContextCreate(data.get(), size, size, 8, pitch, rgb, bitmap_info);
  CFRelease(rgb);
  if (ctx == nullptr)
    return false;

  AtScopeExit(ctx) { CFRelease(ctx); };

  const void *keys[] = {kCTFontAttributeName};
  const void *values[] = {font};
  CFDictionaryRef attributes =
    CFDictionaryCreate(nullptr, keys, values, 1,
                       &kCFTypeDictionaryKeyCallBacks,
                       &kCFTypeDictionaryValueCallBacks);
  if (attributes == nullptr)
    return false;

  CFAttributedStringRef attributed =
    CFAttributedStringCreate(nullptr, string, attributes);
  CFRelease(attributes);
  if (attributed == nullptr)
    return false;

  CTLineRef line = CTLineCreateWithAttributedString(attributed);
  CFRelease(attributed);
  if (line == nullptr)
    return false;

  AtScopeExit(line) { CFRelease(line); };

  const CGRect bounds = CTLineGetImageBounds(line, ctx);

  /* a flag, a family or any other sequence of characters is wider
     than one glyph and would be cut off by the square: draw the whole
     line smaller instead */
  const CGFloat room = std::max(bounds.size.width, CGFloat(size));
  const CGFloat scale = CGFloat(size) / room;

  if (scale < 1)
    CGContextScaleCTM(ctx, scale, scale);

  /* center the glyph in the square */
  CGContextSetTextPosition(ctx,
                           (room - bounds.size.width) / 2 - bounds.origin.x,
                           (room - bounds.size.height) / 2 - bounds.origin.y);
  CTLineDraw(line, ctx);

  Unpremultiply(data.get(), pitch * size);

  return bitmap.Load(UncompressedImage(UncompressedImage::Format::RGBA,
                                       pitch, size, size, std::move(data)));
}
