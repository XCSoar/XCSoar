// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EdlControlsModel.hpp"

#include "ActionInterface.hpp"
#include "Components.hpp"
#include "Dialogs/Message.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "NetComponents.hpp"
#include "UIState.hpp"
#include "Weather/EDL/FieldControls.hpp"
#include "Weather/EDL/Glue.hpp"
#include "Weather/EDL/StateController.hpp"

namespace WeatherMapOverlay {

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

bool
EdlControlsModel::StepPrimary(int delta) noexcept
{
  return EDL::StepForecastTime(delta);
}

bool
EdlControlsModel::StepSecondary(int delta) noexcept
{
  return EDL::StepLevel(delta);
}

void
EdlControlsModel::ResumePrimaryAuto() noexcept
{
  if (GetPrimaryAutoAdvance())
    return;

  EnablePrimaryAutoFromInput();
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
  EDL::FormatTimeCursorLabel(text, GetPrimaryAutoAdvance());
}

void
EdlControlsModel::FormatSecondaryLabel(StaticString<64> &text) const noexcept
{
  EDL::FormatLevelCursorLabel(text, GetSecondaryAutoAdvance());
}

bool
EdlControlsModel::HasPrimaryData() const noexcept
{
  return EDL::HasOverlayCache();
}

bool
EdlControlsModel::HasSecondaryData() const noexcept
{
  return EDL::HasOverlayCache();
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

void
EdlControlsModel::EnablePrimaryAutoFromInput() noexcept
{
  EDL::EnableForecastAutoFromInput();
  NotifyOverlay();
}

PrimaryLabelAction
EdlControlsModel::GetPrimaryLabelAction() const noexcept
{
  return PrimaryLabelAction::OPEN_PICKER;
}

void
EdlControlsModel::OpenPrimaryPicker() noexcept
{
  EDL::OpenTimePicker();
  NotifyOverlay();
}

SecondaryLabelAction
EdlControlsModel::GetSecondaryLabelAction() const noexcept
{
  return SecondaryLabelAction::OPEN_PICKER;
}

SecondaryPickerResult
EdlControlsModel::OpenSecondaryPicker() noexcept
{
  return HandleSecondaryFieldPicker(EDL::OpenLevelPicker(true), [this] {
    Notify(ControlsUpdate::OVERLAY);
  });
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
  if (auto_advance)
    EDL::EnableLevelAutoFromInput();
  else {
    auto &edl = CommonInterface::SetUIState().weather.edl;
    edl.level_auto_advance = false;
    EDL::SelectLevel(edl.isobar);
  }
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
  const auto &state = CommonInterface::GetUIState().weather.edl;
  /* "Now" stores an invalid datetime and still needs hourly UI/overlay
     refresh even when neither Auto flag is set. */
  const bool follows_now = !state.forecast_auto_advance &&
    !state.forecast_datetime.IsPlausible();
  if (!(GetPrimaryAutoAdvance() || GetSecondaryAutoAdvance() ||
        follows_now))
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

} // namespace WeatherMapOverlay
