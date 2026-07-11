// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XcthermControlsModel.hpp"

#include "Dialogs/ComboPicker.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "PrimaryTimePicker.hpp"
#include "Form/DataField/Enum.hpp"
#include "util/StaticString.hxx"
#include "Weather/MapOverlay/CursorBarLabels.hpp"
#include "Weather/xctherm/XCThermCatalog.hpp"

#include <algorithm>

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
  const bool time_plausible =
    CommonInterface::Basic().date_time_utc.IsPlausible();

  WithBackend([&](XCTherm::XCThermControlsModel &backend) noexcept {
    const auto persist = [this, &backend]() noexcept {
      backend.ApplyCurrentSelectionToMap();
      backend.SaveCursorSession();
      NotifyOverlay();
    };

    StaticString<64> caption;
    caption.Format("%s %s (UTC)", "XCTherm", _("Time"));

    OpenPrimaryTimePicker(*this, caption.c_str(),
      [&backend](DataFieldEnum &field) noexcept {
        field.ClearChoices();

        const auto &cached_hours = backend.GetCachedHours();
        for (unsigned hour = 0; hour < 24; ++hour) {
          StaticString<20> label;
          const bool has_data =
            std::find(cached_hours.begin(), cached_hours.end(), hour) !=
            cached_hours.end();
          label.Format("%02u:00 %s", hour, has_data ? "[x]" : "[ ]");
          field.addEnumText(label.c_str(), hour);
        }
      },
      [&backend]() noexcept {
        return backend.GetCurrentForecastHour();
      },
      [](ControlsModel &model) noexcept {
        model.EnablePrimaryAutoFromInput();
      },
      [&backend, &persist](ControlsModel &) noexcept {
        const auto &basic = CommonInterface::Basic();
        if (!basic.date_time_utc.IsPlausible())
          return;

        backend.SetTimeAutoAdvance(false);
        backend.SetCurrentTimeIndex(unsigned(basic.date_time_utc.hour));
        persist();
      },
      [&backend, &persist](ControlsModel &, unsigned hour) noexcept {
        backend.SetTimeAutoAdvance(false);
        backend.SetCurrentTimeIndex(hour);
        persist();
      },
      time_plausible);
  });
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

void
XcthermControlsModel::OpenSecondaryPicker() noexcept
{
#ifdef HAVE_HTTP
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = XCTherm::GetRegion(settings.model);
  if (region.layer_count == 0)
    return;

  DataFieldEnum field;
  unsigned current_layer = 0;

  WithBackend([&](const XCTherm::XCThermControlsModel &backend) noexcept {
    current_layer = backend.GetCurrentLayer();
  });

  for (unsigned i = 0; i < region.layer_count; ++i) {
    field.addEnumText(gettext(region.layers[i].short_label), int(i));
  }

  field.SetValue(int(current_layer));

  if (!ComboPicker(_("XCTherm Altitude"), field, nullptr))
    return;

  const int selected = field.GetValue();
  if (selected < 0 || unsigned(selected) >= region.layer_count)
    return;

  const auto &basic = CommonInterface::Basic();
  WithBackend([&](XCTherm::XCThermControlsModel &backend) noexcept {
    backend.SetAltitudeAutoAdvance(false, basic);
    backend.SetCurrentLayer(unsigned(selected));
    backend.ApplyCurrentSelectionToMap();
    backend.SaveCursorSession();
  });

  Notify(ControlsUpdate::OVERLAY);
#else
  (void)0;
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
