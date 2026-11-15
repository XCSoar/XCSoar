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

/**
 * The heading, with a title that says where it comes from: "HDG"
 * from a compass, "HDG*" for the track corrected for the wind and
 * "TRK*" for the track alone.
 */
class InfoBoxContentHeading : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
};
