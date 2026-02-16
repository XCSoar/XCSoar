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
  { 0, "Silence" },
  { 1, "Short" },
  { 2, "Medium" },
  { 3, "Long" },
  { 4, "Continuous" },
  { 5, "Short double" },
  { 0 }
};

static constexpr StaticEnumChoice pitch_schemes[] = {
  { 0, "Constant high" },
  { 1, "Constant medium" },
  { 2, "Constant low" },
  { 3, "Speed percent" },
  { 4, "Speed error" },
  { 5, "Vario gross" },
  { 6, "Vario net" },
  { 7, "Vario relative" },
  { 8, "Vario gross/relative" },
  { 0 }
};

static constexpr StaticEnumChoice period_schemes[] = {
  { 0, "Constant high" },
  { 1, "Constant medium" },
  { 2, "Constant low" },
  { 3, "Speed percent" },
  { 4, "Speed error" },
  { 5, "Vario gross" },
  { 6, "Vario net" },
  { 7, "Vario relative" },
  { 8, "Vario gross/relative" },
  { 9, "Intermittent" },
  { 0 }
};

static constexpr StaticEnumChoice pitch_and_period_scales[] = {
  { 0, "+Linear" },
  { 1, "+Low end" },
  { 2, "+High end" },
  { 3, "-Linear" },
  { 4, "-Low end" },
  { 5, "-High end" },
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
