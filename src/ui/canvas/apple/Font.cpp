/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Look/FontDescription.hpp"
#include "ui/canvas/Font.hpp"
#include "util/ScopeExit.hxx"
#include "util/TStringView.hxx"

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

void
Font::Load(const FontDescription &d)
{
  NativeFontT *native_font;

#ifndef ENABLE_OPENGL
  const std::lock_guard<Mutex> lock(apple_font_mutex);
#endif

  if (d.IsMonospace())
    native_font = [NativeFontT fontWithName: @"Courier" size: d.GetHeight()];
  else
    native_font = [NativeFontT fontWithName: @"Helvetica" size: d.GetHeight()];

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
#else
    UIFontDescriptorSymbolicTraits mask = 0;
    if (d.IsBold())
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
}

PixelSize
Font::TextSize(const TCHAR *text) const noexcept
{
  assert(nil != draw_attributes);

  NSString *ns_str =
      [NSString stringWithCString: text encoding: NSUTF8StringEncoding];
  assert(nil != ns_str);

#ifndef ENABLE_OPENGL
  const std::lock_guard<Mutex> lock(apple_font_mutex);
#endif

  CGSize size = [ns_str sizeWithAttributes: draw_attributes];
  return PixelSize(static_cast<int>(ceilf(size.width)),
                   static_cast<int>(ceilf(size.height)));
}

void
Font::Render(TStringView text, const PixelSize size,
             void *buffer) const noexcept
{
  assert(nil != draw_attributes);

  NSString *ns_str =
      [[NSString alloc] initWithBytes: text.data length: text.size encoding: NSUTF8StringEncoding];
  assert(nil != ns_str);

  memset(buffer, 0, size.width * size.height);

  static CGColorSpaceRef grey_colorspace = CGColorSpaceCreateDeviceGray();
  CGContextRef ctx = CGBitmapContextCreate(buffer, size.width, size.height, 8,
                                           size.width, grey_colorspace,
                                           kCGImageAlphaOnly);
  assert(nullptr != ctx);

  AtScopeExit(ctx) { CFRelease(ctx); };

#ifndef ENABLE_OPENGL
  const std::lock_guard<Mutex> lock(apple_font_mutex);
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
  [ns_str drawAtPoint: p withAttributes: draw_attributes];
}
