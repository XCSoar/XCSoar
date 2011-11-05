/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Base.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Math/Angle.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Util/StringUtil.hpp"

#include <stdio.h>

static
void FillInfoBoxWaypointName(InfoBoxData &data, const Waypoint* way_point,
                             const bool title=true)
{
  TCHAR tmp[32];
  if (!way_point) {
    tmp[0] = '\0';
  } else {
    switch (XCSoarInterface::SettingsMap().waypoint.display_text_type) {
    case DISPLAYFIRSTTHREE:
      CopyString(tmp, way_point->name.c_str(), 4);
      break;

    case DISPLAYFIRSTFIVE:
      CopyString(tmp, way_point->name.c_str(), 6);
      break;

    default:
      CopyString(tmp, way_point->name.c_str(), sizeof(tmp) / sizeof(TCHAR));
      break;
    }
  }
  if (title) {
    data.SetTitle(tmp);
  } else {
    data.SetComment(tmp);
  }
}

void
InfoBoxContent::SetValueBearingDifference(InfoBoxData &data, Angle delta)
{
  fixed delta_degrees = delta.AsDelta().Degrees();
  TCHAR tmp[32];
  if (delta_degrees > fixed_one)
    _stprintf(tmp, _T("%2.0f°»"), (double)delta_degrees);
  else if (delta_degrees < fixed_minus_one)
    _stprintf(tmp, _T("«%2.0f°"), (double)-delta_degrees);
  else
    _tcscpy(tmp, _T("«»"));

  data.SetValue(tmp);
}

void
InfoBoxContent::SetCommentBearingDifference(InfoBoxData &data, Angle delta)
{
  fixed delta_degrees = delta.AsDelta().Degrees();
  TCHAR tmp[32];
  if (delta_degrees > fixed_one)
    _stprintf(tmp, _T("%2.0f°»"), (double)delta_degrees);
  else if (delta_degrees < fixed_minus_one)
    _stprintf(tmp, _T("«%2.0f°"), (double)-delta_degrees);
  else
    _tcscpy(tmp, _T("«»"));

  data.SetComment(tmp);
}

void
InfoBoxContent::SetTitleFromWaypointName(InfoBoxData &data,
                                         const Waypoint* waypoint)
{
  FillInfoBoxWaypointName(data, waypoint, true);
}

void
InfoBoxContent::SetCommentFromWaypointName(InfoBoxData &data,
                                           const Waypoint* waypoint)
{
  FillInfoBoxWaypointName(data, waypoint, false);
}

void
InfoBoxContent::SetValueFromFixed(InfoBoxData &data,
                                  const TCHAR* format, fixed value)
{
  TCHAR tmp[32];
  _stprintf(tmp, format, (double)value);
  data.SetValue(tmp);
}

void
InfoBoxContent::SetValueFromDistance(InfoBoxData &data, fixed distance)
{
  TCHAR tmp[32];
  Units::FormatUserDistance(distance, tmp, 32, false);
  data.SetValue(tmp);

  data.SetValueUnit(Units::current.distance_unit);
}
