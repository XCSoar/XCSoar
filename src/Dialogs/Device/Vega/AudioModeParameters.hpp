// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VegaParametersWidget.hpp"
#include "Schemes.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"

static constexpr StaticEnumChoice tone_climb_comparisons[] = {
  { 0, N_("None") },
  { 1, "Gross>MacCready" },
  { 2, "Gross>Average" },
  { 0 }
};

static constexpr StaticEnumChoice tone_cruise_lift_detection_types[] = {
  { 0, N_("Disabled") },
  { 1, "Relative>0" },
  { 2, "Relative>MacCready/2" },
  { 3, "Gross>0" },
  { 4, "Net>MacCready/2" },
  { 5, "Relative>MacCready" },
  { 6, "Net>MacCready" },
  { 0 }
};

static constexpr StaticEnumChoice time_scales[] = {
  { 0, " 0.0s" },
  { 1, " 0.8s" },
  { 2, " 1.7s" },
  { 3, " 3.5s" },
  { 4, " 7.5s" },
  { 5, "15.0s" },
  { 6, "30.0s" },
  { 0 }
};

static constexpr StaticEnumChoice filter_time[] = {
  { 0, " 1.0s" },
  { 1, " 1.3s" },
  { 2, " 1.8s" },
  { 3, " 2.7s" },
  { 0 }
};

static constexpr
VegaParametersWidget::StaticParameter audio_mode_parameters[] = {
  { DataField::Type::ENUM, "ToneClimbComparisonType",
    N_("Climb fast trigger"),
    N_("Comparison method used to detect climb states (HIGH/LOW).\n[NONE]: State LOW disabled\n[MACCREADY]: State High if gross vario is greater than MacCready setting.\n[AVERAGE]: State HIGH if gross vario value is greater than average gross vario value."),
    tone_climb_comparisons,
  },
  { DataField::Type::ENUM, "ToneCruiseLiftDetectionType",
    N_("Cruise lift trigger"),
    N_("Comparison method used to detect cruise states for switching to lift tones during cruise.\n[NONE]: LIFT tone disabled in cruise\n[RELATIVE ZERO] LIFT tone when relative vario greater than zero.\n[GROSS ZERO] LIFT tone when glider is climbing.\n[RELATIVE MC/2] LIFT tone when relative vario greater than half MC.\n[NET MC/2] LIFT tone when airmass velocity greater than half MC."),
    tone_cruise_lift_detection_types,
  },
  { DataField::Type::ENUM, "ToneAveragerVarioTimeScale",
    N_("Climb averager scale"), N_("Time scale used for vario averager."),
    time_scales,
  },
  { DataField::Type::ENUM, "ToneAveragerCruiseTimeScale",
    N_("Cruise averager scale"),
    N_("Time scale used for cruise speed command averager."),
    time_scales,
  },
  { DataField::Type::INTEGER, "ToneMeanVolumeCircling",
    N_("Circling volume"),
    N_("Mean volume level in circling modes."),
    NULL, 0, 8, 1, "%d/8",
  },
  { DataField::Type::INTEGER, "ToneMeanVolumeCruise",
    N_("Cruise volume"),
    N_("Mean volume level in cruise modes. If set to zero, scales with airspeed."),
    NULL, 0, 8, 1, "%d/8",
  },
  { DataField::Type::INTEGER, "ToneBaseFrequencyOffset",
    N_("Base frequency"),
    N_("Adjustment to base frequency of tones in all modes."),
    NULL, -30, 30, 1, "%d",
  },
  { DataField::Type::ENUM, "VarioTimeConstantCircling",
    N_("Filter circling"),
    N_("Variometer low pass filter time constant in circling mode."),
    filter_time,
  },
  { DataField::Type::ENUM, "VarioTimeConstantCruise",
    N_("Filter cruise"),
    N_("Variometer low pass filter time constant in cruise mode."),
    filter_time,
  },
  { DataField::Type::INTEGER, "TonePitchScale",
    N_("Tone pitch scale"),
    N_("Adjustment to base pitch scale of tones in all modes."),
    NULL, 32, 100, 1, "%d",
  },

  /* sentinel */
  { DataField::Type::BOOLEAN }
};

static void
LoadAudioModeScheme(VegaParametersWidget &widget, const VEGA_SCHEME &scheme)
{
  widget.LoadValueEnum(0, scheme.ToneClimbComparisonType);
  widget.LoadValueEnum(1, scheme.ToneLiftComparisonType);
}
