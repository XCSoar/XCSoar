// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LogoView.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/FontDescription.hpp"
#include "util/Compiler.h"
#include "Resources.hpp"
#include "Version.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

#include <algorithm>

LogoView::LogoView() noexcept try
  :logo(IDB_LOGO), big_logo(IDB_LOGO_HD), huge_logo(IDB_LOGO_UHD),
   title(IDB_TITLE), big_title(IDB_TITLE_HD), huge_title(IDB_TITLE_UHD)
{
#ifndef USE_WIN32_RESOURCES
  /* Load RGBA logo variants for dark mode (transparent background) */
  logo_rgba.Load(IDB_LOGO_RGBA);
  big_logo_rgba.Load(IDB_LOGO_HD_RGBA);
  huge_logo_rgba.Load(IDB_LOGO_UHD_RGBA);
  /* Load the white title variant for dark mode */
  white_title.Load(IDB_TITLE_HD_WHITE);
#endif
#ifndef USE_GDI
  font.Load(FontDescription(Layout::FontScale(10)));
#ifndef NDEBUG
  FontDescription bold_desc(Layout::FontScale(16));
  bold_desc.SetBold(true);
  bold_font.Load(bold_desc);
#endif
#endif
} catch (...) {
  /* ignore Bitmap/Font loader exceptions */
}

static int
Center(unsigned canvas_size, unsigned element_size)
{
  /* cast to int to force signed integer division, just in case the
     difference is negative */
  return int(canvas_size - element_size) / 2;
}

enum class LogoViewOrientation {
    LANDSCAPE, PORTRAIT, SQUARE,
};

static constexpr PixelSize
EstimateLogoViewSize(LogoViewOrientation orientation,
                     PixelSize logo_size,
                     PixelSize title_size,
                     unsigned spacing) noexcept
{
  switch (orientation) {
  case LogoViewOrientation::LANDSCAPE:
    return {
      logo_size.width + spacing + title_size.width,
      logo_size.height,
    };

  case LogoViewOrientation::PORTRAIT:
    return {
      title_size.width,
      logo_size.height + spacing + title_size.height,
    };

  case LogoViewOrientation::SQUARE:
    return logo_size;
  }

  gcc_unreachable();
}

