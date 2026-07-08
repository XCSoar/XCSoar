// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspControlsModel.hpp"

#include "ActionInterface.hpp"
#include "DataGlobals.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "Weather/Rasp/RaspStore.hpp"

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
}

[[nodiscard]]
SecondaryLabelAction
RaspControlsModel::GetSecondaryLabelAction() const noexcept
{
  return SecondaryLabelAction::OPEN_PICKER;
}

void
RaspControlsModel::ResumePrimaryAuto() noexcept
{
  if (GetPrimaryAutoAdvance())
    return;

  Rasp::ResumeAutoAdvance();
  last_quarter = unsigned(-1);
  Notify(ControlsUpdate::OVERLAY);

#ifdef HAVE_DOWNLOAD_MANAGER
  if (!Rasp::HasSelectedTimeData(true))
    RequestConfiguredRaspUpdateIfOutOfDate();
#endif
}

void
RaspControlsModel::OpenSecondaryPicker() noexcept
{
  const auto rasp = DataGlobals::GetRasp();
  if (rasp == nullptr || rasp->GetItemCount() == 0)
    return;

  DataFieldEnum field;
  Rasp::FillFieldChoices(field, rasp.get());

  const int current = Rasp::GetEffectiveFieldIndex();
  field.SetValue(current >= 0 ? current : 0);

  if (!ComboPicker(_("RASP Layer"), field, nullptr))
    return;

  const int selected = field.GetValue();
  if (selected < 0)
    return;

  Rasp::SelectField(unsigned(selected));
  Notify(ControlsUpdate::OVERLAY);
}

void
RaspControlsModel::RefreshOverlay() noexcept
{
  ActionInterface::SendUIState(true);
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

  const auto local = basic.date_time_utc.ToLocal().FloorToQuarterHour();
  const unsigned quarter =
    RaspStore::TimeToIndex(BrokenTime(local.hour, local.minute));
  if (quarter == last_quarter)
    return;

  last_quarter = quarter;
  ActionInterface::SendUIState(true);
}

} // namespace WeatherMapOverlay
