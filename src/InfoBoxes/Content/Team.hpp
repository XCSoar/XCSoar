// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"

class InfoBoxContentTeamCode : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleKey(const InfoBoxKeyCodes keycode) noexcept override;
  const InfoBoxPanel *GetDialogContent() noexcept override;
};

void
UpdateInfoBoxTeamBearing(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTeamBearingDiff(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTeamDistance(InfoBoxData &data) noexcept;
