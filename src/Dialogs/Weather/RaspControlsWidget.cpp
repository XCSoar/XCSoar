// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspControlsWidget.hpp"

#include "Blackboard/BlackboardListenerRegistration.hpp"
#include "ActionInterface.hpp"
#include "Dialogs/Message.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"
#include "Weather/MapOverlay/InputEventMisc.hpp"
#include "Weather/MapOverlay/RaspControlsModel.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "util/StaticString.hxx"

#ifdef HAVE_DOWNLOAD_MANAGER
#include "Weather/Rasp/DownloadGlue.hpp"
#endif

#include <memory>

struct RaspControlsWidget::Private {
  WeatherMapOverlay::RaspControlsModel model;
  BlackboardListenerRegistration blackboard_listener;
  unsigned last_quarter = unsigned(-1);
};

RaspControlsWidget::RaspControlsWidget()
  :CursorBarWidget(1),
   data(std::make_unique<Private>())
{
  SetStepCallback([this](unsigned row, int delta) {
    if (row == TIME_ROW)
      OnStepTime(delta);
  });
  SetLabelClickCallback([this](unsigned row) {
    (void)row;
    OnResumeAuto();
  });
}

RaspControlsWidget::~RaspControlsWidget() noexcept
{
  data->blackboard_listener.Reset();
}

void
RaspControlsWidget::UpdateLabels() noexcept
{
  StaticString<64> time_text;
  data->model.FormatTimeLabel(time_text);
  SetRowText(TIME_ROW, time_text, data->model.HasTimeData());
}

void
RaspControlsWidget::RefreshRaspOverlay() noexcept
{
  UpdateLabels();
  ActionInterface::SendUIState(true);
}

void
RaspControlsWidget::OnStepTime(int delta) noexcept
{
  if (data->model.StepTime(delta))
    RefreshRaspOverlay();
  else
    UpdateLabels();
}

void
RaspControlsWidget::OnResumeAuto() noexcept
{
  if (data->model.GetTimeAutoAdvance())
    return;

  data->model.ResumeAutoAdvance();
  data->last_quarter = unsigned(-1);
  RefreshRaspOverlay();

#ifdef HAVE_DOWNLOAD_MANAGER
  if (!data->model.HasTimeData())
    RequestConfiguredRaspUpdateIfOutOfDate();
#endif
}

void
RaspControlsWidget::HandleWeatherOverlayInput(const char *misc) noexcept
{
  switch (WeatherMapOverlay::ParseOverlayInputAction(misc)) {
  case WeatherMapOverlay::OverlayInputAction::TIME_PLUS:
    OnStepTime(+1);
    break;

  case WeatherMapOverlay::OverlayInputAction::TIME_MINUS:
    OnStepTime(-1);
    break;

  case WeatherMapOverlay::OverlayInputAction::TIME_AUTO_TOGGLE:
    data->model.SetTimeAutoAdvance(!data->model.GetTimeAutoAdvance());
    if (data->model.GetTimeAutoAdvance())
      data->model.ApplyAutoAdvanceTime();
    RefreshRaspOverlay();
    break;

  case WeatherMapOverlay::OverlayInputAction::TIME_AUTO_ON:
    data->model.SetTimeAutoAdvance(true);
    data->model.ApplyAutoAdvanceTime();
    RefreshRaspOverlay();
    break;

  case WeatherMapOverlay::OverlayInputAction::TIME_AUTO_OFF:
    data->model.SetTimeAutoAdvance(false);
    UpdateLabels();
    break;

  case WeatherMapOverlay::OverlayInputAction::TIME_AUTO_SHOW:
    if (data->model.GetTimeAutoAdvance())
      Message::AddMessage(_("Auto. weather time on"));
    else
      Message::AddMessage(_("Auto. weather time off"));
    break;

  case WeatherMapOverlay::OverlayInputAction::NONE:
  case WeatherMapOverlay::OverlayInputAction::ALTITUDE_PLUS:
  case WeatherMapOverlay::OverlayInputAction::ALTITUDE_MINUS:
  case WeatherMapOverlay::OverlayInputAction::ALTITUDE_AUTO_TOGGLE:
  case WeatherMapOverlay::OverlayInputAction::ALTITUDE_AUTO_ON:
  case WeatherMapOverlay::OverlayInputAction::ALTITUDE_AUTO_OFF:
  case WeatherMapOverlay::OverlayInputAction::ALTITUDE_AUTO_SHOW:
    break;
  }
}

void
RaspControlsWidget::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  CursorBarWidget::Prepare(parent, rc);
}

void
RaspControlsWidget::Unprepare() noexcept
{
  data->blackboard_listener.Reset();
  CursorBarWidget::Unprepare();
}

void
RaspControlsWidget::Show(const PixelRect &rc) noexcept
{
  data->model.OnShow();
  data->last_quarter = unsigned(-1);

  CursorBarWidget::Show(rc);
  UpdateLabels();
  data->blackboard_listener.Register(
    CommonInterface::GetLiveBlackboard(), *this);
  RefreshRaspOverlay();
}

void
RaspControlsWidget::Hide() noexcept
{
  data->blackboard_listener.Reset();
  CursorBarWidget::Hide();
}

void
RaspControlsWidget::OnGPSUpdate(const MoreData &basic)
{
  if (!data->model.GetTimeAutoAdvance())
    return;

  UpdateLabels();
  Rasp::MaybeRequestConfiguredRaspUpdateOnAutoNoData();

  if (!basic.date_time_utc.IsPlausible())
    return;

  const auto local = basic.date_time_utc.ToLocal().FloorToQuarterHour();
  const unsigned quarter = unsigned(local.hour) * 4u +
    unsigned(local.minute) / 15u;
  if (quarter == data->last_quarter)
    return;

  data->last_quarter = quarter;
  ActionInterface::SendUIState(true);
}
