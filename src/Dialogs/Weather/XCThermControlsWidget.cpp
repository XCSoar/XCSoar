// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermControlsWidget.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_HTTP

#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Interface.hpp"
#include "Form/Button.hpp"
#include "Renderer/SymbolButtonRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/window/SolidContainerWindow.hpp"
#include "ui/window/PaintWindow.hpp"
#include "Weather/xctherm/XCThermControlsModel.hpp"
#include "Weather/xctherm/XCThermDownloadJob.hpp"
#include "Weather/xctherm/XCThermForecastTime.hpp"
#include "Weather/xctherm/XCThermMapOverlay.hpp"
#include "Weather/xctherm/XCThermAutoSwitch.hpp"
#include "Weather/xctherm/XCThermAPI.hpp"
#include "Weather/xctherm/XCThermCatalog.hpp"
#include "Weather/Settings.hpp"
#include "Asset.hpp"
#include "Language/Language.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/StaticString.hxx"

#include <chrono>
#include <cstdlib>

class XCThermLabel final : public PaintWindow {
  const DialogLook &look;
  StaticString<64> text{""};
  bool is_available = true;

public:
  explicit XCThermLabel(const DialogLook &_look) noexcept
    :look(_look) {}

  void SetText(const char *_text) noexcept {
    text = _text;
    if (IsDefined())
      Invalidate();
  }

  void SetAvailable(bool avail) noexcept {
    is_available = avail;
    if (IsDefined())
      Invalidate();
  }

protected:
  void OnPaint(Canvas &canvas) noexcept override {
    const auto rc = GetClientRect();
    canvas.SetBackgroundTransparent();
    canvas.SetTextColor(is_available
                        ? look.text_color
                        : look.button.disabled.color);
    canvas.Select(look.text_font);

    const auto text_size = canvas.CalcTextSize(text);
    const int x = (rc.GetWidth() - (int)text_size.width) / 2;
    const int y = (rc.GetHeight() - (int)text_size.height) / 2;
    canvas.DrawText({x, y}, text);
  }
};

class XCThermControlsWidget::ControlsWindow final : public SolidContainerWindow {
  const DialogLook &look;
  static constexpr int SEPARATOR_H = 1;

  XCTherm::XCThermControlsModel model;
  XCThermAutoSwitch auto_switch;

  Button layer_prev, layer_next;
  XCThermLabel layer_label;
  Button time_prev, time_next;
  XCThermLabel time_label;

  [[gnu::pure]]
  const XCTherm::RegionDef &RegionLayers() const noexcept {
    const auto &settings =
      CommonInterface::GetComputerSettings().weather.xctherm;
    return XCTherm::GetRegion(settings.model);
  }

  [[gnu::pure]]
  const XCTherm::Layer &LayerAt(unsigned index) const noexcept {
    return RegionLayers().layers[index];
  }

  void RefreshLabels() noexcept {
    UpdateLayerLabel();
    UpdateTimeLabel();
  }

  void InitAutoSwitch() noexcept {
    auto_switch.SetLoadedLayers(model.BuildAutoSwitchLayers());
    auto_switch.SyncCurrentLayerIndex(model.GetCurrentLayer());
    auto_switch.SetLoadedTimes(model.GetAvailableHours());
    if (!model.GetAvailableHours().empty())
      auto_switch.SetCurrentTimePos(0);

    auto_switch.SetLayerSwitchCallback([this](unsigned layer_index) {
      if (!model.LayerUsableForAutoSwitch(layer_index))
        return;

      model.SetCurrentLayer(layer_index);
      model.RefreshCachedHours();
      if (!model.GetCachedHours().empty()) {
        const auto &hours = model.GetCachedHours();
        const unsigned ti = model.GetCurrentTimeIndex() < hours.size()
          ? model.GetCurrentTimeIndex() : 0;
        model.ApplyLayerToMap(layer_index, hours[ti]);
      }
      else if (int(XCTherm::FindActiveLayerIndex(
                 CommonInterface::GetComputerSettings().weather.xctherm)) ==
               int(layer_index))
        model.MaybeFetchActiveLayer(
          [this](std::shared_ptr<XCThermDownloadJob> job) {
            model.OnDownloadFinished(job);
            RefreshLabels();
          });

      auto_switch.SyncCurrentLayerIndex(model.GetCurrentLayer());
      UpdateLayerLabel();
    });

    auto_switch.SetEnabled(
      CommonInterface::GetComputerSettings().weather.xctherm.auto_switch);
  }

public:
  explicit ControlsWindow(const DialogLook &_look) noexcept
    :look(_look), layer_label(_look), time_label(_look) {}

