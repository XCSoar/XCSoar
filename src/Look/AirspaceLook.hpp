// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Icon.hpp"
#include "Engine/Airspace/AirspaceClass.hpp"

static constexpr unsigned NUMAIRSPACECOLORS = 18;

struct AirspaceRendererSettings;
struct AirspaceClassRendererSettings;
class Font;

struct AirspaceClassLook {
  Color fill_color;

  /**
   * Solid fill used for transparent airspace rendering.
   */
  Brush solid_brush;

  Pen border_pen;

  void Initialise(const AirspaceClassRendererSettings &settings);
};

struct AirspaceLook {
  static const RGB8Color preset_colors[NUMAIRSPACECOLORS];

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
