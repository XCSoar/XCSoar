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
  /* Transparent variants for OpenGL compositing on tinted dialog
     backgrounds (light and dark). */
  logo_rgba.Load(IDB_LOGO_RGBA);
  big_logo_rgba.Load(IDB_LOGO_HD_RGBA);
  huge_logo_rgba.Load(IDB_LOGO_UHD_RGBA);
  title_rgba.Load(IDB_TITLE_HD_RGBA);
  white_title.Load(IDB_TITLE_HD_WHITE);
  huge_white_title.Load(IDB_TITLE_UHD_WHITE);
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

#ifdef ENABLE_OPENGL
/**
 * If @a selected points at @a match and @a variant is loaded, switch
 * to the variant (used for transparent OpenGL assets).
 */
static void
UseVariant(const Bitmap *&selected,
           const Bitmap &match, const Bitmap &variant) noexcept
{
  if (selected == &match && variant.IsDefined())
    selected = &variant;
}
#endif

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

  /* Pick the highest-resolution bitmap that IsDefined() */
  const Bitmap *bitmap_logo, *bitmap_title;

  if ((orientation == LogoViewOrientation::LANDSCAPE && width >= 1024 && height >= 340) ||
      (orientation == LogoViewOrientation::PORTRAIT && width >= 660 && height >= 500) ||
      (orientation == LogoViewOrientation::SQUARE && width >= 420 && height >= 420)) {
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
    if (big_logo.IsDefined() && big_title.IsDefined()) {
      bitmap_logo = &big_logo;
      bitmap_title = &big_title;
    } else {
      bitmap_logo = &logo;
      bitmap_title = &title;
    }
  } else {
    bitmap_logo = &logo;
    bitmap_title = &title;
  }

#ifdef ENABLE_OPENGL
  /* Opaque BMP-derived assets leave white squares on tinted dialog
     backgrounds; prefer transparent variants when available. */
  UseVariant(bitmap_logo, huge_logo, huge_logo_rgba);
  UseVariant(bitmap_logo, big_logo, big_logo_rgba);
  UseVariant(bitmap_logo, logo, logo_rgba);

  if (dark_mode) {
    UseVariant(bitmap_title, huge_title, huge_white_title);
    /* Fall back UHD→HD white title without replacing base SD. */
    UseVariant(bitmap_title, huge_title, white_title);
    UseVariant(bitmap_title, big_title, white_title);
  } else {
    /* No UHD black RGBA title; fall back to HD. */
    UseVariant(bitmap_title, huge_title, title_rgba);
    UseVariant(bitmap_title, big_title, title_rgba);
  }
#endif

  PixelSize logo_size = bitmap_logo->GetSize();
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
    title_position.x = 0;
    title_position.y = 0;
    break;
  default:
    gcc_unreachable();
  }

  if (orientation != LogoViewOrientation::SQUARE) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    canvas.Stretch(title_position, title_size, *bitmap_title);
  }

  {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    canvas.Stretch(logo_position, logo_size, *bitmap_logo);
  }

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

  const char *warning_text = "DEBUG BUILD - DO NOT FLY!";
  const auto text_size = canvas.CalcTextSize(warning_text);

  const int padding = text_size.height / 2;
  const int banner_height = text_size.height + padding * 2;
  const int banner_width = text_size.width + padding * 2;

  int banner_y;
  if (orientation == LogoViewOrientation::PORTRAIT)
    banner_y = title_position.y + title_size.height + spacing / 2;
  else if (orientation == LogoViewOrientation::SQUARE)
    banner_y = logo_position.y + logo_size.height + spacing / 2;
  else
    banner_y = std::max(logo_position.y + logo_size.height,
                        title_position.y + title_size.height) + spacing / 2;

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

    canvas.DrawText({warning_rect.left + padding,
                     warning_rect.top + padding},
                    warning_text);
  }
#endif
}
