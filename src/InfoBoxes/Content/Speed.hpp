// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"

class InfoBoxContentSpeedGround : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleKey(const InfoBoxKeyCodes keycode) noexcept override;
};

void
UpdateInfoBoxSpeedIndicated(InfoBoxData &data) noexcept;

void
UpdateInfoBoxSpeed(InfoBoxData &data) noexcept;

void
UpdateInfoBoxSpeedMacCready(InfoBoxData &data) noexcept;

void
UpdateInfoBoxSpeedDolphin(InfoBoxData &data) noexcept;
