// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspControlsWidget.hpp"

#include "Blackboard/BlackboardListenerRegistration.hpp"
#include "ActionInterface.hpp"
#include "DataGlobals.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Weather/Rasp/RaspStore.hpp"
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
  :CursorBarWidget(2),
   data(std::make_unique<Private>())
{
  SetStepCallback([this](unsigned row, int delta) {
    if (row == TIME_ROW)
      OnStepTime(delta);
    else if (row == FIELD_ROW)
      OnStepField(delta);
  });
  SetLabelClickCallback([this](unsigned row) {
    if (row == TIME_ROW)
      OnResumeAuto();
    else if (row == FIELD_ROW)
      OnPickField();
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

  StaticString<64> field_text;
  data->model.FormatFieldLabel(field_text);
  SetRowText(FIELD_ROW, field_text, data->model.HasFieldData());
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
RaspControlsWidget::OnStepField(int delta) noexcept
{
  if (data->model.StepField(delta))
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
RaspControlsWidget::OnPickField() noexcept
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

  data->model.SelectField(unsigned(selected));
  RefreshRaspOverlay();
}

void
RaspControlsWidget::HandleWeatherOverlayInput(const char *misc) noexcept
{
  if (misc == nullptr || *misc == '\0')
    return;

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
    WeatherMapOverlay::ShowAutoTimeStatusMessage(
      data->model.GetTimeAutoAdvance());
    break;

  case WeatherMapOverlay::OverlayInputAction::NONE:
    break;

  case WeatherMapOverlay::OverlayInputAction::ALTITUDE_PLUS:
    OnStepField(+1);
    break;

  case WeatherMapOverlay::OverlayInputAction::ALTITUDE_MINUS:
    OnStepField(-1);
    break;

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
