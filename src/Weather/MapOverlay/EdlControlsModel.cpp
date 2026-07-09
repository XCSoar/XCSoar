// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EdlControlsModel.hpp"

#include "ActionInterface.hpp"
#include "Components.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Dialogs/Message.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "NetComponents.hpp"
#include "UIState.hpp"
#include "Weather/EDL/Glue.hpp"
#include "Weather/EDL/Levels.hpp"
#include "Weather/EDL/StateController.hpp"
#include "Weather/EDL/TileStore.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"

#include <chrono>
#include <limits>

namespace WeatherMapOverlay {

static constexpr unsigned TIME_PICKER_AUTO =
  std::numeric_limits<unsigned>::max() - 1;
static constexpr unsigned TIME_PICKER_NOW =
  std::numeric_limits<unsigned>::max();

EdlControlsModel::~EdlControlsModel() noexcept
{
  UnregisterEdlDownloadListener();
}

void
EdlControlsModel::UnregisterEdlDownloadListener() noexcept
{
  if (edl_listener_glue == nullptr)
    return;

  edl_listener_glue->RemoveListener(*this);
  edl_listener_glue = nullptr;
}

void
EdlControlsModel::OnShow() noexcept
{
  EDL::EnsureInitialised();

  if (edl_listener_glue == nullptr &&
      net_components != nullptr && net_components->edl != nullptr) {
    edl_listener_glue = net_components->edl.get();
    edl_listener_glue->AddListener(*this);
  }
}

void
EdlControlsModel::OnHide() noexcept
{
  EDL::ClearGpsUiRefreshPending();
  UnregisterEdlDownloadListener();
}

void
EdlControlsModel::SelectForecast(unsigned index) noexcept
{
  if (index >= forecast_times.size())
    return;

  EDL::EnsureInitialised();
  auto &edl = CommonInterface::SetUIState().weather.edl;
  edl.forecast_datetime = forecast_times[index];
  edl.forecast_auto_advance = false;
  edl.session.cursor_initialized = true;
}

void
EdlControlsModel::RebuildForecastTimes() noexcept
{
  EDL::EnsureInitialised();

  auto selected_time = EDL::GetForecastTime();
  if (!selected_time.IsPlausible())
    selected_time = EDL::GetTrackedForecastTime(BrokenDateTime::NowUTC());

  if (!selected_time.IsPlausible())
    return;

  const auto base_time = selected_time + std::chrono::hours{-11};
  for (unsigned i = 0; i < forecast_choices; ++i)
    forecast_times[i] = base_time + std::chrono::hours{i};
}

unsigned
EdlControlsModel::FindForecastIndex() const noexcept
{
  const auto selected = EDL::GetForecastTime();
  for (unsigned i = 0; i < forecast_choices; ++i)
    if (forecast_times[i] == selected)
      return i;

  return 0;
}

bool
EdlControlsModel::StepPrimary(int delta) noexcept
{
  RebuildForecastTimes();

  const int index = int(FindForecastIndex()) + delta;
  if (index < 0 || index >= int(forecast_choices))
    return false;

  SelectForecast(unsigned(index));
  return true;
}

bool
EdlControlsModel::StepSecondary(int delta) noexcept
{
  EDL::EnsureInitialised();

  const unsigned current = EDL::GetIsobar();
  int index = -1;
  for (unsigned i = 0; i < EDL::NUM_ISOBARS; ++i) {
    if (EDL::ISOBARS[i] == current) {
      index = int(i);
      break;
    }
  }

  if (index < 0)
    index = 0;

  /* ISOBARS are ascending pressure (descending altitude); invert delta so
     "<" steps down and ">" steps up in height. */
  const int new_index = index - delta;
  if (new_index < 0 || new_index >= int(EDL::NUM_ISOBARS))
    return false;

  SelectLevel(EDL::ISOBARS[unsigned(new_index)]);
  return true;
}

void
EdlControlsModel::ResumePrimaryAuto() noexcept
{
  if (GetPrimaryAutoAdvance())
    return;

  SetPrimaryAutoAdvance(true);
  ApplyPrimaryAutoAdvance();
  Notify(ControlsUpdate::OVERLAY);
}

void
EdlControlsModel::ResumeSecondaryAuto() noexcept
{
  if (GetSecondaryAutoAdvance())
    return;

  SetSecondaryAutoAdvance(true);
  ApplySecondaryAutoAdvance();
  Notify(ControlsUpdate::OVERLAY);
}

void
EdlControlsModel::FormatPrimaryLabel(StaticString<64> &text) const noexcept
{
  StaticString<64> base;
  EDL::FormatForecastCursorLabel(base, GetPrimaryAutoAdvance());

  if (HasOverlayData())
    text = base;
  else
    AppendNoDataTag(text, base.c_str());
}

void
EdlControlsModel::FormatSecondaryLabel(StaticString<64> &text) const noexcept
{
  const unsigned isobar = EDL::GetIsobar();
  const int altitude = EDL::GetAltitudeForIsobar(isobar);

  StaticString<64> base;
  if (GetSecondaryAutoAdvance())
    base.Format(_("AUTO: %u hPa (%d m)"), isobar / 100, altitude);
  else
    base.Format(_("%u hPa (%d m)"), isobar / 100, altitude);

  if (HasOverlayData())
    text = base;
  else
    AppendNoDataTag(text, base.c_str());
}

bool
EdlControlsModel::HasOverlayData() const noexcept
{
  return EDL::HasOverlayCache();
}

bool
EdlControlsModel::HasPrimaryData() const noexcept
{
  return HasOverlayData();
}

bool
EdlControlsModel::HasSecondaryData() const noexcept
{
  return HasOverlayData();
}

bool
EdlControlsModel::GetPrimaryAutoAdvance() const noexcept
{
  return CommonInterface::GetUIState().weather.edl.forecast_auto_advance;
}

void
EdlControlsModel::SetPrimaryAutoAdvance(bool auto_advance) noexcept
{
  CommonInterface::SetUIState().weather.edl.forecast_auto_advance =
    auto_advance;
}

void
EdlControlsModel::ApplyPrimaryAutoAdvance() noexcept
{
  if (!GetPrimaryAutoAdvance())
    return;

  const auto &basic = CommonInterface::Basic();
  if (basic.date_time_utc.IsPlausible())
    EDL::OnTimeUpdate(basic.date_time_utc);
}

PrimaryLabelAction
EdlControlsModel::GetPrimaryLabelAction() const noexcept
{
  return PrimaryLabelAction::OPEN_PICKER;
}

void
EdlControlsModel::OpenPrimaryPicker() noexcept
{
  RebuildForecastTimes();

  DataFieldEnum picker;
  picker.ClearChoices();
  picker.addEnumText(_("Auto"), TIME_PICKER_AUTO);
  picker.addEnumText(_("Now"), TIME_PICKER_NOW);

  for (unsigned i = 0; i < forecast_choices; ++i) {
    const auto local = forecast_times[i].ToLocal();
    StaticString<16> label;
    label.Format("%02u:00", unsigned(local.hour));
    picker.addEnumText(label.c_str(), i);
  }

  if (GetPrimaryAutoAdvance())
    picker.SetValue(TIME_PICKER_AUTO);
  else
    picker.SetValue(FindForecastIndex());

  if (!ComboPicker(_("EDL Time"), picker, nullptr))
    return;

  const unsigned selected = picker.GetValue();
  if (selected == TIME_PICKER_AUTO) {
    ResumePrimaryAuto();
    return;
  }

  if (selected == TIME_PICKER_NOW) {
    const BrokenDateTime tracked = EDL::GetTrackedForecastTime(BrokenDateTime::NowUTC());
    for (unsigned i = 0; i < forecast_choices; ++i) {
      if (forecast_times[i] == tracked) {
        SetPrimaryAutoAdvance(false);
        SelectForecast(i);
        Notify(ControlsUpdate::OVERLAY);
        return;
      }
    }
    return;
  } else if (selected < forecast_choices) {
    SetPrimaryAutoAdvance(false);
    SelectForecast(selected);
    Notify(ControlsUpdate::OVERLAY);
  }
}

bool
EdlControlsModel::SupportsSecondaryAutoAdvance() const noexcept
{
  return true;
}

bool
EdlControlsModel::GetSecondaryAutoAdvance() const noexcept
{
  return CommonInterface::GetUIState().weather.edl.level_auto_advance;
}

void
EdlControlsModel::SetSecondaryAutoAdvance(bool auto_advance) noexcept
{
  CommonInterface::SetUIState().weather.edl.level_auto_advance =
    auto_advance;
}

void
EdlControlsModel::ApplySecondaryAutoAdvance() noexcept
{
  EDL::UpdateCurrentLevel();
}

void
EdlControlsModel::RefreshOverlay() noexcept
{
#if !defined(HAVE_HTTP)
  ShowMessageBox(_("HTTP support is not available in this build."),
                 _("Weather"), MB_OK);
#else
  if (!EDL::OverlayEnabled())
    return;

  EDL::RequestOverlayRefresh();
#endif
}

void
EdlControlsModel::OnGPSUpdate([[maybe_unused]] const MoreData &basic) noexcept
{
  if (!(GetPrimaryAutoAdvance() || GetSecondaryAutoAdvance()))
    return;

  Notify(EDL::TakeGpsUiRefreshPending()
         ? ControlsUpdate::OVERLAY
         : ControlsUpdate::LABELS);
}

void
EdlControlsModel::OnDownloadFinished(
  const EDL::DownloadNotification &notification) noexcept
{
  Notify(ControlsUpdate::LABELS);

  if (notification.job != EDL::DownloadJob::OVERLAY ||
      EDL::OverlayVisible())
    return;

  if (EDL::TryApplyOverlayFromCache())
    ActionInterface::ScheduleSendUIState();
}

void
EdlControlsModel::SelectLevel(unsigned isobar) noexcept
{
  EDL::EnsureInitialised();
  auto &edl = CommonInterface::SetUIState().weather.edl;
  edl.SelectIsobar(isobar);
  edl.level_auto_advance = false;
  edl.session.cursor_initialized = true;
}

} // namespace WeatherMapOverlay
