// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"
#include "Gauge/ThermalAssistantRenderer.hpp"

void
UpdateInfoBoxVario(InfoBoxData &data) noexcept;

void
UpdateInfoBoxVarioNetto(InfoBoxData &data) noexcept;

void
UpdateInfoBoxThermal30s(InfoBoxData &data) noexcept;

void
UpdateInfoBoxThermalLastAvg(InfoBoxData &data) noexcept;

void
UpdateInfoBoxThermalLastGain(InfoBoxData &data) noexcept;

void
UpdateInfoBoxThermalLastTime(InfoBoxData &data) noexcept;

void
UpdateInfoBoxThermalAllAvg(InfoBoxData &data) noexcept;

void
UpdateInfoBoxThermalAvg(InfoBoxData &data) noexcept;

void
UpdateInfoBoxThermalGain(InfoBoxData &data) noexcept;

void
UpdateInfoBoxThermalTime(InfoBoxData &data) noexcept;

void
UpdateInfoBoxThermalRatio(InfoBoxData &data) noexcept;

void
UpdateInfoBoxNonCirclingClimbRatio(InfoBoxData &data) noexcept;

void
UpdateInfoBoxVarioDistance(InfoBoxData &data) noexcept;

void
UpdateInfoBoxNextLegEqThermal(InfoBoxData &data) noexcept;

void
UpdateInfoBoxCircleDiameter(InfoBoxData &data) noexcept;

class InfoBoxContentThermalAssistant: public InfoBoxContent
{
  ThermalAssistantRenderer renderer;

public:
  InfoBoxContentThermalAssistant() noexcept;

  void Update(InfoBoxData &data) noexcept override;
  void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept override;
};

class InfoBoxContentClimbPercent : public InfoBoxContent
{
 public:
  void Update(InfoBoxData &data) noexcept override;
  void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept override;
};
