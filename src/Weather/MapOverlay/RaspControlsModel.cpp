// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspControlsModel.hpp"

#include "ActionInterface.hpp"
#include "Interface.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "UIState.hpp"

#ifdef HAVE_DOWNLOAD_MANAGER
#include "Weather/Rasp/DownloadGlue.hpp"
#endif

namespace WeatherMapOverlay {

void
RaspControlsModel::OnShow() noexcept
{
  Rasp::SyncCursorFromPageLayout();
  last_quarter = unsigned(-1);
}

void
RaspControlsModel::FormatPrimaryLabel(StaticString<64> &text) const noexcept
{
  Rasp::FormatTimeCursorLabel(text, GetPrimaryAutoAdvance());
}

void
RaspControlsModel::FormatSecondaryLabel(StaticString<64> &text) const noexcept
{
  Rasp::FormatFieldCursorLabel(text);
}

bool
RaspControlsModel::HasPrimaryData() const noexcept
{
  return Rasp::HasSelectedTimeData(GetPrimaryAutoAdvance());
}

bool
RaspControlsModel::HasSecondaryData() const noexcept
{
  return Rasp::HasSelectedField();
}

bool
RaspControlsModel::StepPrimary(int delta) noexcept
{
  return Rasp::StepCursorTime(delta);
}

bool
RaspControlsModel::StepSecondary(int delta) noexcept
{
  return Rasp::StepField(delta);
}

bool
RaspControlsModel::GetPrimaryAutoAdvance() const noexcept
{
  return Rasp::GetTimeAutoAdvance();
}

void
RaspControlsModel::SetPrimaryAutoAdvance(bool auto_advance) noexcept
{
  Rasp::SetTimeAutoAdvance(auto_advance);
}

void
RaspControlsModel::ApplyPrimaryAutoAdvance() noexcept
{
  Rasp::ApplyAutoAdvanceTime();
  last_quarter = unsigned(-1);
}

void
RaspControlsModel::EnablePrimaryAutoFromInput() noexcept
{
  Rasp::EnableTimeAutoFromInput();
  NotifyOverlay();
}

PrimaryLabelAction
RaspControlsModel::GetPrimaryLabelAction() const noexcept
{
  /* No layer yet: send the pilot to Weather Setup instead of a
     time picker that cannot do anything useful. */
  if (!Rasp::HasSelectedField())
    return PrimaryLabelAction::OPEN_SETUP;

  return PrimaryLabelAction::OPEN_PICKER;
}

[[nodiscard]]
SecondaryLabelAction
RaspControlsModel::GetSecondaryLabelAction() const noexcept
{
  return SecondaryLabelAction::OPEN_PICKER;
}

void
RaspControlsModel::OpenPrimaryPicker() noexcept
{
  Rasp::OpenTimePicker();
  NotifyOverlay();
}

void
RaspControlsModel::ResumePrimaryAuto() noexcept
{
  if (GetPrimaryAutoAdvance())
    return;

  EnablePrimaryAutoFromInput();
}

SecondaryPickerResult
RaspControlsModel::OpenSecondaryPicker() noexcept
{
  /* No RASP file / no layer: open Setup (destroys this widget). */
  if (!Rasp::HasSelectedField())
    return SecondaryPickerResult::OPEN_SETUP;

  /* Do not open Weather Setup from here — it can destroy
     ControlsWidget (and this model) while we are still on the call
     stack. */
  return HandleSecondaryFieldPicker(Rasp::OpenLayerPicker(true), [this] {
    Notify(ControlsUpdate::OVERLAY);
  });
}

void
RaspControlsModel::RefreshOverlay() noexcept
{
}

void
RaspControlsModel::OnGPSUpdate(const MoreData &basic) noexcept
{
  if (!GetPrimaryAutoAdvance())
    return;

  Notify(ControlsUpdate::LABELS);
  Rasp::MaybeRequestConfiguredRaspUpdateOnAutoNoData();

  if (!basic.date_time_utc.IsPlausible())
    return;

  const auto local = (basic.date_time_utc +
                      CommonInterface::GetComputerSettings().utc_offset
                      .ToDuration()).FloorToQuarterHour();
  const unsigned quarter =
    RaspStore::TimeToIndex(BrokenTime(local.hour, local.minute));
  if (quarter == last_quarter)
    return;

  last_quarter = quarter;
  ActionInterface::SendUIState(true);
}

} // namespace WeatherMapOverlay