void
LogoView::draw(Canvas &canvas, const PixelRect &rc,
               bool dark_mode) noexcept
{
  /* Return only if all logo and title variants are missing */
  if (!huge_logo.IsDefined() && !big_logo.IsDefined() && !logo.IsDefined() &&
      !huge_title.IsDefined() && !big_title.IsDefined() && !title.IsDefined())
    return;

  const unsigned width = rc.GetWidth(), height = rc.GetHeight();

  LogoViewOrientation orientation;
  if (width == height)
    orientation = LogoViewOrientation::SQUARE;
  else if (width > height)
    orientation = LogoViewOrientation::LANDSCAPE;
  else
    orientation = LogoViewOrientation::PORTRAIT;

  /* Select appropriate bitmap size based on display dimensions */
  /* Pick the highest-resolution bitmap that IsDefined() (fall back from huge -> big -> logo) */
  const Bitmap *bitmap_logo, *bitmap_title;
  
  if ((orientation == LogoViewOrientation::LANDSCAPE && width >= 1024 && height >= 340) ||
      (orientation == LogoViewOrientation::PORTRAIT && width >= 660 && height >= 500) ||
      (orientation == LogoViewOrientation::SQUARE && width >= 420 && height >= 420)) {
    /* Use huge (320px logo, 640px title) for very high resolution displays */
    if (huge_logo.IsDefined() && huge_title.IsDefined()) {
      bitmap_logo = &huge_logo;
      bitmap_title = &huge_title;
    } else if (big_logo.IsDefined() && big_title.IsDefined()) {
      bitmap_logo = &big_logo;
      bitmap_title = &big_title;
    } else {
      bitmap_logo = &logo;
      bitmap_title = &title;
    }
  } else if ((orientation == LogoViewOrientation::LANDSCAPE && width >= 510 && height >= 170) ||
             (orientation == LogoViewOrientation::PORTRAIT && width >= 330 && height >= 250) ||
             (orientation == LogoViewOrientation::SQUARE && width >= 210 && height >= 210)) {
    /* Use big (160px logo, 320px title) for HD displays */
    if (big_logo.IsDefined() && big_title.IsDefined()) {
      bitmap_logo = &big_logo;
      bitmap_title = &big_title;
    } else {
      bitmap_logo = &logo;
      bitmap_title = &title;
    }
  } else {
    /* Use standard (80px logo, 110px title) for low resolution displays */
    bitmap_logo = &logo;
    bitmap_title = &title;
  }

  /* In dark mode, use RGBA logos (transparent background) if available */
  if (dark_mode) {
    if (bitmap_logo == &huge_logo && huge_logo_rgba.IsDefined())
      bitmap_logo = &huge_logo_rgba;
    else if (bitmap_logo == &big_logo && big_logo_rgba.IsDefined())
      bitmap_logo = &big_logo_rgba;
    else if (bitmap_logo == &logo && logo_rgba.IsDefined())
      bitmap_logo = &logo_rgba;
  }

  // Determine logo size
  PixelSize logo_size = bitmap_logo->GetSize();

  // Determine title image size
  PixelSize title_size = bitmap_title->GetSize();

  unsigned spacing = title_size.height / 2;

  const auto estimated_size = EstimateLogoViewSize(orientation, logo_size,
                                                   title_size, spacing);

  const unsigned magnification =
    std::min((width - 16u) / estimated_size.width,
             (height - 16u) / estimated_size.height);

  if (magnification > 1) {
    logo_size.width *= magnification;
    logo_size.height *= magnification;
    title_size.width *= magnification;
    title_size.height *= magnification;
    spacing *= magnification;
  }

  PixelPoint logo_position, title_position;

  // Determine logo and title positions
  switch (orientation) {
  case LogoViewOrientation::LANDSCAPE:
    logo_position.x = Center(width, logo_size.width + spacing + title_size.width);
    logo_position.y = Center(height, logo_size.height);
    title_position.x = logo_position.x + logo_size.width + spacing;
    title_position.y = Center(height, title_size.height);
    break;
  case LogoViewOrientation::PORTRAIT:
    logo_position.x = Center(width, logo_size.width);
    logo_position.y = Center(height, logo_size.height + spacing + title_size.height);
    title_position.x = Center(width, title_size.width);
    title_position.y = logo_position.y + logo_size.height + spacing;
    break;
  case LogoViewOrientation::SQUARE:
    logo_position.x = Center(width, logo_size.width);
    logo_position.y = Center(height, logo_size.height);
    // not needed - silence compiler "may be used uninitialized"
    title_position.x = 0;
    title_position.y = 0;
    break;
  default:
    gcc_unreachable();
  }

  // Draw 'XCSoar N.N' title
  if (orientation != LogoViewOrientation::SQUARE) {
    const Bitmap *draw_title = bitmap_title;
    if (dark_mode && white_title.IsDefined())
      draw_title = &white_title;

#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    canvas.Stretch(title_position, title_size, *draw_title);
  }

  // Draw XCSoar swift logo
  {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    canvas.Stretch(logo_position, logo_size, *bitmap_logo);
  }

  // Draw full XCSoar version number

#ifndef USE_GDI
  if (!font.IsDefined())
    return;

  canvas.Select(font);
#endif

  canvas.SetTextColor(dark_mode ? COLOR_WHITE : COLOR_BLACK);
  canvas.SetBackgroundTransparent();
  canvas.DrawText({2, 2}, XCSoar_ProductToken);

#ifndef NDEBUG
  /* Draw debug build warning banner below logo (like "Remove before flight") */
#ifndef USE_GDI
  if (bold_font.IsDefined())
    canvas.Select(bold_font);
#endif
  
  const TCHAR *warning_text = _T("DEBUG BUILD - DO NOT FLY!");
  const auto text_size = canvas.CalcTextSize(warning_text);
  
  /* Half character padding (max) */
  const int padding = text_size.height / 2;
  const int banner_height = text_size.height + padding * 2;
  const int banner_width = text_size.width + padding * 2;
  
  /* Position banner below the logo/title with some spacing */
  int banner_y;
  if (orientation == LogoViewOrientation::PORTRAIT) {
    // Below title in portrait mode
    banner_y = title_position.y + title_size.height + spacing / 2;
  } else if (orientation == LogoViewOrientation::SQUARE) {
    // Below logo in square mode
    banner_y = logo_position.y + logo_size.height + spacing / 2;
  } else {
    // Below whichever is lower in landscape mode
    banner_y = std::max(logo_position.y + logo_size.height,
                       title_position.y + title_size.height) + spacing / 2;
  }
  
  /* Only draw if banner fits within the visible area */
  if (banner_y + banner_height <= int(height)) {
    const PixelRect warning_rect{
      Center(width, banner_width),
      banner_y,
      Center(width, banner_width) + banner_width,
      banner_y + banner_height
    };
    
    canvas.DrawFilledRectangle(warning_rect, COLOR_RED);
    canvas.SetTextColor(COLOR_WHITE);
    canvas.SetBackgroundTransparent();
    
    const PixelPoint text_pos{
      warning_rect.left + padding,
      warning_rect.top + padding
    };
    
    canvas.DrawText(text_pos, warning_text);
  }
#endif
}
