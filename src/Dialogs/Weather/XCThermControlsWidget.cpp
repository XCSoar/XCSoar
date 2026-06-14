// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermControlsWidget.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_HTTP

#include "Blackboard/BlackboardListenerRegistration.hpp"
#include "Interface.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"
#include "util/StaticString.hxx"

XCThermControlsWidget::XCThermControlsWidget()
  :CursorBarWidget(2)
{
  SetStepCallback([this](unsigned row, int delta) {
    if (row == LAYER_ROW)
      OnStepLayer(delta);
    else if (row == TIME_ROW)
      OnStepTime(delta);
  });
  SetLabelClickCallback([this](unsigned row) {
    if (row == LAYER_ROW)
      OnResumeLayerAuto();
    else if (row == TIME_ROW)
      OnResumeTimeAuto();
  });
}

void
XCThermControlsWidget::UpdateLabels() noexcept
{
  StaticString<80> layer_text;
  model.FormatLayerLabel(layer_text);
  SetRowText(LAYER_ROW, layer_text,
             model.HasCacheAtCurrentHour(model.GetCurrentLayer()));

  StaticString<64> time_text;
  model.FormatTimeLabel(time_text);
  SetRowText(TIME_ROW, time_text,
             model.HasCacheAtCurrentHour(model.GetCurrentLayer()));
}

void
XCThermControlsWidget::OnStepLayer(int delta) noexcept
{
  if (!model.StepLayer(delta)) {
    SetRowText(LAYER_ROW, WeatherMapOverlay::NoDataStepHint(), false);
    return;
  }

  UpdateLabels();
}

void
XCThermControlsWidget::OnStepTime(int delta) noexcept
{
  if (!model.StepTime(delta)) {
    SetRowText(TIME_ROW, WeatherMapOverlay::NoDataStepHint(), false);
    return;
  }

  UpdateLabels();
}

void
XCThermControlsWidget::OnResumeLayerAuto() noexcept
{
  model.ResumeLayerAuto(CommonInterface::Basic());
  UpdateLabels();
}

void
XCThermControlsWidget::OnResumeTimeAuto() noexcept
{
  model.ResumeTimeAuto();
  UpdateLabels();
}

void
XCThermControlsWidget::Prepare(ContainerWindow &parent,
                                 const PixelRect &rc) noexcept
{
  CursorBarWidget::Prepare(parent, rc);
  model.Prepare([this]{ UpdateLabels(); });
  UpdateLabels();
}

void
XCThermControlsWidget::Unprepare() noexcept
{
  blackboard_listener.Reset();
  CursorBarWidget::Unprepare();
}

void
XCThermControlsWidget::Show(const PixelRect &rc) noexcept
{
  CursorBarWidget::Show(rc);
  model.OnShow();
  blackboard_listener.Register(CommonInterface::GetLiveBlackboard(), *this);
  UpdateLabels();
}

void
XCThermControlsWidget::Hide() noexcept
{
  model.OnHide();
  blackboard_listener.Reset();
  CursorBarWidget::Hide();
}

void
XCThermControlsWidget::OnGPSUpdate(const MoreData &basic)
{
  model.OnGPSUpdate(basic);
  UpdateLabels();
}

#endif /* HAVE_HTTP */
