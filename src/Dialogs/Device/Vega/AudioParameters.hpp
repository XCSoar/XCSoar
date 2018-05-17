/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_VEGA_AUDIO_PARAMETERS
#define XCSOAR_VEGA_AUDIO_PARAMETERS

#include "VegaParametersWidget.hpp"
#include "Schemes.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Util/StaticString.hxx"

#include <stdio.h>

static constexpr StaticEnumChoice beep_types[] = {
  { 0, _T("Silence") },
  { 1, _T("Short") },
  { 2, _T("Medium") },
  { 3, _T("Long") },
  { 4, _T("Continuous") },
  { 5, _T("Short double") },
  { 0 }
};

static constexpr StaticEnumChoice pitch_schemes[] = {
  { 0, _T("Constant high") },
  { 1, _T("Constant medium") },
  { 2, _T("Constant low") },
  { 3, _T("Speed percent") },
  { 4, _T("Speed error") },
  { 5, _T("Vario gross") },
  { 6, _T("Vario net") },
  { 7, _T("Vario relative") },
  { 8, _T("Vario gross/relative") },
  { 0 }
};

static constexpr StaticEnumChoice period_schemes[] = {
  { 0, _T("Constant high") },
  { 1, _T("Constant medium") },
  { 2, _T("Constant low") },
  { 3, _T("Speed percent") },
  { 4, _T("Speed error") },
  { 5, _T("Vario gross") },
  { 6, _T("Vario net") },
  { 7, _T("Vario relative") },
  { 8, _T("Vario gross/relative") },
  { 9, _T("Intermittent") },
  { 0 }
};

static constexpr StaticEnumChoice pitch_and_period_scales[] = {
  { 0, _T("+Linear") },
  { 1, _T("+Low end") },
  { 2, _T("+High end") },
  { 3, _T("-Linear") },
  { 4, _T("-Low end") },
  { 5, _T("-High end") },
  { 0 }
};

class VegaAudioParametersWidget : public VegaParametersWidget {
  const char *mode;

  char names[5][64];

public:
  VegaAudioParametersWidget(const DialogLook &look, VegaDevice &device,
                            const char *_mode)
    :VegaParametersWidget(look, device), mode(_mode) {}

  void LoadScheme(const VEGA_SCHEME::Audio &scheme) {
    LoadValueEnum(0, scheme.beep_type);
    LoadValueEnum(1, scheme.pitch_scheme);
    LoadValueEnum(2, scheme.pitch_scale);
    LoadValueEnum(3, scheme.period_scheme);
    LoadValueEnum(4, scheme.period_scale);
  }

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) {
    VegaParametersWidget::Prepare(parent, rc);

    sprintf(names[0], "Tone%sBeepType", mode);
    AddEnum(names[0], _("Beep type"), NULL, beep_types);

    sprintf(names[1], "Tone%sPitchScheme", mode);
    AddEnum(names[1], _("Pitch scheme"), NULL, pitch_schemes);

    sprintf(names[2], "Tone%sPitchScale", mode);
    AddEnum(names[2], _("Pitch scale"), NULL, pitch_and_period_scales);

    sprintf(names[3], "Tone%sPeriodScheme", mode);
    AddEnum(names[3], _("Period scheme"), NULL, period_schemes);

    sprintf(names[4], "Tone%sPeriodScale", mode);
    AddEnum(names[4], _("Period scale"), NULL, pitch_and_period_scales);
  }
};

#endif
