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

#ifndef XCSOAR_INFOBOX_CONTENT_TASK_HPP
#define XCSOAR_INFOBOX_CONTENT_TASK_HPP

#include "InfoBoxes/Content/Base.hpp"

extern const InfoBoxPanel next_waypoint_infobox_panels[];

void
UpdateInfoBoxBearing(InfoBoxData &data);

void
UpdateInfoBoxBearingDiff(InfoBoxData &data);

void
UpdateInfoBoxRadial(InfoBoxData &data);

class InfoBoxContentNextWaypoint : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) override;
  virtual const InfoBoxPanel *GetDialogContent() override;
};

void
UpdateInfoBoxNextDistance(InfoBoxData &data);

void
UpdateInfoBoxNextDistanceNominal(InfoBoxData &data);

void
UpdateInfoBoxNextETE(InfoBoxData &data);

void
UpdateInfoBoxNextETA(InfoBoxData &data);

void
UpdateInfoBoxNextAltitudeDiff(InfoBoxData &data);

void
UpdateInfoBoxNextMC0AltitudeDiff(InfoBoxData &data);

void
UpdateInfoBoxNextAltitudeRequire(InfoBoxData &data);

void
UpdateInfoBoxNextAltitudeArrival(InfoBoxData &data);

void
UpdateInfoBoxNextGR(InfoBoxData &data);

void
UpdateInfoBoxFinalDistance(InfoBoxData &data);

void
UpdateInfoBoxFinalETE(InfoBoxData &data);

void
UpdateInfoBoxFinalETA(InfoBoxData &data);

void
UpdateInfoBoxFinalAltitudeDiff(InfoBoxData &data);

void
UpdateInfoBoxFinalMC0AltitudeDiff(InfoBoxData &data);

void
UpdateInfoBoxFinalAltitudeRequire(InfoBoxData &data);

void
UpdateInfoBoxFinalGR(InfoBoxData &data);

void
UpdateInfoBoxTaskSpeed(InfoBoxData &data);

void
UpdateInfoBoxTaskSpeedAchieved(InfoBoxData &data);

void
UpdateInfoBoxTaskSpeedInstant(InfoBoxData &data);

void
UpdateInfoBoxTaskSpeedHour(InfoBoxData &data);

void
UpdateInfoBoxTaskAATime(InfoBoxData &data);

void
UpdateInfoBoxTaskAATimeDelta(InfoBoxData &data);

void
UpdateInfoBoxTaskAADistance(InfoBoxData &data);

void
UpdateInfoBoxTaskAADistanceMax(InfoBoxData &data);

void
UpdateInfoBoxTaskAADistanceMin(InfoBoxData &data);

void
UpdateInfoBoxTaskAASpeed(InfoBoxData &data);

void
UpdateInfoBoxTaskAASpeedMax(InfoBoxData &data);

void
UpdateInfoBoxTaskAASpeedMin(InfoBoxData &data);

void
UpdateInfoBoxTaskTimeUnderMaxHeight(InfoBoxData &data);

void
UpdateInfoBoxNextETEVMG(InfoBoxData &data);

void
UpdateInfoBoxNextETAVMG(InfoBoxData &data);

void
UpdateInfoBoxFinalETEVMG(InfoBoxData &data);

void
UpdateInfoBoxCruiseEfficiency(InfoBoxData &data);

void
UpdateInfoBoxStartOpen(InfoBoxData &data);

void
UpdateInfoBoxStartOpenArrival(InfoBoxData &data);

class InfoBoxContentNextArrow: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) override;
  virtual void OnCustomPaint(Canvas &canvas, const PixelRect &rc) override;
};

#endif
