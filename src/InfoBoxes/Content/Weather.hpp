// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"

void
UpdateInfoBoxHumidity(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTemperature(InfoBoxData &data) noexcept;

class InfoBoxContentTemperatureForecast : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleKey(const InfoBoxKeyCodes keycode) noexcept override;
};

class InfoBoxContentWindSpeed: public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleClick() noexcept override;
};

class InfoBoxContentWindBearing: public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleClick() noexcept override;
};

class InfoBoxContentHeadWind: public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleClick() noexcept override;
};

class InfoBoxContentHeadWindSimplified: public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleClick() noexcept override;
};

class InfoBoxContentWindArrow: public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept override;
  bool HandleClick() noexcept override;
};
