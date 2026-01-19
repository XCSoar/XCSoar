// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"

class InfoBoxContentContest: public InfoBoxContent
{
public:
  bool HandleClick() noexcept override;
  void Update(InfoBoxData &data) noexcept override;
};

class InfoBoxContentContestSpeed: public InfoBoxContent
{
public:
  bool HandleClick() noexcept override;
  void Update(InfoBoxData &data) noexcept override;
};
