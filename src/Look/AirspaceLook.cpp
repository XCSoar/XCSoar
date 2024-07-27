// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Look/AirspaceLook.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Resources.hpp"
#include "util/Macros.hpp"

const RGB8Color AirspaceLook::preset_colors[] = {
  RGB8_RED,
  RGB8_GREEN,
  RGB8_BLUE,
  RGB8_YELLOW,
  RGB8_MAGENTA,
  RGB8_CYAN,
  RGB8_GRAYISH_VIOLET,
  RGB8_RED.Darken(),
  RGB8_GREEN.Darken(),
  RGB8_BLUE.Darken(),
  RGB8_YELLOW.Darken(),
  RGB8_MAGENTA.Darken(),
  RGB8_CYAN.Darken(),
  RGB8_GRAYISH_VIOLET.Darken(),
  RGB8_WHITE,
  RGB8_LIGHT_GRAY,
  RGB8_GRAY,
  RGB8_BLACK,
};

void
AirspaceClassLook::Initialise(const AirspaceClassRendererSettings &settings)
{
  fill_color = Color(settings.fill_color);

#if defined(HAVE_ALPHA_BLEND) || !defined(HAVE_HATCHED_BRUSH)
  solid_brush.Create(fill_color);
#endif

  if (settings.border_width != 0)
    border_pen.Create(Layout::ScalePenWidth(settings.border_width),
                      Color(settings.border_color));
}

void
AirspaceLook::Reinitialise(const AirspaceRendererSettings &settings)
{
  for (unsigned i = 0; i < AIRSPACECLASSCOUNT; ++i)
    classes[i].Initialise(settings.classes[i]);
}

void
AirspaceLook::Initialise(const AirspaceRendererSettings &settings,
                         const Font &_name_font)
{
  Reinitialise(settings);

  // airspace brushes and colors
#ifdef HAVE_HATCHED_BRUSH
  bitmaps[0].Load(IDB_AIRSPACE0);
  bitmaps[1].Load(IDB_AIRSPACE1);
  bitmaps[2].Load(IDB_AIRSPACE2);
  bitmaps[3].Load(IDB_AIRSPACE3);
  bitmaps[4].Load(IDB_AIRSPACE4);
  bitmaps[5].Load(IDB_AIRSPACE5);
  bitmaps[6].Load(IDB_AIRSPACE6);
  bitmaps[7].Load(IDB_AIRSPACE7);

  for (unsigned i = 0; i < ARRAY_SIZE(AirspaceLook::brushes); i++)
    brushes[i].Create(bitmaps[i]);
#endif

  thick_pen.Create(Layout::ScalePenWidth(10), COLOR_BLACK);

  intercept_icon.LoadResource(IDB_AIRSPACEI_ALL);

  // labels
  label_pen.Create(1, COLOR_BLUE);
  label_brush.Create(COLOR_WHITE);
  label_text_color = COLOR_BLUE;

  name_font = &_name_font;
}
