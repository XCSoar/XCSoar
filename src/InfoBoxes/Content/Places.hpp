// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"
#include "Engine/Waypoint/Ptr.hpp"

struct InfoBoxData;

void
UpdateInfoBoxHomeDistance(InfoBoxData &data) noexcept;

void
UpdateInfoBoxHomeAltitudeDiff(InfoBoxData &data) noexcept;

class InfoBoxContentHome : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleClick() noexcept override;
};

class InfoBoxContentActiveWaypoint : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleClick() noexcept override;
};

class InfoBoxContentPreviousWaypoint : public InfoBoxContent
{
  /* User override. nullptr = "auto": track the task waypoint before
     the active leg (or task[0] when on the first leg). */
  WaypointPtr override_waypoint;

public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleClick() noexcept override;
};

void
UpdateInfoBoxTakeoffDistance(InfoBoxData &data) noexcept;

extern const struct InfoBoxPanel atc_infobox_panels[];

void
UpdateInfoBoxATCRadial(InfoBoxData &data) noexcept;
