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

#include "ConditionMonitors.hpp"
#include "ConditionMonitor.hpp"
#include "ConditionMonitorAATTime.hpp"
#include "ConditionMonitorFinalGlide.hpp"
#include "ConditionMonitorGlideTerrain.hpp"
#include "ConditionMonitorLandableReachable.hpp"
#include "ConditionMonitorStartRules.hpp"
#include "ConditionMonitorSunset.hpp"
#include "ConditionMonitorWind.hpp"
#include "Message.hpp"
#include "Device/device.hpp"
#include "Protection.hpp"
#include "Math/SunEphemeris.hpp"
#include "LocalTime.hpp"
#include "InputEvents.hpp"
#include "Computer/GlideComputer.hpp"
#include "Language/Language.hpp"
#include "Units/Units.hpp"

#include <math.h>

ConditionMonitorWind cm_wind;
ConditionMonitorFinalGlide cm_finalglide;
ConditionMonitorSunset cm_sunset;
ConditionMonitorAATTime cm_aattime;
ConditionMonitorStartRules cm_startrules;
ConditionMonitorGlideTerrain cm_glideterrain;
ConditionMonitorLandableReachable cm_landablereachable;
void
ConditionMonitorsUpdate(const GlideComputer& cmp)
{
  cm_wind.Update(cmp);
  cm_finalglide.Update(cmp);
  cm_sunset.Update(cmp);
  cm_aattime.Update(cmp);
  cm_startrules.Update(cmp);
  cm_glideterrain.Update(cmp);
  cm_landablereachable.Update(cmp);
}
