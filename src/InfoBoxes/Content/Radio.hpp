// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"

class InfoBoxContentActiveRadioFrequency : public InfoBoxContent
{
public:
  const InfoBoxPanel *GetDialogContent() noexcept override;
  void Update(InfoBoxData &data) noexcept override;
};

class InfoBoxContentStandbyRadioFrequency : public InfoBoxContent
{
public:
  const InfoBoxPanel *GetDialogContent() noexcept override;
  void Update(InfoBoxData &data) noexcept override;
};

class InfoBoxContentTransponderCode : public InfoBoxContent
{
public:
  const InfoBoxPanel *GetDialogContent() noexcept override;
  void Update(InfoBoxData &data) noexcept override;
};
