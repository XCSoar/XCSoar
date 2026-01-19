// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"


bool
NextWaypointClick() noexcept;

void
UpdateInfoBoxBearing(InfoBoxData &data) noexcept;

class InfoBoxContentBearing : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxBearing(data);
  }
  bool HandleClick() noexcept override {
    return NextWaypointClick();
  }
};

void
UpdateInfoBoxBearingDiff(InfoBoxData &data) noexcept;

class InfoBoxContentBearingDiff : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxBearingDiff(data);
  }
  bool HandleClick() noexcept override {
    return NextWaypointClick();
  }
};

void
UpdateInfoBoxRadial(InfoBoxData &data) noexcept;

class InfoBoxContentRadial : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxRadial(data);
  }
  bool HandleClick() noexcept override {
    return NextWaypointClick();
  }
};

class InfoBoxContentNextWaypoint : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleClick() noexcept override {
    return NextWaypointClick();
  }
};

void
UpdateInfoBoxNextDistance(InfoBoxData &data) noexcept;

class InfoBoxContentNextDistance : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextDistance(data);
  }
  bool HandleClick() noexcept override {
    return NextWaypointClick();
  }
};

void
UpdateInfoBoxNextDistanceNominal(InfoBoxData &data) noexcept;

class InfoBoxContentNextDistanceNominal : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextDistanceNominal(data);
  }
  bool HandleClick() noexcept override {
    return NextWaypointClick();
  }
};

void
UpdateInfoBoxNextETE(InfoBoxData &data) noexcept;

class InfoBoxContentNextETE : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextETE(data);
  }
  bool HandleClick() noexcept override {
    return NextWaypointClick();
  }
};

void
UpdateInfoBoxNextETA(InfoBoxData &data) noexcept;

class InfoBoxContentNextETA : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextETA(data);
  }
  bool HandleClick() noexcept override {
    return NextWaypointClick();
  }
};

void
UpdateInfoBoxNextAltitudeDiff(InfoBoxData &data) noexcept;

class InfoBoxContentNextAltitudeDiff : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextAltitudeDiff(data);
  }
  bool HandleClick() noexcept override {
    return NextWaypointClick();
  }
};

void
UpdateInfoBoxNextMC0AltitudeDiff(InfoBoxData &data) noexcept;

class InfoBoxContentNextMC0AltitudeDiff : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextMC0AltitudeDiff(data);
  }
  bool HandleClick() noexcept override {
    return NextWaypointClick();
  }
};

void
UpdateInfoBoxNextAltitudeRequire(InfoBoxData &data) noexcept;

class InfoBoxContentNextAltitudeRequire : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextAltitudeRequire(data);
  }
  bool HandleClick() noexcept override {
    return NextWaypointClick();
  }
};

void
UpdateInfoBoxNextAltitudeArrival(InfoBoxData &data) noexcept;

class InfoBoxContentNextAltitudeArrival : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextAltitudeArrival(data);
  }
  bool HandleClick() noexcept override {
    return NextWaypointClick();
  }
};

void
UpdateInfoBoxNextGR(InfoBoxData &data) noexcept;

class InfoBoxContentNextGR : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextGR(data);
  }
  bool HandleClick() noexcept override {
    return NextWaypointClick();
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

class InfoBoxContentNextETEVMG : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextETEVMG(data);
  }
  bool HandleClick() noexcept override {
    return NextWaypointClick();
  }
};

void
UpdateInfoBoxNextETAVMG(InfoBoxData &data) noexcept;

class InfoBoxContentNextETAVMG : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override {
    UpdateInfoBoxNextETAVMG(data);
  }
  bool HandleClick() noexcept override {
    return NextWaypointClick();
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

class InfoBoxContentNextArrow: public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept override;
  bool HandleClick() noexcept override {
    return NextWaypointClick();
  }
};

void
UpdateInfoTaskETAorAATdT(InfoBoxData &data) noexcept;
