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

#ifndef XCSOAR_VEGA_AUDIO_MODE_PARAMETERS
#define XCSOAR_VEGA_AUDIO_MODE_PARAMETERS

#include "VegaParametersWidget.hpp"
#include "Schemes.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"

static constexpr StaticEnumChoice tone_climb_comparisons[] = {
  { 0, N_("None") },
  { 1, _T("Gross>MacCready") },
  { 2, _T("Gross>Average") },
  { 0 }
};

static constexpr StaticEnumChoice tone_cruise_lift_detection_types[] = {
  { 0, N_("Disabled") },
  { 1, _T("Relative>0") },
  { 2, _T("Relative>MacCready/2") },
  { 3, _T("Gross>0") },
  { 4, _T("Net>MacCready/2") },
  { 5, _T("Relative>MacCready") },
  { 6, _T("Net>MacCready") },
  { 0 }
};

static constexpr StaticEnumChoice time_scales[] = {
  { 0, _T(" 0.0s") },
  { 1, _T(" 0.8s") },
  { 2, _T(" 1.7s") },
  { 3, _T(" 3.5s") },
  { 4, _T(" 7.5s") },
  { 5, _T("15.0s") },
  { 6, _T("30.0s") },
  { 0 }
};

static constexpr StaticEnumChoice filter_time[] = {
  { 0, _T(" 1.0s") },
  { 1, _T(" 1.3s") },
  { 2, _T(" 1.8s") },
  { 3, _T(" 2.7s") },
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
    NULL, 0, 8, 1, _T("%d/8"),
  },
  { DataField::Type::INTEGER, "ToneMeanVolumeCruise",
    N_("Cruise volume"),
    N_("Mean volume level in cruise modes.  If set to zero, scales with airspeed."),
    NULL, 0, 8, 1, _T("%d/8"),
  },
  { DataField::Type::INTEGER, "ToneBaseFrequencyOffset",
    N_("Base frequency"),
    N_("Adjustment to base frequency of tones in all modes."),
    NULL, -30, 30, 1, _T("%d"),
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
    NULL, 32, 100, 1, _T("%d"),
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

#endif
