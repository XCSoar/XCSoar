// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XcthermControlsModel.hpp"

#include "ActionInterface.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"

#ifdef HAVE_HTTP
#include "Weather/xctherm/XCThermControlsModel.hpp"
#endif

#include <algorithm>
#include <cstdio>
#include <limits>

namespace WeatherMapOverlay {

static constexpr unsigned TIME_PICKER_AUTO =
  std::numeric_limits<unsigned>::max() - 1;
static constexpr unsigned TIME_PICKER_NOW =
  std::numeric_limits<unsigned>::max();

XcthermControlsModel::XcthermControlsModel() noexcept
{
#ifdef HAVE_HTTP
  model = new XCTherm::XCThermControlsModel();
#endif
}

XcthermControlsModel::~XcthermControlsModel() noexcept
{
#ifdef HAVE_HTTP
  delete model;
#endif
}

void
XcthermControlsModel::OnShow() noexcept
{
#ifdef HAVE_HTTP
  if (model == nullptr)
    return;

  if (!prepared) {
    model->Prepare([this]() noexcept {
      Notify(ControlsUpdate::LABELS);
    });
    prepared = true;
  }

  model->OnShow();
#endif
}

void
XcthermControlsModel::OnHide() noexcept
{
#ifdef HAVE_HTTP
  if (model != nullptr)
    model->OnHide();
#endif
}

void
XcthermControlsModel::FormatPrimaryLabel(StaticString<64> &text) const noexcept
{
#ifdef HAVE_HTTP
  if (model != nullptr) {
    model->FormatTimeLabel(text);
    return;
  }
#endif
  text = "XCTherm";
}

void
XcthermControlsModel::FormatSecondaryLabel(StaticString<64> &text) const noexcept
{
#ifdef HAVE_HTTP
  if (model != nullptr) {
    StaticString<80> layer_text;
    model->FormatLayerLabel(layer_text);
    text = layer_text;
    return;
  }
#endif
  text = NoForecastHint();
}

bool
XcthermControlsModel::HasPrimaryData() const noexcept
{
#ifdef HAVE_HTTP
  if (model != nullptr)
    return model->HasCacheAtCurrentHour(model->GetCurrentLayer());
#endif
  return false;
}

bool
XcthermControlsModel::HasSecondaryData() const noexcept
{
#ifdef HAVE_HTTP
  if (model != nullptr)
    return model->HasCacheAtCurrentHour(model->GetCurrentLayer());
#endif
  return false;
}

bool
XcthermControlsModel::StepPrimary(int delta) noexcept
{
#ifdef HAVE_HTTP
  if (model != nullptr)
    return model->StepTime(delta);
#endif
  (void)delta;
  return true;
}

bool
XcthermControlsModel::StepSecondary(int delta) noexcept
{
#ifdef HAVE_HTTP
  if (model != nullptr)
    return model->StepLayer(delta);
#endif
  (void)delta;
  return true;
}

bool
XcthermControlsModel::GetPrimaryAutoAdvance() const noexcept
{
#ifdef HAVE_HTTP
  if (model != nullptr)
    return model->IsTimeAutoActive();
#endif
  return true;
}

void
XcthermControlsModel::SetPrimaryAutoAdvance(bool auto_advance) noexcept
{
#ifdef HAVE_HTTP
  if (model != nullptr)
    model->SetTimeAutoAdvance(auto_advance);
#else
  (void)auto_advance;
#endif
}

void
XcthermControlsModel::ApplyPrimaryAutoAdvance() noexcept
{
}

PrimaryLabelAction
XcthermControlsModel::GetPrimaryLabelAction() const noexcept
{
  return PrimaryLabelAction::OPEN_PICKER;
}

void
XcthermControlsModel::OpenPrimaryPicker() noexcept
{
#ifdef HAVE_HTTP
  if (model == nullptr)
    return;

  DataFieldEnum picker;
  picker.ClearChoices();
  picker.addEnumText(_("Auto"), TIME_PICKER_AUTO);

  const auto &state = model->GetState();
  const auto &cached_hours = state.cached_hours;
  const auto &basic = CommonInterface::Basic();
  const bool time_plausible = basic.date_time_utc.IsPlausible();
  if (time_plausible)
    picker.addEnumText(_("Now"), TIME_PICKER_NOW);

  for (unsigned hour = 0; hour < 24; ++hour) {
    char label[20];
    const bool has_data =
      std::find(cached_hours.begin(), cached_hours.end(), hour) !=
      cached_hours.end();
    std::snprintf(label, sizeof(label), "%02u:00 %s", hour,
                  has_data ? "[x]" : "[ ]");
    picker.addEnumText(label, hour);
  }

  if (model->IsTimeAutoActive()) {
    picker.SetValue(TIME_PICKER_AUTO);
  } else {
    if (!time_plausible)
      picker.SetValue(model->GetCurrentForecastHour());
    else {
      const unsigned now_hour = unsigned(basic.date_time_utc.hour);
      picker.SetValue(model->GetCurrentForecastHour() == now_hour
                      ? TIME_PICKER_NOW
                      : model->GetCurrentForecastHour());
    }
  }

  if (!ComboPicker(_("XCTherm Time"), picker, nullptr))
    return;

  const unsigned selected = picker.GetValue();
  if (selected == TIME_PICKER_AUTO) {
    model->ResumeTimeAuto();
  } else if (selected == TIME_PICKER_NOW && time_plausible) {
    model->SetTimeAutoAdvance(false);
    model->SetCurrentTimeIndex(unsigned(basic.date_time_utc.hour));
  } else {
    model->SetTimeAutoAdvance(false);
    model->SetCurrentTimeIndex(selected);
  }

  model->ApplyCurrentSelectionToMap();
  model->SaveCursorSession();
  Notify(ControlsUpdate::OVERLAY);
#endif
}

bool
XcthermControlsModel::SupportsSecondaryAutoAdvance() const noexcept
{
  return true;
}

bool
XcthermControlsModel::GetSecondaryAutoAdvance() const noexcept
{
#ifdef HAVE_HTTP
  if (model != nullptr)
    return model->IsAltitudeAutoActive();
#endif
  return true;
}

void
XcthermControlsModel::SetSecondaryAutoAdvance(bool auto_advance) noexcept
{
#ifdef HAVE_HTTP
  if (model != nullptr)
    model->SetAltitudeAutoAdvance(auto_advance, CommonInterface::Basic());
#else
  (void)auto_advance;
#endif
}

void
XcthermControlsModel::ResumePrimaryAuto() noexcept
{
#ifdef HAVE_HTTP
  if (model != nullptr)
    model->ResumeTimeAuto();
#endif
  Notify(ControlsUpdate::OVERLAY);
}

void
XcthermControlsModel::ResumeSecondaryAuto() noexcept
{
#ifdef HAVE_HTTP
  if (model != nullptr)
    model->ResumeLayerAuto(CommonInterface::Basic());
#endif
  Notify(ControlsUpdate::OVERLAY);
}

void
XcthermControlsModel::OnGPSUpdate(const MoreData &basic) noexcept
{
#ifdef HAVE_HTTP
  if (model != nullptr)
    model->OnGPSUpdate(basic);
#else
  (void)basic;
#endif
  Notify(ControlsUpdate::LABELS);
}

void
XcthermControlsModel::RefreshOverlay() noexcept
{
#ifdef HAVE_HTTP
  if (model != nullptr)
    model->ApplyCurrentSelectionToMap();
#endif
  ActionInterface::SendUIState(true);
}

} // namespace WeatherMapOverlay
