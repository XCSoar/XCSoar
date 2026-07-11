// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspControlsModel.hpp"

#include "ActionInterface.hpp"
#include "DataGlobals.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "PageActions.hpp"
#include "PrimaryTimePicker.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "UIState.hpp"
#include "util/StaticString.hxx"

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
  EnablePrimaryAutoAndRefresh();

#ifdef HAVE_DOWNLOAD_MANAGER
  if (!Rasp::HasSelectedTimeData(true))
    RequestConfiguredRaspUpdateIfOutOfDate();
#endif
}

PrimaryLabelAction
RaspControlsModel::GetPrimaryLabelAction() const noexcept
{
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
  const auto rasp = DataGlobals::GetRasp();
  const int field_index = Rasp::GetEffectiveFieldIndex();
  if (rasp == nullptr || field_index < 0 ||
      unsigned(field_index) >= rasp->GetItemCount())
    return;

  OpenPrimaryTimePicker(*this, _("RASP Time"),
    [&](DataFieldEnum &field) noexcept {
      field.ClearChoices();

      for (unsigned i = 0; i < RaspStore::MAX_WEATHER_TIMES; ++i) {
        const BrokenTime t = RaspStore::IndexToTime(i);
        StaticString<24> label;
        label.Format("%02u:%02u %s",
                     unsigned(t.hour), unsigned(t.minute),
                     rasp->IsTimeAvailable(unsigned(field_index), i)
                     ? "[x]" : "[ ]");
        field.addEnumText(label.c_str(), t.GetMinuteOfDay());
      }
    },
    []() noexcept {
      return Rasp::GetCursorBarMinuteOfDay();
    },
    [](ControlsModel &model) noexcept {
      model.EnablePrimaryAutoFromInput();
    },
    [](ControlsModel &) noexcept {
      Rasp::SetCursorNow();
    },
    [](ControlsModel &, unsigned minute_of_day) noexcept {
      Rasp::SetCursorTime(minute_of_day);
    });
}

void
RaspControlsModel::ResumePrimaryAuto() noexcept
{
  if (GetPrimaryAutoAdvance())
    return;

  EnablePrimaryAutoFromInput();
}

void
RaspControlsModel::OpenSecondaryPicker() noexcept
{
  const auto rasp = DataGlobals::GetRasp();
  if (rasp == nullptr || rasp->GetItemCount() == 0)
    return;

  DataFieldEnum field;
  Rasp::FieldChoicesOptions options;
  options.include_none = true;
  Rasp::FillFieldChoices(field, rasp.get(), options);

  const int current = Rasp::GetEffectiveFieldIndex();
  field.SetValue(current >= 0 ? current : 0);

  if (!ComboPicker(_("RASP Layer"), field, nullptr))
    return;

  const int selected = field.GetValue();
  if (selected < 0) {
    Rasp::ClearSelectedField();
    return;
  }

  Rasp::SelectField(unsigned(selected));
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
