// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"

struct InfoBoxData;


class InfoBoxNearestAirspaceHorizontal : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleClick() noexcept override;
};

void
UpdateInfoBoxNearestAirspaceVertical(InfoBoxData &data) noexcept;
