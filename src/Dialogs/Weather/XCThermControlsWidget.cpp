// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermControlsWidget.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_HTTP

#include "Blackboard/BlackboardListenerRegistration.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/Weather/WeatherDialog.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"
#include "Weather/MapOverlay/InputEventMisc.hpp"
#include "Weather/xctherm/XCThermMapOverlay.hpp"
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
    OnLabelClicked(row);
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
XCThermControlsWidget::OnLabelClicked(unsigned row) noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  if (!settings.credentials.IsDefined() &&
      XCTherm::HasActiveMapOverlay()) {
    ShowWeatherDialog("xctherm");
    return;
  }

  if (!model.HasCacheAtCurrentHour(model.GetCurrentLayer())) {
    OnRequestDownload();
    return;
  }

  if (row == LAYER_ROW)
    OnResumeLayerAuto();
  else if (row == TIME_ROW)
    OnResumeTimeAuto();
}

void
XCThermControlsWidget::OnRequestDownload() noexcept
{
  if (model.RequestLayerDownload([this](std::shared_ptr<XCThermDownloadJob>){
        UpdateLabels();
      }))
    return;

  ShowWeatherDialog("xctherm");
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

void
XCThermControlsWidget::HandleWeatherOverlayInput(const char *misc) noexcept
{
  const MoreData &basic = CommonInterface::Basic();

  switch (WeatherMapOverlay::ParseOverlayInputAction(misc)) {
  case WeatherMapOverlay::OverlayInputAction::TIME_PLUS:
    OnStepTime(+1);
    break;

  case WeatherMapOverlay::OverlayInputAction::TIME_MINUS:
    OnStepTime(-1);
    break;

  case WeatherMapOverlay::OverlayInputAction::ALTITUDE_PLUS:
    OnStepLayer(+1);
    break;

  case WeatherMapOverlay::OverlayInputAction::ALTITUDE_MINUS:
    OnStepLayer(-1);
    break;

  case WeatherMapOverlay::OverlayInputAction::TIME_AUTO_TOGGLE:
    model.SetTimeAutoAdvance(!model.IsTimeAutoActive());
    UpdateLabels();
    break;

  case WeatherMapOverlay::OverlayInputAction::TIME_AUTO_ON:
    model.SetTimeAutoAdvance(true);
    UpdateLabels();
    break;

  case WeatherMapOverlay::OverlayInputAction::TIME_AUTO_OFF:
    model.SetTimeAutoAdvance(false);
    UpdateLabels();
    break;

  case WeatherMapOverlay::OverlayInputAction::TIME_AUTO_SHOW:
    if (model.IsTimeAutoActive())
      Message::AddMessage(_("Auto. weather time on"));
    else
      Message::AddMessage(_("Auto. weather time off"));
    break;

  case WeatherMapOverlay::OverlayInputAction::ALTITUDE_AUTO_TOGGLE:
    model.SetAltitudeAutoAdvance(!model.IsAltitudeAutoActive(), basic);
    UpdateLabels();
    break;

  case WeatherMapOverlay::OverlayInputAction::ALTITUDE_AUTO_ON:
    model.SetAltitudeAutoAdvance(true, basic);
    UpdateLabels();
    break;

  case WeatherMapOverlay::OverlayInputAction::ALTITUDE_AUTO_OFF:
    model.SetAltitudeAutoAdvance(false, basic);
    UpdateLabels();
    break;

  case WeatherMapOverlay::OverlayInputAction::ALTITUDE_AUTO_SHOW:
    if (model.IsAltitudeAutoActive())
      Message::AddMessage(_("Auto. weather altitude on"));
    else
      Message::AddMessage(_("Auto. weather altitude off"));
    break;

  case WeatherMapOverlay::OverlayInputAction::NONE:
    break;
  }
}

#endif /* HAVE_HTTP */
