// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EdlControlsWidget.hpp"

#include "Blackboard/BlackboardListenerRegistration.hpp"
#include "Weather/Features.hpp"
#include "Components.hpp"
#include "Dialogs/Message.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "NetComponents.hpp"
#include "PageSettings.hpp"
#include "Weather/EDL/DownloadGlue.hpp"
#include "ActionInterface.hpp"
#include "Weather/EDL/Glue.hpp"
#include "Weather/EDL/StateController.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"
#include "Weather/MapOverlay/EdlControlsModel.hpp"
#include "util/StaticString.hxx"

#include <memory>

struct EdlControlsWidget::Private {
  WeatherMapOverlay::EdlControlsModel model;
  BlackboardListenerRegistration blackboard_listener;
  EDL::DownloadGlue *edl_listener_glue = nullptr;
};

EdlControlsWidget::EdlControlsWidget()
  :data(std::make_unique<Private>())
{
  SetStepCallback([this](unsigned row, int delta) {
    if (row == FORECAST_ROW)
      OnStepForecast(delta);
    else if (row == LEVEL_ROW)
      OnStepLevel(delta);
  });
  SetLabelClickCallback([this](unsigned row) {
    (void)row;
    OnResumeAuto();
  });
}

EdlControlsWidget::~EdlControlsWidget() noexcept
{
  data->blackboard_listener.Reset();
  UnregisterEdlDownloadListener();
}

void
EdlControlsWidget::UnregisterEdlDownloadListener() noexcept
{
  if (data->edl_listener_glue == nullptr)
    return;

  data->edl_listener_glue->RemoveListener(*this);
  data->edl_listener_glue = nullptr;
}

void
EdlControlsWidget::UpdateLabels() noexcept
{
  const bool has_data = data->model.HasOverlayData();

  StaticString<64> forecast_text;
  data->model.FormatForecastLabel(forecast_text);
  SetRowText(FORECAST_ROW, forecast_text, has_data);

  StaticString<64> level_text;
  data->model.FormatLevelLabel(level_text);
  SetRowText(LEVEL_ROW, level_text, has_data);
}

void
EdlControlsWidget::RefreshEdlOverlay() noexcept
{
#if !defined(HAVE_HTTP)
  ShowMessageBox(_("HTTP support is not available in this build."),
                 _("Weather"), MB_OK);
#else
  UpdateLabels();

  if (!EDL::OverlayEnabled())
    return;

  EDL::RequestOverlayRefresh();
#endif
}

void
EdlControlsWidget::OnStepForecast(int delta) noexcept
{
  if (data->model.StepForecast(delta))
    RefreshEdlOverlay();
  else
    UpdateLabels();
}

void
EdlControlsWidget::OnStepLevel(int delta) noexcept
{
  if (data->model.StepLevel(delta))
    RefreshEdlOverlay();
  else
    UpdateLabels();
}

void
EdlControlsWidget::OnResumeAuto() noexcept
{
  if (data->model.GetForecastAutoAdvance() &&
      data->model.GetLevelAutoAdvance())
    return;

  data->model.ResumeAutoAdvance();
  RefreshEdlOverlay();
}

void
EdlControlsWidget::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  CursorBarWidget::Prepare(parent, rc);
}

void
EdlControlsWidget::Unprepare() noexcept
{
  EDL::ClearGpsUiRefreshPending();
  data->blackboard_listener.Reset();
  UnregisterEdlDownloadListener();
  CursorBarWidget::Unprepare();
}

void
EdlControlsWidget::Show(const PixelRect &rc) noexcept
{
  data->model.OnShow();

  CursorBarWidget::Show(rc);
  UpdateLabels();
  data->blackboard_listener.Register(
    CommonInterface::GetLiveBlackboard(), *this);

  if (data->edl_listener_glue == nullptr &&
      net_components != nullptr && net_components->edl != nullptr) {
    data->edl_listener_glue = net_components->edl.get();
    data->edl_listener_glue->AddListener(*this);
  }

  RefreshEdlOverlay();
}

void
EdlControlsWidget::Hide() noexcept
{
  EDL::ClearGpsUiRefreshPending();
  data->blackboard_listener.Reset();
  UnregisterEdlDownloadListener();

  CursorBarWidget::Hide();
}

void
EdlControlsWidget::OnGPSUpdate([[maybe_unused]] const MoreData &basic)
{
  if (!data->model.GetForecastAutoAdvance() &&
      !data->model.GetLevelAutoAdvance())
    return;

  if (EDL::TakeGpsUiRefreshPending())
    RefreshEdlOverlay();
  else
    UpdateLabels();
}

void
EdlControlsWidget::OnDownloadFinished(
  const EDL::DownloadNotification &notification) noexcept
{
  UpdateLabels();

  if (notification.job != EDL::DownloadJob::OVERLAY ||
      EDL::OverlayVisible())
    return;

  if (EDL::TryApplyOverlayFromCache())
    ActionInterface::ScheduleSendUIState();
}
