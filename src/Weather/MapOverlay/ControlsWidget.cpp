// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ControlsWidget.hpp"

#include "ActionInterface.hpp"
#include "CursorBarLabels.hpp"
#include "InputEventMisc.hpp"
#include "Interface.hpp"

#include <cassert>
#include <utility>

namespace WeatherMapOverlay {

ControlsWidget::ControlsWidget(std::unique_ptr<ControlsModel> _model) noexcept
  :CursorBarWidget(2),
   model(std::move(_model))
{
  assert(model != nullptr);

  model->SetNotify([this](ControlsUpdate update) {
    ApplyUpdate(update);
  });

  SetStepCallback([this](unsigned row, int delta) {
    if (row == PRIMARY_ROW)
      OnStepPrimary(delta);
    else if (row == SECONDARY_ROW)
      OnStepSecondary(delta);
  });

  SetLabelClickCallback([this](unsigned row) {
    if (row == PRIMARY_ROW)
      OnPrimaryLabelClick();
    else if (row == SECONDARY_ROW)
      OnSecondaryLabelClick();
  });
}

ControlsWidget::~ControlsWidget() noexcept
{
  UnregisterBlackboard();
  model->SetNotify({});
}

void
ControlsWidget::RegisterBlackboard() noexcept
{
  blackboard_listener.Register(CommonInterface::GetLiveBlackboard(),
                               *this);
}

void
ControlsWidget::UnregisterBlackboard() noexcept
{
  blackboard_listener.Reset();
}

void
ControlsWidget::UpdateLabels() noexcept
{
  StaticString<64> primary_text;
  model->FormatPrimaryLabel(primary_text);
  SetRowText(PRIMARY_ROW, primary_text, model->HasPrimaryData());

  StaticString<64> secondary_text;
  model->FormatSecondaryLabel(secondary_text);
  SetRowText(SECONDARY_ROW, secondary_text, model->HasSecondaryData());
}

void
ControlsWidget::RefreshOverlay() noexcept
{
  UpdateLabels();
  model->RefreshOverlay();
  ActionInterface::SendUIState(true);
}

void
ControlsWidget::ApplyUpdate(ControlsUpdate update) noexcept
{
  switch (update) {
  case ControlsUpdate::NONE:
    break;

  case ControlsUpdate::LABELS:
    UpdateLabels();
    ActionInterface::SendUIState(false);
    break;

  case ControlsUpdate::OVERLAY:
    RefreshOverlay();
    break;
  }
}

void
ControlsWidget::OnStepPrimary(int delta) noexcept
{
  if (model->StepPrimary(delta))
    RefreshOverlay();
  else
    UpdateLabels();
}

void
ControlsWidget::OnStepSecondary(int delta) noexcept
{
  if (model->StepSecondary(delta))
    RefreshOverlay();
  else
    UpdateLabels();
}

void
ControlsWidget::OnPrimaryLabelClick() noexcept
{
  switch (model->GetPrimaryLabelAction()) {
  case PrimaryLabelAction::OPEN_PICKER:
    model->OpenPrimaryPicker();
    UpdateLabels();
    break;

  case PrimaryLabelAction::RESUME_AUTO:
    if (!model->GetPrimaryAutoAdvance())
      model->ResumePrimaryAuto();
    break;

  case PrimaryLabelAction::NONE:
    break;
  }
}

void
ControlsWidget::OnSecondaryLabelClick() noexcept
{
  switch (model->GetSecondaryLabelAction()) {
  case SecondaryLabelAction::OPEN_PICKER:
    model->OpenSecondaryPicker();
    RefreshOverlay();
    break;

  case SecondaryLabelAction::NONE:
    break;
  }
}

void
ControlsWidget::HandleWeatherOverlayInput(const char *misc) noexcept
{
  if (misc == nullptr || *misc == '\0')
    return;

  switch (ParseOverlayInputAction(misc)) {
  case OverlayInputAction::TIME_PICKER:
    OnPrimaryLabelClick();
    break;

  case OverlayInputAction::TIME_PLUS:
    OnStepPrimary(+1);
    break;

  case OverlayInputAction::TIME_MINUS:
    OnStepPrimary(-1);
    break;

  case OverlayInputAction::ALTITUDE_PLUS:
    OnStepSecondary(+1);
    break;

  case OverlayInputAction::ALTITUDE_MINUS:
    OnStepSecondary(-1);
    break;

  case OverlayInputAction::FIELD_PICKER:
    model->OpenSecondaryPicker();
    RefreshOverlay();
    break;

  case OverlayInputAction::TIME_AUTO_TOGGLE:
    if (model->GetPrimaryAutoAdvance()) {
      model->SetPrimaryAutoAdvance(false);
      UpdateLabels();
    } else {
      model->EnablePrimaryAutoFromInput();
    }
    break;

  case OverlayInputAction::TIME_AUTO_ON:
    model->EnablePrimaryAutoFromInput();
    break;

  case OverlayInputAction::TIME_AUTO_OFF:
    model->SetPrimaryAutoAdvance(false);
    UpdateLabels();
    break;

  case OverlayInputAction::TIME_AUTO_SHOW:
    ShowAutoTimeStatusMessage(model->GetPrimaryAutoAdvance());
    break;

  case OverlayInputAction::ALTITUDE_AUTO_TOGGLE:
    if (!model->SupportsSecondaryAutoAdvance())
      break;
    model->SetSecondaryAutoAdvance(!model->GetSecondaryAutoAdvance());
    if (model->GetSecondaryAutoAdvance())
      model->ApplySecondaryAutoAdvance();
    RefreshOverlay();
    break;

  case OverlayInputAction::ALTITUDE_AUTO_ON:
    if (!model->SupportsSecondaryAutoAdvance())
      break;
    model->SetSecondaryAutoAdvance(true);
    model->ApplySecondaryAutoAdvance();
    RefreshOverlay();
    break;

  case OverlayInputAction::ALTITUDE_AUTO_OFF:
    if (!model->SupportsSecondaryAutoAdvance())
      break;
    model->SetSecondaryAutoAdvance(false);
    UpdateLabels();
    break;

  case OverlayInputAction::ALTITUDE_AUTO_SHOW:
    if (!model->SupportsSecondaryAutoAdvance())
      break;
    ShowAutoAltitudeStatusMessage(model->GetSecondaryAutoAdvance());
    break;

  case OverlayInputAction::NONE:
    break;
  }
}

void
ControlsWidget::Prepare(ContainerWindow &parent,
                        const PixelRect &rc) noexcept
{
  CursorBarWidget::Prepare(parent, rc);
}

void
ControlsWidget::Unprepare() noexcept
{
  model->OnHide();
  UnregisterBlackboard();
  CursorBarWidget::Unprepare();
}

void
ControlsWidget::Show(const PixelRect &rc) noexcept
{
  model->OnShow();

  CursorBarWidget::Show(rc);
  UpdateLabels();
  RegisterBlackboard();
}

void
ControlsWidget::Hide() noexcept
{
  model->OnHide();
  UnregisterBlackboard();
  CursorBarWidget::Hide();
}

void
ControlsWidget::OnGPSUpdate(const MoreData &basic)
{
  model->OnGPSUpdate(basic);
}

} // namespace WeatherMapOverlay
