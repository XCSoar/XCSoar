/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Formatter/Base.hpp"
#include "Math/FastMath.h"
#include "Atmosphere.h"
#include "Battery.h"
#include "Units.hpp"
#include "Interface.hpp"
#include <stdio.h>
#include "Components.hpp"
#include "Asset.hpp"

InfoBoxFormatter::InfoBoxFormatter(const TCHAR *theformat)
{
  _tcscpy(Format, theformat);
  Valid = true;
  Value = 0.0;
  Text[0] = 0;
  CommentText[0] = 0;
}

const TCHAR *
InfoBoxFormatter::Render(int *color)
{
  if (Valid) {
    _stprintf(Text, Format, Value);
    *color = 0;
  } else {
    RenderInvalid(color);
  }
  return Text;
}

const TCHAR *
InfoBoxFormatter::RenderTitle(int *color)
{
  if (Valid) {
    _stprintf(Text, Format, Value);
    *color = 0;
  } else {
    RenderInvalid(color);
  }
  return Text;
}

void
InfoBoxFormatter::RenderInvalid(int *color)
{
  _tcscpy(CommentText, _T(""));
  _tcscpy(Text, _T("---"));
  *color = -1;
}

// TODO enhancement: crop long text or provide alternate
// e.g. 10300 ft ==> 10,3
// e.g. "Ardlethan" => "Ardl."