  static int CalcButtonWidth(int row_h) noexcept {
    return HasTouchScreen() ? row_h * 3 : row_h * 2;
  }

  static int CursorBarHeight() noexcept {
    return HasTouchScreen() ? 110 : 80;
  }

  static void
  ComputeCursorBarLayout(const PixelRect &rc,
                         int &total_h, int &row_h, int &row2_y,
                         int &btn_w, int &w) noexcept
  {
    w = std::max(1, (int)rc.GetWidth());
    total_h = std::max(SEPARATOR_H + 2,
                       std::min((int)rc.GetHeight(), CursorBarHeight()));
    row_h = std::max(1, (total_h - SEPARATOR_H) / 2);
    row2_y = row_h + SEPARATOR_H;
    btn_w = CalcButtonWidth(row_h);
    const int max_btn_w = std::max(1, w / 2);
    if (btn_w > max_btn_w)
      btn_w = max_btn_w;
  }

  void Create(ContainerWindow &parent, const PixelRect &rc) noexcept {
    WindowStyle style;
    style.Hide();
    style.ControlParent();
    SolidContainerWindow::Create(parent, rc, look.background_color, style);
    SetGradientTopColor(look.background_gradient_top_color);

    int total_h, row_h, row2_y, btn_w, w;
    ComputeCursorBarLayout(rc, total_h, row_h, row2_y, btn_w, w);
    (void)total_h;

    WindowStyle child_style;

    layer_prev.Create(*this, {0, 0, btn_w, row_h}, child_style,
                      std::make_unique<SymbolButtonRenderer>(look.button, "<"),
                      [this]{ OnStepLayer(-1); });
    layer_next.Create(*this, {w - btn_w, 0, w, row_h}, child_style,
                      std::make_unique<SymbolButtonRenderer>(look.button, ">"),
                      [this]{ OnStepLayer(+1); });
    {
      WindowStyle ls;
      layer_label.Create(*this, {btn_w, 0, w - btn_w, row_h}, ls);
    }

    time_prev.Create(*this, {0, row2_y, btn_w, row2_y + row_h}, child_style,
                    std::make_unique<SymbolButtonRenderer>(look.button, "<"),
                    [this]{ OnStepTime(-1); });
    time_next.Create(*this, {w - btn_w, row2_y, w, row2_y + row_h},
                    child_style,
                    std::make_unique<SymbolButtonRenderer>(look.button, ">"),
                    [this]{ OnStepTime(+1); });
    {
      WindowStyle ts;
      time_label.Create(*this, {btn_w, row2_y, w - btn_w, row2_y + row_h}, ts);
    }

    model.BootstrapSession();
    InitAutoSwitch();
    model.RequestBackgroundIndex([this]{ OnIndexReady(); });
    model.MaybeFetchActiveLayer(
      [this](std::shared_ptr<XCThermDownloadJob> job) {
        model.OnDownloadFinished(job);
        RefreshLabels();
      });
    RefreshLabels();
  }

  void OnStepLayer(int delta) noexcept {
    auto_switch.OnManualLayerStep();
    if (!model.StepLayer(delta)) {
      layer_label.SetText(_("No data – use Info → Weather"));
      layer_label.SetAvailable(false);
      return;
    }
    auto_switch.SyncCurrentLayerIndex(model.GetCurrentLayer());
    UpdateLayerLabel();
  }

  void OnStepTime(int delta) noexcept {
    auto_switch.OnManualTimeStep();
    if (!model.StepTime(delta)) {
      time_label.SetText(_("No data – use Info → Weather"));
      time_label.SetAvailable(false);
      return;
    }
    UpdateTimeLabel();
  }

