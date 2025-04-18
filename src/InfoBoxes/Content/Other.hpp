// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"

void
UpdateInfoBoxHeartRate(InfoBoxData &data) noexcept;

void
UpdateInfoBoxGLoad(InfoBoxData &data) noexcept;

void
UpdateInfoBoxBattery(InfoBoxData &data) noexcept;

void
UpdateInfoBoxExperimental1(InfoBoxData &data) noexcept;

void
UpdateInfoBoxExperimental2(InfoBoxData &data) noexcept;

void
UpdateInfoBoxCPULoad(InfoBoxData &data) noexcept;

void
UpdateInfoBoxFreeRAM(InfoBoxData &data) noexcept;

void
UpdateInfoBoxNbrSat(InfoBoxData &data) noexcept;

class InfoBoxContentHorizon : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept override;
};