void
InfoBoxFormatter::AssignValue(int i)
{
  switch (i) {
  case 0:
    Value = Units::ToUserUnit(Basic().GPSAltitude, Units::AltitudeUnit);
    break;

  case 1:
    Value = Units::ToUserUnit(Basic().AltitudeAGL,
                              Units::AltitudeUnit);
    Valid = Calculated().TerrainValid;
    break;

  case 2:
    Value = Units::ToUserUnit(Calculated().Average30s,
                              Units::VerticalSpeedUnit);
    break;

  case 3:
    Value = Calculated().task_stats.current_leg.
            solution_remaining.Vector.Bearing;
    Valid = Calculated().task_stats.current_leg.
            solution_remaining.Vector.Distance > 10.0;
    break;

  case 4:
    if (Calculated().LD == 999) {
      Valid = false;
    } else {
      Valid = true;
      Value = Calculated().LD;
    }
    break;

  case 5:
    if (Calculated().CruiseLD == 999) {
      Valid = false;
    } else {
      Valid = true;
      Value = Calculated().CruiseLD;
    }
    break;

  case 6:
    Value = Units::ToUserUnit(Basic().GroundSpeed,
                              Units::SpeedUnit);
    break;

  case 7:
    Value = Units::ToUserUnit(Calculated().LastThermalAverage,
                              Units::VerticalSpeedUnit);
    break;

  case 8:
    Value = Units::ToUserUnit(Calculated().LastThermalGain,
                              Units::VerticalSpeedUnit);
    break;

  case 10:
    Value = (int)Units::ToUserUnit(Calculated().common_stats.current_mc * 10,
                                   Units::VerticalSpeedUnit) / 10.0;
    break;

  case 11:
    /// @todo this produces 0 if task not started! (bug)
    Value = Units::ToUserUnit(Calculated().task_stats.current_leg.
                              solution_remaining.Vector.Distance,
                              Units::DistanceUnit);
    Valid = Calculated().task_stats.task_valid;
    break;

  case 12:
    Value = Units::ToUserUnit(Calculated().task_stats.current_leg.
                              solution_remaining.AltitudeDifference,
                              Units::AltitudeUnit);
    Valid = Calculated().task_stats.task_valid;
    break;

  case 13:
    Value = Units::ToUserUnit(Calculated().task_stats.current_leg.
                              solution_remaining.AltitudeRequired,
                              Units::AltitudeUnit);
    Valid = Calculated().task_stats.task_valid;
    break;

  case 14:
    Value = 0; // Next Waypoint Text
    break;

  case 15:
    Value = Units::ToUserUnit(Calculated().task_stats.total.
                              solution_remaining.AltitudeDifference,
                              Units::AltitudeUnit);
    Valid = Calculated().task_stats.task_valid;
    break;

  case 16:
    Value = Units::ToUserUnit(Calculated().task_stats.total.
                              solution_remaining.AltitudeRequired,
                              Units::AltitudeUnit);
    Valid = Calculated().task_stats.task_valid;
    break;

  case 17:
    Value = Units::ToUserUnit(Calculated().task_stats.total.
                              remaining.get_speed(),
                              Units::TaskSpeedUnit);
    Valid = Calculated().task_stats.task_valid;
    break;

  case 18:
    if (Calculated().common_stats.task_finished)
      Value = Units::ToUserUnit(Calculated().task_stats.current_leg.
                                solution_remaining.Vector.Distance,
                                Units::DistanceUnit);
    else
      Value = Units::ToUserUnit(Calculated().task_stats.total.
                                remaining.get_distance(),
                                Units::DistanceUnit);

    Valid = Calculated().task_stats.task_valid;
    break;

  case 19:
#ifdef OLD_TASK
    if (Calculated().LDFinish== 999) {
      Valid = false;
    } else {
      Valid = Calculated().task_stats.task_valid;
      if (Calculated().ValidFinish) {
        Value = 0;
      } else {
        Value = Calculated().LDFinish;
      }
    }
#endif
    break;

  case 20:
    Value = Units::ToUserUnit(Calculated().TerrainAlt, Units::AltitudeUnit);
    Valid = Calculated().TerrainValid;
    break;

  case 21:
    Value = Units::ToUserUnit(Calculated().AverageThermal,
                              Units::VerticalSpeedUnit);
    break;

  case 22:
    Value = Units::ToUserUnit(Calculated().ThermalGain, Units::AltitudeUnit);
    break;

  case 23:
    Value = Basic().TrackBearing;
    break;

  case 24:
    Value = Units::ToUserUnit(Basic().TotalEnergyVario,
                              Units::VerticalSpeedUnit);
    break;

  case 25:
    Value = Units::ToUserUnit(Basic().wind.norm,
                              Units::WindSpeedUnit);
    break;

  case 26:
    Value = Basic().wind.bearing;
    break;

  case 28:
    Value = Units::ToUserUnit(Calculated().task_stats.distance_max,
                              Units::DistanceUnit);
    Valid = Calculated().task_stats.task_valid;
    break;

  case 29:
    Value = Units::ToUserUnit(Calculated().task_stats.distance_min,
                              Units::DistanceUnit);
    Valid = Calculated().task_stats.task_valid;
    break;

  case 30:
    Value = Units::ToUserUnit(Calculated().common_stats.aat_speed_max,
                              Units::TaskSpeedUnit);
    Valid = Calculated().task_stats.task_valid &&
            positive(Calculated().common_stats.aat_speed_max);
    break;

  case 31:
    Value = Units::ToUserUnit(Calculated().common_stats.aat_speed_min,
                              Units::TaskSpeedUnit);
    Valid = Calculated().task_stats.task_valid &&
            positive(Calculated().common_stats.aat_speed_min);
    break;

  case 32:
    Valid = Basic().AirspeedAvailable;
    Value = Units::ToUserUnit(Basic().IndicatedAirspeed,
                              Units::SpeedUnit);
    break;

  case 33:
    Valid = Basic().BaroAltitudeAvailable;
    Value = Units::ToUserUnit(Basic().BaroAltitude, Units::AltitudeUnit);
    break;

  case 34:
    Value = Units::ToUserUnit(Calculated().common_stats.V_block,
                              Units::SpeedUnit);
    break;

  case 35:
    Value = Calculated().PercentCircling;
    break;

  case 37:
    Valid = Basic().acceleration.Available;
    Value = Basic().acceleration.Gload;
    break;

  case 38:
#ifdef OLD_TASK
    if (Calculated().LDNext== 999) {
      Valid = false;
    } else {
      Valid = Calculated().task_stats.task_valid;
      Value = Calculated().LDNext;
    }
#endif
    break;

  case 43:
    Value = Units::ToUserUnit(Calculated().V_stf, Units::SpeedUnit);
    break;

  case 44:
    Value = Units::ToUserUnit(Basic().NettoVario,
                              Units::VerticalSpeedUnit);
    break;

  case 48:
    Value = Basic().OutsideAirTemperature;
    break;

  case 49:
    Value = Basic().RelativeHumidity;
    break;

  case 50:
    Value = CuSonde::maxGroundTemperature;
    break;

  case 51:
    Value = Units::ToUserUnit(Calculated().task_stats.total.
                              planned.get_distance(),
                              Units::DistanceUnit);
    Valid = Calculated().task_stats.task_valid;
    break;

  case 52:
    Value = Units::ToUserUnit(Calculated().common_stats.aat_speed_remaining,
                              Units::TaskSpeedUnit);
    Valid = Calculated().task_stats.task_valid &&
            positive(Calculated().common_stats.aat_speed_remaining);
    break;

  case 53:
    if (Calculated().LDvario == 999) {
      Valid = false;
    } else {
      Valid = Basic().TotalEnergyVarioAvailable && Basic().AirspeedAvailable;
      Value = Calculated().LDvario;
    }
    break;

  case 54:
    Valid = Basic().AirspeedAvailable;
    Value = Units::ToUserUnit(Basic().TrueAirspeed,
                              Units::SpeedUnit);
    break;

  case 56: // team bearing
    Value = Calculated().TeammateBearing;
    Valid = true;

  case 58: // team range
    if (SettingsComputer().TeammateCodeValid) {
      Value = Units::ToUserUnit(Calculated().TeammateRange,
                                Units::DistanceUnit);
      if (Value > 100)
        _tcscpy(Format, _T("%.0lf"));
      else
        _tcscpy(Format, _T("%.1lf"));
      Valid = true;
    } else
      Valid = false;
    break;

  case 59:
    Value = Units::ToUserUnit(Calculated().task_stats.total.
                              remaining_effective.get_speed_incremental(),
                              Units::TaskSpeedUnit);
    Valid = Calculated().task_stats.task_valid;
    break;

  case 60:
    Value = Units::ToUserUnit(Calculated().common_stats.vector_home.Distance,
                              Units::DistanceUnit);
    Valid = true;
    break;

  case 61:
    Value = Units::ToUserUnit(Calculated().task_stats.total.
                              remaining_effective.get_speed(),
                              Units::TaskSpeedUnit);
    Valid = Calculated().task_stats.task_valid;
    break;

  case 63:
    if (Calculated().timeCircling > 0) {
      Value = Units::ToUserUnit(Calculated().TotalHeightClimb /
                                Calculated().timeCircling,
                                Units::VerticalSpeedUnit);
      Valid = true;
    } else {
      Value = 0.0;
      Valid = false;
    }
    break;

  case 64:
    Value = Units::ToUserUnit(Calculated().task_stats.total.vario.get_value(),
                              Units::VerticalSpeedUnit);
    Valid = Calculated().task_stats.task_valid;
    break;

  case 65: // battery voltage
#if !defined(WINDOWSPC) && !defined(HAVE_POSIX)
    if (!is_altair()) {
      Value = PDABatteryPercent;
      Valid = true;
    } else {
      Value = Basic().SupplyBatteryVoltage;
      if (Value > 0.0)
        Valid = true;
      else
        Valid = false;
    }
#else
    Value = 0.0;
    Valid = false;
#endif
    break;

  case 66: // GR Final
#ifdef OLD_TASK
    Valid = Calculated().task_stats.task_valid;
    if (Calculated().GRFinish== 999) {
      Valid = false;
    } else {
      Valid = Calculated().task_stats.task_valid;
      if (Calculated().ValidFinish) {
        Value = 0;
      } else {
        Value = Calculated().GRFinish;
        if (Value > 100)
          _tcscpy(Format, _T("%1.0f"));
        else
          _tcscpy(Format, _T("%1.1f"));
      }
    }
#endif      
    break;

  case 70:	// QFE
#ifdef OLD_TASK
    Value = Units::ToUserUnit(Basic().GPSAltitude - QFEAltitudeOffset,
                              Units::AltitudeUnit);
#else
    Value = Units::ToUserUnit(Basic().GPSAltitude, Units::AltitudeUnit);
#endif
    break;

  case 71:
    if (Calculated().AverageLD == 0) {
      Valid = false;
    } else {
      Valid = true;
      Value = Calculated().AverageLD;
      if (Value < 0)
        _tcscpy(Format, _T("^^^"));
      else if (Value >= 999)
        _tcscpy(Format, _T("+++"));
      else
        _tcscpy(Format, _T("%2.0f"));
    }
    break;

  case 72:
    Value = Calculated().Experimental;
    Valid = true;
    break;

  /* TODO feature: add extra infoboxes from Lars
  case 68: // distance flown
    if (Calculated().TaskDistanceCovered != 0) {
      Value = Units::ToUserUnit(Calculated().TaskDistanceCovered,
                                Units::DistanceUnit);
      Valid = true;
    } else {
      Value = 0.0;
      Valid = false;
    }
    break;

  case 67: // termik liga points
    if (Calculated().TermikLigaPoints != 0) {
      Value = Calculated().TermikLigaPoints;
      Valid = true;
    } else {
      Value = 0.0;
      Valid = false;
    }
    break;
    */

  default:
    break;
  };
}

const TCHAR *
InfoBoxFormatter::GetCommentText(void)
{
  return CommentText;
}

bool
InfoBoxFormatter::isValid(void)
{
  return Valid;
}
