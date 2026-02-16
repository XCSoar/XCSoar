// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"
#include "Engine/Waypoint/Ptr.hpp"

struct Waypoint;
struct GlideResult;

struct WaypointInfoWidget : public RowFormWidget {
  const WaypointPtr waypoint;

public:
  WaypointInfoWidget(const DialogLook &look, WaypointPtr _waypoint) noexcept
    :RowFormWidget(look), waypoint(std::move(_waypoint)) {}

  void AddGlideResult(const char *label, const GlideResult &result) noexcept;

  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
