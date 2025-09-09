// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"

class InfoBoxContentAlternateName : public InfoBoxContent
{
public:
  InfoBoxContentAlternateName(const unsigned _index) noexcept
    :index(_index) {}

  void Update(InfoBoxData &data) noexcept override;
  const InfoBoxPanel *GetDialogContent() noexcept override;

private:
  unsigned index;
};

class InfoBoxContentAlternateGR : public InfoBoxContent
{
public:
  InfoBoxContentAlternateGR(const unsigned _index) noexcept
    :index(_index) {}

  void Update(InfoBoxData &data) noexcept override;
  const InfoBoxPanel *GetDialogContent() noexcept override;

private:
  unsigned index;
};

class InfoBoxContentAlternateAltDiff : public InfoBoxContent
{
public:
  explicit InfoBoxContentAlternateAltDiff(const unsigned _index) noexcept
    :index(_index) {}

  void Update(InfoBoxData &data) noexcept override;
  const InfoBoxPanel *GetDialogContent() noexcept override;

private:
  unsigned index;
};
