// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"

struct InfoBoxData;

void
UpdateInfoBoxTimeLocal(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTimeUTC(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTimeFlight(InfoBoxData &data) noexcept;

class InfoBoxContentTimeFlight final : public InfoBoxContent {
public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleClick() noexcept override;
};

class InfoBoxContentTimeLocal final : public InfoBoxContent {
public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleClick() noexcept override;
};

class InfoBoxContentTimeUTC final : public InfoBoxContent {
public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleClick() noexcept override;
};
