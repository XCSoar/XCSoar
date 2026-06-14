// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspControlsModel.hpp"

#include "ActionInterface.hpp"
#include "DataGlobals.hpp"
#include "Interface.hpp"
#include "PageActions.hpp"
#include "UIState.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "Weather/Rasp/RaspStore.hpp"

namespace WeatherMapOverlay {

void
RaspControlsModel::OnShow() noexcept
{
  SyncFromPageLayout();
}

void
RaspControlsModel::SyncFromPageLayout() noexcept
{
  const auto &layout = PageActions::GetCurrentLayout();
  if (layout.overlay != PageLayout::Overlay::RASP)
    return;

  auto &weather = CommonInterface::SetUIState().weather;
  weather.map = -1;

  const int field_index = Rasp::GetActiveFieldIndex();
  if (field_index >= 0)
    weather.map = field_index;
}

void
RaspControlsModel::SetTime(unsigned minute_of_day) noexcept
{
  auto &weather = CommonInterface::SetUIState().weather;
  weather.time = Rasp::TimeFromMinuteOfDay(minute_of_day);
  weather.time_auto_advance = false;

  ActionInterface::SendUIState(true);
}

bool
RaspControlsModel::GetTimeAutoAdvance() const noexcept
{
  return CommonInterface::GetUIState().weather.time_auto_advance;
}

void
RaspControlsModel::SetTimeAutoAdvance(bool auto_advance) noexcept
{
  CommonInterface::SetUIState().weather.time_auto_advance = auto_advance;
}

void
RaspControlsModel::ApplyAutoAdvanceTime() noexcept
{
  CommonInterface::SetUIState().weather.time = BrokenTime::Invalid();
  ActionInterface::SendUIState(true);
}

void
RaspControlsModel::ResumeAutoAdvance() noexcept
{
  if (GetTimeAutoAdvance())
    return;

  SetTimeAutoAdvance(true);
  ApplyAutoAdvanceTime();
}

bool
RaspControlsModel::StepTime(int delta) noexcept
{
  const auto rasp = DataGlobals::GetRasp();
  const int field_index = Rasp::GetActiveFieldIndex();
  if (rasp == nullptr || field_index < 0)
    return false;

  const auto &weather = CommonInterface::GetUIState().weather;
  unsigned minute_of_day = 0;
  if (!Rasp::StepTime(rasp.get(), unsigned(field_index),
                      weather.time, weather.time_auto_advance,
                      delta, minute_of_day))
    return false;

  SetTime(minute_of_day);
  return true;
}

void
RaspControlsModel::FormatTimeLabel(StaticString<64> &text) const noexcept
{
  Rasp::FormatTimeCursorLabel(text, GetTimeAutoAdvance());
}

bool
RaspControlsModel::HasTimeData() const noexcept
{
  return Rasp::HasSelectedTimeData(GetTimeAutoAdvance());
}

} // namespace WeatherMapOverlay
