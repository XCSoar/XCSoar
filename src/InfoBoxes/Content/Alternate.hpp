// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"

/**
 * Base class for alternate-related InfoBox content that opens the
 * alternates list dialog on click.
 */
class InfoBoxContentAlternateBase : public InfoBoxContent
{
protected:
  unsigned index;

public:
  explicit InfoBoxContentAlternateBase(unsigned _index) noexcept
    :index(_index) {}

  bool HandleClick() noexcept override;
};

class InfoBoxContentAlternateName : public InfoBoxContentAlternateBase
{
public:
  using InfoBoxContentAlternateBase::InfoBoxContentAlternateBase;

  void Update(InfoBoxData &data) noexcept override;
};

class InfoBoxContentAlternateGR : public InfoBoxContentAlternateBase
{
public:
  using InfoBoxContentAlternateBase::InfoBoxContentAlternateBase;

  void Update(InfoBoxData &data) noexcept override;
};

class InfoBoxContentAlternateAltDiff : public InfoBoxContentAlternateBase
{
public:
  using InfoBoxContentAlternateBase::InfoBoxContentAlternateBase;

  void Update(InfoBoxData &data) noexcept override;
};
