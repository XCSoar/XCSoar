// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VegaParametersWidget.hpp"
#include "Schemes.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"

#include <array>

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

  std::array<StaticString<64>, 5> names;

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

    const char *safe_mode = (mode != nullptr && mode[0] != '\0')
      ? mode
      : "DefaultMode";

    names[0].Format("Tone%sBeepType", safe_mode);
    AddEnum(names[0].c_str(), _("Beep type"), NULL, beep_types);

    names[1].Format("Tone%sPitchScheme", safe_mode);
    AddEnum(names[1].c_str(), _("Pitch scheme"), NULL, pitch_schemes);

    names[2].Format("Tone%sPitchScale", safe_mode);
    AddEnum(names[2].c_str(), _("Pitch scale"), NULL, pitch_and_period_scales);

    names[3].Format("Tone%sPeriodScheme", safe_mode);
    AddEnum(names[3].c_str(), _("Period scheme"), NULL, period_schemes);

    names[4].Format("Tone%sPeriodScale", safe_mode);
    AddEnum(names[4].c_str(), _("Period scale"), NULL, pitch_and_period_scales);
  }
};
