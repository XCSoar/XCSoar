// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"

extern const InfoBoxPanel next_waypoint_infobox_panels[];

void
UpdateInfoBoxBearing(InfoBoxData &data) noexcept;

void
UpdateInfoBoxBearingDiff(InfoBoxData &data) noexcept;

void
UpdateInfoBoxRadial(InfoBoxData &data) noexcept;

class InfoBoxContentNextWaypoint : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  const InfoBoxPanel *GetDialogContent() noexcept override;
};

void
UpdateInfoBoxNextDistance(InfoBoxData &data) noexcept;

void
UpdateInfoBoxNextDistanceNominal(InfoBoxData &data) noexcept;

void
UpdateInfoBoxNextETE(InfoBoxData &data) noexcept;

void
UpdateInfoBoxNextETA(InfoBoxData &data) noexcept;

void
UpdateInfoBoxNextAltitudeDiff(InfoBoxData &data) noexcept;

void
UpdateInfoBoxNextMC0AltitudeDiff(InfoBoxData &data) noexcept;

void
UpdateInfoBoxNextAltitudeRequire(InfoBoxData &data) noexcept;

void
UpdateInfoBoxNextAltitudeArrival(InfoBoxData &data) noexcept;

void
UpdateInfoBoxNextGR(InfoBoxData &data) noexcept;

void
UpdateInfoBoxFinalDistance(InfoBoxData &data) noexcept;

void
UpdateInfoBoxFinalETE(InfoBoxData &data) noexcept;

void
UpdateInfoBoxFinalETA(InfoBoxData &data) noexcept;

void
UpdateInfoBoxFinalAltitudeDiff(InfoBoxData &data) noexcept;

void
UpdateInfoBoxFinalMC0AltitudeDiff(InfoBoxData &data) noexcept;

void
UpdateInfoBoxFinalAltitudeRequire(InfoBoxData &data) noexcept;

void
UpdateInfoBoxFinalGR(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTaskSpeed(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTaskSpeedAchieved(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTaskSpeedInstant(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTaskSpeedHour(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTaskSpeedEst(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTaskAATime(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTaskAATimeDelta(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTaskAADistance(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTaskAADistanceMax(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTaskAADistanceMin(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTaskAASpeed(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTaskAASpeedMax(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTaskAASpeedMin(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTaskTimeUnderMaxHeight(InfoBoxData &data) noexcept;

void
UpdateInfoBoxNextETEVMG(InfoBoxData &data) noexcept;

void
UpdateInfoBoxNextETAVMG(InfoBoxData &data) noexcept;

void
UpdateInfoBoxFinalETEVMG(InfoBoxData &data) noexcept;

void
UpdateInfoBoxCruiseEfficiency(InfoBoxData &data) noexcept;

void
UpdateInfoBoxStartOpen(InfoBoxData &data) noexcept;

void
UpdateInfoBoxStartOpenArrival(InfoBoxData &data) noexcept;

class InfoBoxContentNextArrow: public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept override;
};

void
UpdateInfoTaskETAorAATdT(InfoBoxData &data) noexcept;
