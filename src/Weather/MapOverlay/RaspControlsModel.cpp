// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspControlsModel.hpp"

#include "ActionInterface.hpp"
#include "Interface.hpp"
#include "PageActions.hpp"
#include "UIState.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Form/DataField/Enum.hpp"

namespace MapOverlay {

void
RaspControlsModel::SyncFromPageLayout() noexcept
{
  const auto &layout = PageActions::GetCurrentLayout();
  if (layout.overlay != PageLayout::Overlay::RASP)
    return;

  CommonInterface::SetUIState().weather.map = GetFieldIndex();
}

int
RaspControlsModel::GetFieldIndex() const noexcept
{
  const auto &layout = PageActions::GetCurrentLayout();
  if (layout.overlay != PageLayout::Overlay::RASP)
    return -1;

  if (layout.rasp_field >= 0)
    return layout.rasp_field;

  return 0;
}

void
RaspControlsModel::FillTimeChoices(DataFieldEnum &field,
                                   const std::shared_ptr<RaspStore> &rasp) const noexcept
{
  const int map = GetFieldIndex();
  if (map < 0)
    return;

  Rasp::FillTimeChoices(field, rasp.get(), unsigned(map),
                        CommonInterface::GetUIState().weather.time);
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

} // namespace MapOverlay
