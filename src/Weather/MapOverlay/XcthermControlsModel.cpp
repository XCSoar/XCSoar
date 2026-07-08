// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XcthermControlsModel.hpp"

#include "ActionInterface.hpp"
#include "Language/Language.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"

namespace WeatherMapOverlay {

void
XcthermControlsModel::FormatPrimaryLabel(StaticString<64> &text) const noexcept
{
  text = "XCTherm";
}

void
XcthermControlsModel::FormatSecondaryLabel(StaticString<64> &text) const noexcept
{
  text = NoForecastHint();
}

bool
XcthermControlsModel::HasPrimaryData() const noexcept
{
  return false;
}

bool
XcthermControlsModel::HasSecondaryData() const noexcept
{
  return false;
}

bool
XcthermControlsModel::StepPrimary(int delta) noexcept
{
  (void)delta;
  return false;
}

bool
XcthermControlsModel::StepSecondary(int delta) noexcept
{
  (void)delta;
  return false;
}

bool
XcthermControlsModel::GetPrimaryAutoAdvance() const noexcept
{
  return true;
}

void
XcthermControlsModel::SetPrimaryAutoAdvance(bool auto_advance) noexcept
{
  (void)auto_advance;
}

void
XcthermControlsModel::ApplyPrimaryAutoAdvance() noexcept
{
}

void
XcthermControlsModel::RefreshOverlay() noexcept
{
  ActionInterface::SendUIState(true);
}

} // namespace WeatherMapOverlay
