// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XcthermControlsModel.hpp"

#include "ActionInterface.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"

#ifdef HAVE_HTTP
#include "Weather/xctherm/XCThermControlsModel.hpp"
#endif

namespace WeatherMapOverlay {

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
