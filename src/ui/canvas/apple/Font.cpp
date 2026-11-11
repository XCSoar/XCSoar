// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Look/FontDescription.hpp"
#include "ui/canvas/Font.hpp"
#include "util/ScopeExit.hxx"

#ifndef ENABLE_OPENGL
#include "thread/Mutex.hxx"
#endif

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>

#include <math.h>
#include <string.h>

#import <CoreGraphics/CoreGraphics.h>

#ifdef USE_APPKIT
#import <AppKit/AppKit.h>
#elif defined(USE_UIKIT)
#import <UIKit/UIKit.h>
#else
#error No font renderer
#endif


#ifndef ENABLE_OPENGL
/**
 * Apple's APIs are not completely thread-safe; this global Mutex is used to
 * protect them from multi-threaded access.
 */
static Mutex apple_font_mutex;
#endif

using NativeFontT =
#ifdef USE_APPKIT
  NSFont;
#else
  UIFont;
#endif

/**
 * Draw the string into the given alpha-only buffer.  The caller is
 * responsible for locking #apple_font_mutex.
 */
static void
RenderNSString(NSString *ns_str, NSDictionary *attributes,
               const PixelSize size, void *buffer) noexcept
{
  memset(buffer, 0, size.width * size.height);

  static CGColorSpaceRef grey_colorspace = CGColorSpaceCreateDeviceGray();
  CGContextRef ctx = CGBitmapContextCreate(buffer, size.width, size.height, 8,
                                           size.width, grey_colorspace,
                                           kCGImageAlphaOnly);
  assert(nullptr != ctx);

  AtScopeExit(ctx) { CFRelease(ctx); };

#ifdef USE_APPKIT
  NSGraphicsContext *ns_ctx =
      [NSGraphicsContext graphicsContextWithCGContext: ctx flipped: false];
  assert(nil != ns_ctx);

  [NSGraphicsContext saveGraphicsState];
  [NSGraphicsContext setCurrentContext: ns_ctx];
#else
  CGContextTranslateCTM(ctx, 0, size.height);
  CGContextScaleCTM(ctx, 1, -1);

  UIGraphicsPushContext(ctx);
#endif

  AtScopeExit() {
#ifdef USE_APPKIT
    [NSGraphicsContext restoreGraphicsState];
#else
    UIGraphicsPopContext();
#endif
  };

  static CGPoint p = CGPointMake(0, 0);
  [ns_str drawAtPoint: p withAttributes: attributes];
}

void
Font::Load(const FontDescription &d)
{
  NativeFontT *native_font;

  /* the bold cut of the system font is asked for directly, so only
     italic (and bold for the monospace font) is left to the trait
     conversion below */
  bool convert_bold = d.IsBold();

#ifndef ENABLE_OPENGL
  const std::lock_guard lock{apple_font_mutex};
#endif

  if (d.IsMonospace()) {
    native_font = [NativeFontT fontWithName: @"Courier" size: d.GetHeight()];
  } else {
    /* the system font covers far more characters than Helvetica (e.g.
       U+232B, the backspace symbol), and its bold cut is designed
       rather than synthesised */
    native_font = convert_bold
      ? [NativeFontT boldSystemFontOfSize: d.GetHeight()]
      : [NativeFontT systemFontOfSize: d.GetHeight()];
    convert_bold = false;
  }

  if (nil == native_font)
    throw std::runtime_error{"no native font"};

  if (d.IsItalic() || convert_bold) {
#ifdef USE_APPKIT
    NSFontTraitMask mask = 0;
    if (convert_bold)
      mask |= NSBoldFontMask;
    if (d.IsItalic())
      mask |= NSItalicFontMask;
    native_font = [[NSFontManager sharedFontManager]
        convertFont: native_font
        toHaveTrait: mask];
#else
    /* start from what the font already has: -fontDescriptorWithSymbolicTraits:
       replaces the traits, and the bold cut asked for above would be
       lost when italic is added here */
    UIFontDescriptorSymbolicTraits mask =
      native_font.fontDescriptor.symbolicTraits;
    if (convert_bold)
      mask |= UIFontDescriptorTraitBold;
    if (d.IsItalic())
      mask |= UIFontDescriptorTraitItalic;
    UIFontDescriptor *font_desc =
        [native_font.fontDescriptor fontDescriptorWithSymbolicTraits: mask];
    native_font = [UIFont fontWithDescriptor: font_desc size: d.GetHeight()];
#endif
  }

  draw_attributes = @{ NSFontAttributeName: native_font };

  height = ceilf([@"ÄjX€µ" sizeWithAttributes: draw_attributes].height);
  ascent_height = static_cast<unsigned>(ceilf([native_font ascender]));
  capital_height = static_cast<unsigned>(ceilf([native_font capHeight]));

  /* The metrics above describe the abstract line box, but the ink
     drawAtPoint places can sit lower.  Measure a reference capital
     so the metrics match the bitmaps this font actually produces. */
  NSString *const reference = @"H";
  const CGSize reference_size = [reference sizeWithAttributes: draw_attributes];
  const PixelSize size(static_cast<int>(ceilf(reference_size.width)),
                       static_cast<int>(ceilf(reference_size.height)));
  if (size.width > 0 && size.height > 0) {
    const std::unique_ptr<uint8_t[]> buffer{new uint8_t[size.width * size.height]};
    RenderNSString(reference, draw_attributes, size, buffer.get());

    int first = -1, last = -1;
    for (unsigned y = 0; y < size.height; ++y) {
      const uint8_t *row = buffer.get() + y * size.width;
      /* ignore the faintest anti-aliasing smear */
      if (std::any_of(row, row + size.width,
                      [](uint8_t alpha) { return alpha > 0x10; })) {
        if (first < 0)
          first = y;
        last = y;
      }
    }

    if (first >= 0) {
      /* the baseline sits directly below the capital's ink */
      ascent_height = last + 1;
      capital_height = last - first + 1;
    }
  }
}

bool
Font::HasGlyph([[maybe_unused]] unsigned unicode) const noexcept
{
  /* CoreText falls back to other fonts of the operating system */
  return true;
}

PixelSize
Font::TextSize(const std::string_view text) const noexcept
{
  assert(nil != draw_attributes);

  NSString *ns_str =
    [[NSString alloc] initWithBytes: text.data() length: text.size() encoding: NSUTF8StringEncoding];
  assert(nil != ns_str);

#ifndef ENABLE_OPENGL
  const std::lock_guard lock{apple_font_mutex};
#endif

  CGSize size = [ns_str sizeWithAttributes: draw_attributes];
  return PixelSize(static_cast<int>(ceilf(size.width)),
                   static_cast<int>(ceilf(size.height)));
}

void
Font::Render(std::string_view text, const PixelSize size,
             void *buffer) const noexcept
{
  assert(nil != draw_attributes);

  NSString *ns_str =
    [[NSString alloc] initWithBytes: text.data() length: text.size() encoding: NSUTF8StringEncoding];
  assert(nil != ns_str);

#ifndef ENABLE_OPENGL
  const std::lock_guard lock{apple_font_mutex};
#endif

  RenderNSString(ns_str, draw_attributes, size, buffer);
}
