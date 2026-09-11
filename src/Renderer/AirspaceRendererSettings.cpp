// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
#include "AirspaceRendererSettings.hpp"
#include "Asset.hpp"

void
AirspaceClassRendererSettings::SetDefaults()
{
  display = true;
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

  fill_mode = FillMode::DEFAULT;
  label_selection = LabelSelection::NONE;
  show_notam_labels = true;

  for (auto it = classes; it != classes + AIRSPACECLASSCOUNT; ++it)
    it->SetDefaults();

  classes[CLASSG].display = false;

  classes[OTHER].SetColors(RGB8_MAGENTA);
  classes[RESTRICTED].SetColors(RGB8_RED);
  classes[PROHIBITED].SetColors(RGB8_RED);
  classes[DANGER].SetColors(RGB8_RED);
  classes[CLASSA].SetColors(RGB8_RED);
  classes[CLASSB].SetColors(RGB8_BLUE);
  classes[CLASSC].SetColors(RGB8_BLUE);
  classes[CLASSD].SetColors(RGB8_BLUE);
  classes[NOGLIDER].SetColors(RGB8_RED);
  classes[CTR].SetColors(RGB8_BLUE);
  classes[WAVE].SetColors(RGB8_YELLOW.Darken());
  classes[AATASK].SetColors(HasColors() ? RGB8_YELLOW : RGB8_MAGENTA);
  classes[CLASSE].SetColors(RGB8_BLUE);
  classes[CLASSF].SetColors(RGB8_BLUE);
  classes[TMZ].SetColors(RGB8_STEEL_BLUE);
  classes[CLASSG].SetColors(RGB8_GRAYISH_VIOLET);
  classes[MATZ].SetColors(RGB8_GRAYISH_VIOLET);
  classes[RMZ].SetColors(RGB8_STEEL_BLUE);
  classes[UNCLASSIFIED].SetColors(RGB8_MAGENTA.Darken());
  classes[TMA].SetColors(RGB8_BLUE);
  classes[TRA].SetColors(RGB8_RED);
  classes[TSA].SetColors(RGB8_RED);
  classes[FIR].SetColors(RGB8_GRAYISH_VIOLET);
  classes[UIR].SetColors(RGB8_GRAYISH_VIOLET);
  classes[ADIZ].SetColors(RGB8_RED);
  classes[ATZ].SetColors(RGB8_GRAYISH_VIOLET);
  classes[AWY].SetColors(RGB8_BLUE);
  classes[MTR].SetColors(RGB8_LIGHT_GRAY);
  classes[ALERT].SetColors(RGB8_DARK_GRAY);
  classes[WARNING].SetColors(RGB8_DARK_GRAY);
  classes[PROTECTED].SetColors(RGB8_GREEN.Darken());
  classes[HTZ].SetColors(RGB8_GREEN.Darken());
  classes[GLIDING_SECTOR].SetColors(RGB8_GREEN.Darken());
  classes[TRP].SetColors(RGB8_RED);
  classes[TIZ].SetColors(RGB8_DARK_GRAY);
  classes[TIA].SetColors(RGB8_DARK_GRAY);
  classes[MTA].SetColors(RGB8_DARK_GRAY);
  classes[CTA].SetColors(RGB8_GRAYISH_VIOLET);
  classes[ACC_SECTOR].SetColors(RGB8_GRAYISH_VIOLET);
  classes[AERIAL_SPORTING_RECREATIONAL].SetColors(RGB8_RED.Darken());
  classes[OVERFLIGHT_RESTRICTION].SetColors(RGB8_RED);
  classes[MRT].SetColors(RGB8_RED);
  classes[TFR].SetColors(RGB8_RED);
  classes[VFR_SECTOR].SetColors(RGB8_BLUE);
  classes[FIS_SECTOR].SetColors(RGB8_BLUE);
  classes[LTA].SetColors(RGB8_BLUE);
  classes[UTA].SetColors(RGB8_BLUE);
  classes[NOTAM].SetColors(RGB8_GRAYISH_VIOLET);
}
