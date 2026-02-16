// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VegaParametersWidget.hpp"
#include "Language/Language.hpp"

static constexpr
VegaParametersWidget::StaticParameter mixer_parameters[] = {
  { DataField::Type::BOOLEAN, "MuteVarioOnPlay", N_("Mute vario on voice") },
  { DataField::Type::BOOLEAN, "MuteVarioOnCom", N_("Mute vario on radio") },
  { DataField::Type::INTEGER, "VarioRelativeMuteVol", N_("Vario muting"),
    NULL, NULL, 0, 254, 1, "%d/255" },
  { DataField::Type::INTEGER, "VoiceRelativeMuteVol", N_("Voice muting"),
    NULL, NULL, 0, 254, 1, "%d/255" },
  { DataField::Type::INTEGER, "MuteComSpkThreshold", N_("Speaker threshold"),
    NULL, NULL, 0, 254, 1, "%d/255" },
  { DataField::Type::INTEGER, "MuteComPhnThreshold", N_("Headset threshold"),
    NULL, NULL, 0, 254, 1, "%d/255" },
  { DataField::Type::INTEGER, "MinUrgentVolume", N_("Urgent min. volume"),
    NULL, NULL, 0, 254, 1, "%d/255" },

  /* sentinel */
  { DataField::Type::BOOLEAN }
};