  void TimerUpdate() noexcept {
    const auto &settings =
      CommonInterface::GetComputerSettings().weather.xctherm;
    auto_switch.SetEnabled(settings.auto_switch);

    model.SyncActiveLayerFromSettings();
    UpdateLayerLabel();

    if (!auto_switch.IsEnabled())
      return;

    const auto &basic = CommonInterface::Basic();
    double gps_alt = basic.gps_altitude_available
      ? basic.gps_altitude : -1.0;
    double baro_alt = basic.baro_altitude_available
      ? basic.baro_altitude : -1.0;

    unsigned utc_hour = 12, utc_minute = 0;
    if (basic.date_time_utc.IsPlausible()) {
      utc_hour = basic.date_time_utc.hour;
      utc_minute = basic.date_time_utc.minute;
    }

    auto_switch.Update(gps_alt, baro_alt, utc_hour, utc_minute, basic.time);

    if (auto_switch.IsTimeAutoActive()) {
      model.RefreshCachedHours();
      const int desired = XCTherm::PickAutoTimeIndex(
        model.GetCachedHours(), utc_hour, utc_minute);
      if (desired >= 0 &&
          unsigned(desired) != model.GetCurrentTimeIndex()) {
        model.SetCurrentTimeIndex(unsigned(desired));
        model.ApplyLayerToMap(model.GetCurrentLayer(),
                              model.GetCachedHours()[unsigned(desired)]);
      }
      UpdateTimeLabel();
    }
  }

  void RestoreOverlayFromCache() noexcept {
    model.RefreshCachedHours();
    model.ApplyCurrentSelectionToMap();
  }

  void LayoutChildren(const PixelRect &rc) noexcept {
    int total_h, row_h, row2_y, btn_w, w;
    ComputeCursorBarLayout(rc, total_h, row_h, row2_y, btn_w, w);
    (void)total_h;

    if (layer_prev.IsDefined())
      layer_prev.Move({0, 0, btn_w, row_h});
    if (layer_next.IsDefined())
      layer_next.Move({w - btn_w, 0, w, row_h});
    if (layer_label.IsDefined())
      layer_label.Move({btn_w, 0, w - btn_w, row_h});

    if (time_prev.IsDefined())
      time_prev.Move({0, row2_y, btn_w, row2_y + row_h});
    if (time_next.IsDefined())
      time_next.Move({w - btn_w, row2_y, w, row2_y + row_h});
    if (time_label.IsDefined())
      time_label.Move({btn_w, row2_y, w - btn_w, row2_y + row_h});
  }

private:
  void OnIndexReady() noexcept {
    model.OnIndexLoaded();
    InitAutoSwitch();
    RefreshLabels();
  }

  void UpdateLayerLabel() noexcept {
    const unsigned layer = model.GetCurrentLayer();
    const bool has_cache = model.HasCacheAtCurrentHour(layer);

    StaticString<80> text;
    if (!has_cache)
      text.Format("%s  %s", LayerAt(layer).short_label, _("[no data]"));
    else if (auto_switch.IsAltitudeAutoActive())
      text.Format("%s %s", _("AUTO:"), LayerAt(layer).short_label);
    else
      text.Format("%s", LayerAt(layer).short_label);

    layer_label.SetText(text);
    layer_label.SetAvailable(has_cache);
  }

