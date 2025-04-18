// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"

extern const InfoBoxPanel altitude_infobox_panels[];

class InfoBoxContentAltitude : public InfoBoxContent
{
public:
  const InfoBoxPanel *GetDialogContent() noexcept override;
};

void
UpdateInfoBoxAltitudeNav(InfoBoxData &data) noexcept;

class InfoBoxContentAltitudeGPS : public InfoBoxContentAltitude
{
public:
  void Update(InfoBoxData &data) noexcept override;
};

void
UpdateInfoBoxAltitudeAGL(InfoBoxData &data) noexcept;

void
UpdateInfoBoxAltitudeBaro(InfoBoxData &data) noexcept;

void
UpdateInfoBoxAltitudeQFE(InfoBoxData &data) noexcept;

void
UpdateInfoBoxAltitudeFlightLevel(InfoBoxData &data) noexcept;
