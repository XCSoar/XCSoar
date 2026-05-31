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
#include "Form/DataField/Enum.hpp"

namespace MapOverlay {

void
RaspControlsModel::SyncFromPageLayout() noexcept
{
  const auto &layout = PageActions::GetCurrentLayout();
  if (layout.overlay != PageLayout::Overlay::RASP)
    return;

  auto &weather = CommonInterface::SetUIState().weather;
  weather.map = -1;

  const int field_index = GetFieldIndex();
  if (field_index >= 0)
    weather.map = field_index;
}

int
RaspControlsModel::GetFieldIndex() const noexcept
{
  const auto &layout = PageActions::GetCurrentLayout();
  if (layout.overlay != PageLayout::Overlay::RASP)
    return -1;

  const auto rasp = DataGlobals::GetRasp();
  if (rasp == nullptr || rasp->GetItemCount() == 0)
    return -1;

  if (layout.rasp_field >= 0 &&
      unsigned(layout.rasp_field) < rasp->GetItemCount())
    return layout.rasp_field;

  return rasp->GetItemCount() > 0 ? 0 : -1;
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
