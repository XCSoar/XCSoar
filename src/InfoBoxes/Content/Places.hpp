// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"

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

void
UpdateInfoBoxTakeoffDistance(InfoBoxData &data) noexcept;

extern const struct InfoBoxPanel atc_infobox_panels[];

void
UpdateInfoBoxATCRadial(InfoBoxData &data) noexcept;
