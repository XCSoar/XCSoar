// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VegaParametersWidget.hpp"
#include "Schemes.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"

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
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
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
