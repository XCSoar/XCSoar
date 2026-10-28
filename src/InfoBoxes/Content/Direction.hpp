// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"

class InfoBoxContentTrack : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;

  [[gnu::pure]]
  const InfoBoxPanel *GetDialogContent() noexcept override;
};
