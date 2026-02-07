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

/**
 * Base class for wind-related InfoBox content that opens the
 * wind settings dialog on click.
 */
class InfoBoxContentWind : public InfoBoxContent
{
public:
  bool HandleClick() noexcept override;
};

class InfoBoxContentWindSpeed : public InfoBoxContentWind
{
public:
  void Update(InfoBoxData &data) noexcept override;
};

class InfoBoxContentWindBearing : public InfoBoxContentWind
{
public:
  void Update(InfoBoxData &data) noexcept override;
};

class InfoBoxContentHeadWind : public InfoBoxContentWind
{
public:
  void Update(InfoBoxData &data) noexcept override;
};

class InfoBoxContentHeadWindSimplified : public InfoBoxContentWind
{
public:
  void Update(InfoBoxData &data) noexcept override;
};

class InfoBoxContentWindArrow : public InfoBoxContentWind
{
public:
  void Update(InfoBoxData &data) noexcept override;
  void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept override;
};
