// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"

void
UpdateInfoBoxHeartRate(InfoBoxData &data) noexcept;

void
UpdateInfoBoxGLoad(InfoBoxData &data) noexcept;

void
UpdateInfoBoxBattery(InfoBoxData &data) noexcept;

void
UpdateInfoBoxExperimental1(InfoBoxData &data) noexcept;

void
UpdateInfoBoxExperimental2(InfoBoxData &data) noexcept;

void
UpdateInfoBoxCPULoad(InfoBoxData &data) noexcept;

void
UpdateInfoBoxFreeRAM(InfoBoxData &data) noexcept;

void
UpdateInfoBoxNbrSat(InfoBoxData &data) noexcept;

/**
 * The layout placeholders: InfoBoxes which are not displayed and only
 * give their space to other InfoBoxes; see
 * InfoBoxLayout::ApplyContents().
 */
void
UpdateInfoBoxPlaceholder(InfoBoxData &data) noexcept;

/**
 * The "invisible" InfoBox: it has no title, no value and no comment.
 * Its window is never shown; the map is extended over the slot
 * instead (see #InfoBoxManager::ExpandOverInvisible()).
 */
void
UpdateInfoBoxInvisible(InfoBoxData &data) noexcept;

class InfoBoxContentNbrSat final : public InfoBoxContent {
public:
  void Update(InfoBoxData &data) noexcept override;
  bool HandleClick() noexcept override;
};

/**
 * Shows the free text configured for this InfoBox slot; see
 * #InfoBoxSettings::Panel::text.
 */
class InfoBoxContentCustomText final : public InfoBoxContent {
public:
  void Update(InfoBoxData &data) noexcept override;

  [[gnu::pure]]
  const InfoBoxPanel *GetDialogContent() noexcept override;
};

class InfoBoxContentHorizon : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept override;
};
