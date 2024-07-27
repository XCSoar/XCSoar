// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Icon.hpp"
#include "ui/canvas/Features.hpp"
#include "Engine/Airspace/AirspaceClass.hpp"

static constexpr unsigned NUMAIRSPACECOLORS = 18;
static constexpr unsigned NUMAIRSPACEBRUSHES = 8;

struct AirspaceRendererSettings;
struct AirspaceClassRendererSettings;
class Font;

struct AirspaceClassLook {
  Color fill_color;

#if defined(HAVE_ALPHA_BLEND) || !defined(HAVE_HATCHED_BRUSH)
  /**
   * Non-pattern brushes used for transparent
   */
  Brush solid_brush;
#endif

  Pen border_pen;

  void Initialise(const AirspaceClassRendererSettings &settings);
};

struct AirspaceLook {
  static const RGB8Color preset_colors[NUMAIRSPACECOLORS];

#ifdef HAVE_HATCHED_BRUSH
  Bitmap bitmaps[NUMAIRSPACEBRUSHES];
  Brush brushes[NUMAIRSPACEBRUSHES];
#endif

  AirspaceClassLook classes[AIRSPACECLASSCOUNT];

  Pen thick_pen;

  MaskedIcon intercept_icon;

  /**
   * look for labels
   */
  Pen label_pen;
  Brush label_brush;
  Color label_text_color;

  /**
   * The font used to render the airspace name.
   */
  const Font *name_font;

  void Initialise(const AirspaceRendererSettings &settings,
                  const Font &_name_font);

  void Reinitialise(const AirspaceRendererSettings &settings);
};
