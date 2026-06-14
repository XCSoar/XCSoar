// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspControlsWidget.hpp"

#include "Blackboard/BlackboardListenerRegistration.hpp"
#include "ActionInterface.hpp"
#include "Interface.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"
#include "Weather/MapOverlay/RaspControlsModel.hpp"
#include "util/StaticString.hxx"

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
