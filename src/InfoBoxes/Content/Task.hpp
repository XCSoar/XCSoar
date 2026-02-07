// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"


bool
NextWaypointClick() noexcept;

/**
 * Base class for InfoBox content that opens the next-waypoint
 * details dialog on click.
 */
class InfoBoxContentNextWaypointBase : public InfoBoxContent
{
public:
  bool HandleClick() noexcept override {
    return NextWaypointClick();
  }
};

void
UpdateInfoBoxBearing(InfoBoxData &data) noexcept;

class InfoBoxContentBearing : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxBearing(data);
  }
};

void
UpdateInfoBoxBearingDiff(InfoBoxData &data) noexcept;

class InfoBoxContentBearingDiff : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxBearingDiff(data);
  }
};

void
UpdateInfoBoxRadial(InfoBoxData &data) noexcept;

class InfoBoxContentRadial : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxRadial(data);
  }
};

class InfoBoxContentNextWaypoint : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override;
};

void
UpdateInfoBoxNextDistance(InfoBoxData &data) noexcept;

class InfoBoxContentNextDistance : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextDistance(data);
  }
};

void
UpdateInfoBoxNextDistanceNominal(InfoBoxData &data) noexcept;

class InfoBoxContentNextDistanceNominal
  : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextDistanceNominal(data);
  }
};

void
UpdateInfoBoxNextETE(InfoBoxData &data) noexcept;

class InfoBoxContentNextETE : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextETE(data);
  }
};

void
UpdateInfoBoxNextETA(InfoBoxData &data) noexcept;

class InfoBoxContentNextETA : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextETA(data);
  }
};

void
UpdateInfoBoxNextAltitudeDiff(InfoBoxData &data) noexcept;

class InfoBoxContentNextAltitudeDiff
  : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextAltitudeDiff(data);
  }
};

void
UpdateInfoBoxNextMC0AltitudeDiff(InfoBoxData &data) noexcept;

class InfoBoxContentNextMC0AltitudeDiff
  : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextMC0AltitudeDiff(data);
  }
};

void
UpdateInfoBoxNextAltitudeRequire(InfoBoxData &data) noexcept;

class InfoBoxContentNextAltitudeRequire
  : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextAltitudeRequire(data);
  }
};

void
UpdateInfoBoxNextAltitudeArrival(InfoBoxData &data) noexcept;

class InfoBoxContentNextAltitudeArrival
  : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextAltitudeArrival(data);
  }
};

void
UpdateInfoBoxNextGR(InfoBoxData &data) noexcept;

class InfoBoxContentNextGR : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextGR(data);
  }
};

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
UpdateInfoBoxTaskSpeedLeg(InfoBoxData &data) noexcept;

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

class InfoBoxContentNextETEVMG : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextETEVMG(data);
  }
};

void
UpdateInfoBoxNextETAVMG(InfoBoxData &data) noexcept;

class InfoBoxContentNextETAVMG : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextETAVMG(data);
  }
};

void
UpdateInfoBoxFinalETEVMG(InfoBoxData &data) noexcept;

void
UpdateInfoBoxCruiseEfficiency(InfoBoxData &data) noexcept;

void
UpdateInfoBoxStartOpen(InfoBoxData &data) noexcept;

void
UpdateInfoBoxStartOpenArrival(InfoBoxData &data) noexcept;

class InfoBoxContentNextArrow : public InfoBoxContentNextWaypointBase
{
public:
  void Update(InfoBoxData &data) noexcept override;
  void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept override;
};

void
UpdateInfoTaskETAorAATdT(InfoBoxData &data) noexcept;
