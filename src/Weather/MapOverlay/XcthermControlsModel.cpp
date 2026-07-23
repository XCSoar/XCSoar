// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XcthermControlsModel.hpp"

#include "Interface.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"
#include "Weather/xctherm/FieldControls.hpp"

namespace WeatherMapOverlay {

void
XcthermControlsModel::OnShow() noexcept
{
#ifdef HAVE_HTTP
  WithBackend([&](XCTherm::XCThermControlsModel &backend) noexcept {
    if (!prepared) {
      backend.Prepare([this]() noexcept {
        Notify(ControlsUpdate::LABELS);
      });
      prepared = true;
    }

    backend.OnShow();
  });
#endif
}

void
XcthermControlsModel::OnHide() noexcept
{
#ifdef HAVE_HTTP
  WithBackend([&](XCTherm::XCThermControlsModel &backend) noexcept {
    backend.OnHide();
  });
#endif
}

void
XcthermControlsModel::FormatPrimaryLabel(StaticString<64> &text) const noexcept
{
#ifdef HAVE_HTTP
  text.clear();
  WithBackend([&](XCTherm::XCThermControlsModel &backend) noexcept {
    backend.FormatTimeLabel(text);
  });
  if (text.empty())
    text = "XCTherm";
#else
  text = "XCTherm";
#endif
}

void
XcthermControlsModel::FormatSecondaryLabel(StaticString<64> &text) const noexcept
{
#ifdef HAVE_HTTP
  text.clear();
  WithBackend([&](const XCTherm::XCThermControlsModel &backend) noexcept {
    StaticString<80> layer_text;
    backend.FormatLayerLabel(layer_text);
    text = layer_text;
  });
  if (text.empty())
    text = NoForecastHint();
#else
  text = NoForecastHint();
#endif
}

bool
XcthermControlsModel::HasPrimaryData() const noexcept
{
#ifdef HAVE_HTTP
  bool has_data = false;
  WithBackend([&](const XCTherm::XCThermControlsModel &backend) noexcept {
    has_data = backend.HasCacheAtCurrentHour(backend.GetCurrentLayer());
  });
  return has_data;
#else
  return false;
#endif
}

bool
XcthermControlsModel::HasSecondaryData() const noexcept
{
  return HasPrimaryData();
}

bool
XcthermControlsModel::StepPrimary(int delta) noexcept
{
#ifdef HAVE_HTTP
  bool stepped = true;
  WithBackend([&](XCTherm::XCThermControlsModel &backend) noexcept {
    stepped = backend.StepTime(delta);
  });
  return stepped;
#else
  (void)delta;
  return true;
#endif
}

bool
XcthermControlsModel::StepSecondary(int delta) noexcept
{
#ifdef HAVE_HTTP
  bool stepped = true;
  WithBackend([&](XCTherm::XCThermControlsModel &backend) noexcept {
    stepped = backend.StepLayer(delta);
  });
  return stepped;
#else
  (void)delta;
  return true;
#endif
}

bool
XcthermControlsModel::GetPrimaryAutoAdvance() const noexcept
{
#ifdef HAVE_HTTP
  bool active = true;
  WithBackend([&](const XCTherm::XCThermControlsModel &backend) noexcept {
    active = backend.IsTimeAutoActive();
  });
  return active;
#else
  return true;
#endif
}

void
XcthermControlsModel::SetPrimaryAutoAdvance(bool auto_advance) noexcept
{
#ifdef HAVE_HTTP
  WithBackend([&](XCTherm::XCThermControlsModel &backend) noexcept {
    backend.SetTimeAutoAdvance(auto_advance);
  });
#else
  (void)auto_advance;
#endif
}

void
XcthermControlsModel::ApplyPrimaryAutoAdvance() noexcept
{
}

void
XcthermControlsModel::EnablePrimaryAutoFromInput() noexcept
{
#ifdef HAVE_HTTP
  WithBackend([&](XCTherm::XCThermControlsModel &backend) noexcept {
    backend.ResumeTimeAuto();
    backend.ApplyCurrentSelectionToMap();
    backend.SaveCursorSession();
  });
#endif
  Notify(ControlsUpdate::OVERLAY);
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
  XCTherm::OpenTimePicker();
  WithBackend([&](XCTherm::XCThermControlsModel &backend) noexcept {
    backend.SyncCursorFromSession();
  });
#endif
  NotifyOverlay();
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
  bool active = true;
  WithBackend([&](const XCTherm::XCThermControlsModel &backend) noexcept {
    active = backend.IsAltitudeAutoActive();
  });
  return active;
#else
  return true;
#endif
}

void
XcthermControlsModel::SetSecondaryAutoAdvance(bool auto_advance) noexcept
{
#ifdef HAVE_HTTP
  const auto &basic = CommonInterface::Basic();
  WithBackend([&](XCTherm::XCThermControlsModel &backend) noexcept {
    backend.SetAltitudeAutoAdvance(auto_advance, basic);
  });
#else
  (void)auto_advance;
#endif
}

SecondaryLabelAction
XcthermControlsModel::GetSecondaryLabelAction() const noexcept
{
  return SecondaryLabelAction::OPEN_PICKER;
}

SecondaryPickerResult
XcthermControlsModel::OpenSecondaryPicker() noexcept
{
#ifdef HAVE_HTTP
  return HandleSecondaryFieldPicker(XCTherm::OpenLayerPicker(true),
                                    [this] {
    WithBackend([&](XCTherm::XCThermControlsModel &backend) noexcept {
      backend.SyncCursorFromSession();
    });
    Notify(ControlsUpdate::OVERLAY);
  });
#else
  return SecondaryPickerResult::NONE;
#endif
}

void
XcthermControlsModel::ResumePrimaryAuto() noexcept
{
  if (GetPrimaryAutoAdvance())
    return;

  EnablePrimaryAutoFromInput();
}

void
XcthermControlsModel::ResumeSecondaryAuto() noexcept
{
#ifdef HAVE_HTTP
  const auto &basic = CommonInterface::Basic();
  WithBackend([&](XCTherm::XCThermControlsModel &backend) noexcept {
    backend.ResumeLayerAuto(basic);
  });
#endif
  Notify(ControlsUpdate::OVERLAY);
}

void
XcthermControlsModel::OnGPSUpdate(const MoreData &basic) noexcept
{
#ifdef HAVE_HTTP
  WithBackend([&](XCTherm::XCThermControlsModel &backend) noexcept {
    backend.OnGPSUpdate(basic);
  });
#else
  (void)basic;
#endif
  Notify(ControlsUpdate::LABELS);
}

void
XcthermControlsModel::RefreshOverlay() noexcept
{
#ifdef HAVE_HTTP
  WithBackend([&](XCTherm::XCThermControlsModel &backend) noexcept {
    backend.ApplyCurrentSelectionToMap();
  });
#endif
}

} // namespace WeatherMapOverlay
