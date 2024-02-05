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
  classes[RESTRICT].brush = 3;
  classes[PROHIBITED].brush = 3;
  classes[DANGER].brush = 3;
  classes[CLASSA].brush = 3;
  classes[CLASSB].brush = 3;
  classes[CLASSC].brush = 3;
  classes[CLASSD].brush = 3;
  classes[NOGLIDER].brush = 3;
  classes[CTR].brush = 3;
  classes[WAVE].brush = 2;
  classes[AATASK].brush = 3;
  classes[CLASSE].brush = 3;
  classes[CLASSF].brush = 3;
  classes[TMZ].brush = 3;
  classes[CLASSG].brush = 3;
  classes[MATZ].brush = 3;
  classes[RMZ].brush = 3;
  classes[UNCLASSIFIED].brush = 3;
  classes[RESTRICTED].brush = 3;
  classes[TMA].brush = 3;
  classes[TRA].brush = 3;
  classes[TSA].brush = 3;
  classes[FIR].brush = 3;
  classes[UIR].brush = 3;
  classes[ADIZ].brush = 3;
  classes[ATZ].brush = 3;
  classes[AWY].brush = 3;
  classes[MTR].brush = 3;
  classes[ALERT].brush = 3;
  classes[WARNING].brush = 3;
  classes[PROTECTED].brush = 3;
  classes[HTZ].brush = 3;
  classes[GLIDING_SECTOR].brush = 3;
  classes[TRP].brush = 3;
  classes[TIZ].brush = 3;
  classes[TIA].brush = 3;
  classes[MTA].brush = 3;
  classes[CTA].brush = 3;
  classes[ACC_SECTOR].brush = 3;
  classes[AERIAL_SPORTING_RECREATIONAL].brush = 3;
  classes[OVERFLIGHT_RESTRICTION].brush = 3;
  classes[MRT].brush = 3;
  classes[TFR].brush = 3;
  classes[VFR_SECTOR].brush = 3;
  classes[AIRSPACECLASSCOUNT].brush = 3;
#endif

  classes[OTHER].SetColors(RGB8_CYAN);
  classes[RESTRICT].SetColors(RGB8_RED);
  classes[PROHIBITED].SetColors(RGB8_RED);
  classes[DANGER].SetColors(RGB8_MAGENTA.Darken());
  classes[CLASSA].SetColors(RGB8_MAGENTA.Darken());
  classes[CLASSB].SetColors(RGB8_MAGENTA.Darken());
  classes[CLASSC].SetColors(RGB8_MAGENTA.Darken());
  classes[CLASSD].SetColors(RGB8_BLUE);
  classes[NOGLIDER].SetColors(RGB8_ORANGE);
  classes[CTR].SetColors(RGB8_MAGENTA.Darken());
  classes[WAVE].SetColors(RGB8_YELLOW.Darken());
  classes[AATASK].SetColors(HasColors() ? RGB8_YELLOW : RGB8_MAGENTA);
  classes[CLASSE].SetColors(RGB8_GREEN.Darken());
  classes[CLASSF].SetColors(RGB8_GREEN.Darken());
  classes[TMZ].SetColors(RGB8_MAGENTA.Darken());
  classes[CLASSG].SetColors(RGB8_MAGENTA.Darken());
  classes[MATZ].SetColors(RGB8_MAGENTA.Darken());
  classes[RMZ].SetColors(RGB8_MAGENTA.Darken());
  classes[UNCLASSIFIED].SetColors(RGB8_MAGENTA.Darken());
  classes[RESTRICTED].SetColors(RGB8_RED); // Assuming RGB8_RED is defined somewhere
  classes[TMA].SetColors(RGB8_MAGENTA.Darken());
  classes[TRA].SetColors(RGB8_MAGENTA.Darken());
  classes[TSA].SetColors(RGB8_MAGENTA.Darken());
  classes[FIR].SetColors(RGB8_MAGENTA.Darken());
  classes[UIR].SetColors(RGB8_MAGENTA.Darken());
  classes[ADIZ].SetColors(RGB8_MAGENTA.Darken());
  classes[ATZ].SetColors(RGB8_MAGENTA.Darken());
  classes[AWY].SetColors(RGB8_MAGENTA.Darken());
  classes[MTR].SetColors(RGB8_MAGENTA.Darken());
  classes[ALERT].SetColors(RGB8_MAGENTA.Darken());
  classes[WARNING].SetColors(RGB8_MAGENTA.Darken());
  classes[PROTECTED].SetColors(RGB8_MAGENTA.Darken());
  classes[HTZ].SetColors(RGB8_MAGENTA.Darken());
  classes[GLIDING_SECTOR].SetColors(RGB8_MAGENTA.Darken());
  classes[TRP].SetColors(RGB8_MAGENTA.Darken());
  classes[TIZ].SetColors(RGB8_MAGENTA.Darken());
  classes[TIA].SetColors(RGB8_MAGENTA.Darken());
  classes[MTA].SetColors(RGB8_MAGENTA.Darken());
  classes[CTA].SetColors(RGB8_MAGENTA.Darken());
  classes[ACC_SECTOR].SetColors(RGB8_MAGENTA.Darken());
  classes[AERIAL_SPORTING_RECREATIONAL].SetColors(RGB8_MAGENTA.Darken());
  classes[OVERFLIGHT_RESTRICTION].SetColors(RGB8_MAGENTA.Darken());
  classes[MRT].SetColors(RGB8_MAGENTA.Darken());
  classes[TFR].SetColors(RGB8_MAGENTA.Darken());
  classes[VFR_SECTOR].SetColors(RGB8_MAGENTA.Darken());
  classes[AIRSPACECLASSCOUNT].SetColors(RGB8_MAGENTA.Darken());
}
