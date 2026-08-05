// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Look/FontDescription.hpp"
#include "ui/canvas/Font.hpp"
#include "util/ScopeExit.hxx"

#ifndef ENABLE_OPENGL
#include "thread/Mutex.hxx"
#endif

#include <cassert>
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

#ifdef USE_UIKIT
static bool
IsAtLeastIOS7() noexcept
{
  static const bool at_least_ios7 =
    [[[UIDevice currentDevice] systemVersion] compare: @"7.0"
        options: NSNumericSearch] != NSOrderedAscending;

  return at_least_ios7;
}

static NSString *
GetUIFontName(const FontDescription &d) noexcept
{
  if (d.IsMonospace()) {
    if (d.IsBold() && d.IsItalic())
      return @"Courier-BoldOblique";

    if (d.IsBold())
      return @"Courier-Bold";

    if (d.IsItalic())
      return @"Courier-Oblique";

    return @"Courier";
  }

  if (d.IsBold() && d.IsItalic())
    return @"Helvetica-BoldOblique";

  if (d.IsBold())
    return @"Helvetica-Bold";

  if (d.IsItalic())
    return @"Helvetica-Oblique";

  return @"Helvetica";
}

static UIFont *
GetUIFontFromAttributes(NSDictionary *attributes) noexcept
{
  return [attributes objectForKey: NSFontAttributeName];
}
#endif

void
Font::Load(const FontDescription &d)
{
  NativeFontT *native_font;

#ifndef ENABLE_OPENGL
  const std::lock_guard lock{apple_font_mutex};
#endif

#ifdef USE_UIKIT
  native_font = [NativeFontT fontWithName: GetUIFontName(d) size: d.GetHeight()];
#else
  if (d.IsMonospace())
    native_font = [NativeFontT fontWithName: @"Courier" size: d.GetHeight()];
  else
    native_font = [NativeFontT fontWithName: @"Helvetica" size: d.GetHeight()];
#endif

  if (nil == native_font)
    throw std::runtime_error{"fontWithName named"};

  if (d.IsItalic() || d.IsBold()) {
#ifdef USE_APPKIT
    NSFontTraitMask mask = 0;
    if (d.IsBold())
      mask |= NSBoldFontMask;
    if (d.IsItalic())
      mask |= NSItalicFontMask;
    native_font = [[NSFontManager sharedFontManager]
        convertFont: native_font
        toHaveTrait: mask];
#elif defined(__IPHONE_7_0)
    if (IsAtLeastIOS7()) {
    UIFontDescriptorSymbolicTraits mask = 0;
    if (d.IsBold())
      mask |= UIFontDescriptorTraitBold;
    if (d.IsItalic())
      mask |= UIFontDescriptorTraitItalic;
    UIFontDescriptor *font_desc =
        [native_font.fontDescriptor fontDescriptorWithSymbolicTraits: mask];
    native_font = [UIFont fontWithDescriptor: font_desc size: d.GetHeight()];
    }
#endif
  }

  draw_attributes = @{ NSFontAttributeName: native_font };

#ifdef USE_UIKIT
  CGSize size = IsAtLeastIOS7()
    ? [@"ÄjX€µ" sizeWithAttributes: draw_attributes]
    : [@"ÄjX€µ" sizeWithFont: native_font];
  height = ceilf(size.height);
#else
  height = ceilf([@"ÄjX€µ" sizeWithAttributes: draw_attributes].height);
#endif
  ascent_height = static_cast<unsigned>(ceilf([native_font ascender]));
  capital_height = static_cast<unsigned>(ceilf([native_font capHeight]));
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

  CGSize size;
#ifdef USE_UIKIT
  if (IsAtLeastIOS7())
    size = [ns_str sizeWithAttributes: draw_attributes];
  else
    size = [ns_str sizeWithFont: GetUIFontFromAttributes(draw_attributes)];
#else
  size = [ns_str sizeWithAttributes: draw_attributes];
#endif

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

  memset(buffer, 0, size.width * size.height);

  static CGColorSpaceRef grey_colorspace = CGColorSpaceCreateDeviceGray();
  CGContextRef ctx = CGBitmapContextCreate(buffer, size.width, size.height, 8,
                                           size.width, grey_colorspace,
                                           kCGImageAlphaOnly);
  assert(nullptr != ctx);

  AtScopeExit(ctx) { CFRelease(ctx); };

#ifndef ENABLE_OPENGL
  const std::lock_guard lock{apple_font_mutex};
#endif

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
#ifdef USE_UIKIT
  if (IsAtLeastIOS7())
    [ns_str drawAtPoint: p withAttributes: draw_attributes];
  else
    [ns_str drawAtPoint: p withFont: GetUIFontFromAttributes(draw_attributes)];
#else
  [ns_str drawAtPoint: p withAttributes: draw_attributes];
#endif
}