  void UpdateTimeLabel() noexcept {
    model.RefreshCachedHours();
    const auto &cached_hours = model.GetCachedHours();

    if (cached_hours.empty()) {
      time_label.SetText(_("No forecast – download first"));
      time_label.SetAvailable(false);
      return;
    }

    const unsigned fcast_h = cached_hours[
      model.GetCurrentTimeIndex() < cached_hours.size()
      ? model.GetCurrentTimeIndex() : 0];

    int offset_min = 0;
    bool has_real_offset = false;
    auto &api = XCThermAPI::Instance();
    const auto slice = api.GetSliceMeta(
      LayerAt(model.GetCurrentLayer()).api_parameter, fcast_h);

    if (slice.has_value() && slice->run_date.size() == 8 &&
        slice->run_hour.size() == 2 &&
        BrokenDateTime::NowUTC().IsPlausible()) {
      const unsigned year =
        (unsigned)std::atoi(slice->run_date.substr(0, 4).c_str());
      const unsigned month =
        (unsigned)std::atoi(slice->run_date.substr(4, 2).c_str());
      const unsigned day =
        (unsigned)std::atoi(slice->run_date.substr(6, 2).c_str());
      const unsigned run_h =
        (unsigned)std::atoi(slice->run_hour.c_str());
      const BrokenDateTime run_dt(year, month, day, run_h, 0, 0);
      const BrokenDateTime forecast_dt =
        run_dt + std::chrono::hours{slice->step};
      const auto delta = forecast_dt - BrokenDateTime::NowUTC();
      offset_min = (int)std::chrono::duration_cast<std::chrono::minutes>(
        delta).count();
      has_real_offset = true;
    } else if (CommonInterface::Basic().date_time_utc.IsPlausible()) {
      const auto &basic = CommonInterface::Basic();
      int cur_min = (int)basic.date_time_utc.hour * 60
                  + (int)basic.date_time_utc.minute;
      int fc_min = (int)fcast_h * 60;
      offset_min = fc_min - cur_min;
      if (offset_min < 0)
        offset_min += 1440;
      has_real_offset = true;
    }

    char offset_buf[16] = {0};
    if (has_real_offset) {
      const int abs_min = std::abs(offset_min);
      std::snprintf(offset_buf, sizeof(offset_buf),
                    "%s%d:%02d",
                    offset_min >= 0 ? "+" : "-",
                    abs_min / 60, abs_min % 60);
    }

    StaticString<64> text;
    if (auto_switch.IsTimeAutoActive())
      text.Format("%s %02u:00 UTC (%s)", _("AUTO:"), fcast_h, offset_buf);
    else
      text.Format("%02u:00 UTC (%s)", fcast_h, offset_buf);

    time_label.SetText(text);
    time_label.SetAvailable(true);
  }

protected:
  void OnResize(PixelSize new_size) noexcept override {
    ContainerWindow::OnResize(new_size);

    if (layer_prev.IsDefined())
      LayoutChildren(GetClientRect());
  }

  void OnPaint(Canvas &canvas) noexcept override {
    SolidContainerWindow::OnPaint(canvas);

    int total_h, row_h, row2_y, btn_w, w;
    ComputeCursorBarLayout(GetClientRect(), total_h, row_h, row2_y, btn_w, w);
    (void)total_h;
    (void)row2_y;
    (void)btn_w;

    const Pen separator_pen(1, look.ReadOnlyValueBorderColor());
    canvas.Select(separator_pen);
    canvas.DrawLine({0, row_h}, {w, row_h});
  }
};

PixelSize
XCThermControlsWidget::GetMinimumSize() const noexcept
{
  const unsigned h = HasTouchScreen() ? 110U : 80U;
  return PixelSize{100U, h};
}

PixelSize
XCThermControlsWidget::GetMaximumSize() const noexcept
{
  const unsigned h = HasTouchScreen() ? 110U : 80U;
  return PixelSize{4096U, h};
}

void
XCThermControlsWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  auto w = std::make_unique<ControlsWindow>(look);
  w->Create(parent, rc);
  SetWindow(std::move(w));
}

void
XCThermControlsWidget::Unprepare() noexcept
{
  WindowWidget::Unprepare();
}

void
XCThermControlsWidget::Show(const PixelRect &rc) noexcept
{
  WindowWidget::Show(rc);

  CommonInterface::GetLiveBlackboard().AddListener(*this);

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  if (settings.overlay_location ==
      XCThermSettings::OverlayLocation::SEPARATE_MAP) {
    auto &w = static_cast<ControlsWindow &>(GetWindow());
    w.RestoreOverlayFromCache();
  }
}

void
XCThermControlsWidget::Move(const PixelRect &rc) noexcept
{
  WindowWidget::Move(rc);

  auto &w = static_cast<ControlsWindow &>(GetWindow());
  if (w.IsDefined())
    w.LayoutChildren({0, 0, (int)rc.GetWidth(), (int)rc.GetHeight()});
}

void
XCThermControlsWidget::Hide() noexcept
{
  CommonInterface::GetLiveBlackboard().RemoveListener(*this);

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  if (settings.overlay_location ==
      XCThermSettings::OverlayLocation::SEPARATE_MAP) {
    XCTherm::ClearMapOverlay();
  }

  WindowWidget::Hide();
}

void
XCThermControlsWidget::OnGPSUpdate([[maybe_unused]] const MoreData &basic)
{
  static_cast<ControlsWindow &>(GetWindow()).TimerUpdate();
}

#endif /* HAVE_HTTP */
