// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
#include "AirspaceRendererSettings.hpp"
#include "Asset.hpp"

void
AirspaceClassRendererSettings::SetDefaults()
{
  display = true;
#ifdef HAVE_HATCHED_BRUSH
  brush = 0;
#endif
  border_color = RGB8_RED;
  fill_color = RGB8_RED;
  border_width = 2;
  fill_mode = FillMode::PADDING;
}

void
AirspaceRendererSettings::SetDefaults()
{
  enable = true;
  black_outline = false;
  altitude_mode = AirspaceDisplayMode::ALLON;
  clip_altitude = 1000;

#if defined(HAVE_HATCHED_BRUSH) && defined(HAVE_ALPHA_BLEND)
  transparency = false;
#endif

  fill_mode = FillMode::DEFAULT;
  label_selection = LabelSelection::NONE;

  for (auto it = classes; it != classes + AIRSPACECLASSCOUNT; ++it)
    it->SetDefaults();

  classes[CLASSG].display = false;

#ifdef HAVE_HATCHED_BRUSH
  classes[OTHER].brush = 2;
  classes[CLASSA].brush = 3;
  classes[CLASSB].brush = 3;
  classes[CLASSC].brush = 3;
  classes[CLASSD].brush = 3;
  classes[CTR].brush = 3;
  classes[WAVE].brush = 2;
  classes[AATASK].brush = 3;
  classes[CLASSE].brush = 3;
  classes[CLASSF].brush = 3;
  classes[RMZ].brush = 3;
#endif

  classes[OTHER].SetColors(RGB8_CYAN);
  classes[DANGER].SetColors(RGB8_MAGENTA.Darken());
  classes[MATZ].SetColors(RGB8_MAGENTA.Darken());
  classes[CLASSC].SetColors(RGB8_MAGENTA.Darken());
  classes[CLASSD].SetColors(RGB8_BLUE);
  classes[CTR].SetColors(RGB8_MAGENTA.Darken());
  classes[WAVE].SetColors(RGB8_YELLOW.Darken());
  classes[AATASK].SetColors(HasColors() ? RGB8_YELLOW : RGB8_MAGENTA);
  classes[CLASSE].SetColors(RGB8_GREEN.Darken());
  classes[CLASSF].SetColors(RGB8_GREEN.Darken());
  classes[RMZ].SetColors(RGB8_MAGENTA.Darken());
}
