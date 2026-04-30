// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermControlsWidget.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_HTTP

#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Interface.hpp"
#include "Form/Button.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "ui/window/PaintWindow.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Weather/xctherm/XCThermGeoJSON.hpp"
#include "Weather/xctherm/XCThermGeoJSONOverlay.hpp"
#include "Weather/xctherm/XCThermAutoSwitch.hpp"
#include "Weather/Settings.hpp"
#include "LogFile.hpp"
#include "util/StaticString.hxx"

#include <fstream>
#include <sstream>
#include <memory>

namespace {

struct LayerDef {
  const char *short_label;
  const char *file_suffix;
  unsigned altitude_m;
  bool is_agl;
};

/* Available layers matching CH model — keep in sync with XCThermDialog.cpp */
static constexpr LayerDef LAYERS[] = {
  { "1500m AMSL", "1500amsl", 1500, false },
  { "2000m AMSL", "2000amsl", 2000, false },
  { "3000m AMSL", "3000amsl", 3000, false },
  { "4000m AMSL", "4000amsl", 4000, false },
  { "5000m AMSL", "5000amsl", 5000, false },
  { "6000m AMSL", "6000amsl", 6000, false },
  { "7000m AMSL", "7000amsl", 7000, false },
  { "8000m AMSL", "8000amsl", 8000, false },
  { "100m AGL",   "100agl",   100,  true },
  { "400m AGL",   "400agl",   400,  true },
};

static constexpr unsigned N_LAYERS = std::size(LAYERS);

/* Available forecast hours (placeholder until API) */
static constexpr unsigned FORECAST_HOURS[] = {
  6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18
};
static constexpr unsigned N_HOURS = std::size(FORECAST_HOURS);

static const std::string BASE_PATH =
  "/Users/pheinrich/Documents/Fliegen/xctherm/xcsoar_wave/geojson_example/";

/**
 * Check if a GeoJSON file exists for the given layer suffix.
 */
static bool
IsLayerFileAvailable(const char *suffix)
{
  StaticString<256> filename;
  filename.Format("vertical_wind_%s.geojson", suffix);
  std::string file_path = BASE_PATH + filename.c_str();
  std::ifstream file(file_path);
  return file.is_open();
}

/**
 * Try to load a GeoJSON file for the given layer suffix and
 * set it as the map overlay.
 */
static void
LoadLayerOverlay(const char *suffix, const char *label)
{
  auto *map = UIGlobals::GetMap();
  if (map == nullptr)
    return;

  StaticString<256> filename;
  filename.Format("vertical_wind_%s.geojson", suffix);
  std::string file_path = BASE_PATH + filename.c_str();

  std::ifstream file(file_path);
  if (!file.is_open()) {
    LogFmt("xctherm controls: file not found: {}", file_path);
    return;
  }

  LogFmt("xctherm controls: loading {}", file_path);

  std::ostringstream ss;
  ss << file.rdbuf();
  file.close();
  std::string content = ss.str();

  auto forecast = XCThermGeoJSON::Parse(content, true);
  if (forecast.IsEmpty())
    return;

  forecast.layer_name = label;

  auto overlay = std::make_unique<XCThermGeoJSONOverlay>();
  overlay->SetForecast(std::move(forecast), label);
  map->SetOverlay(std::move(overlay));
}

} // anonymous namespace

/**
 * A label that draws centered text.
 */
class XCThermLabel final : public PaintWindow {
  const DialogLook &look;
  StaticString<64> text{""};

public:
  explicit XCThermLabel(const DialogLook &_look) noexcept
    :look(_look) {}

  void SetText(const char *_text) noexcept {
    text = _text;
    if (IsDefined())
      Invalidate();
  }

protected:
  void OnPaint(Canvas &canvas) noexcept override {
    const auto rc = GetClientRect();
    canvas.Clear(COLOR_WHITE);
    canvas.SetBackgroundTransparent();
    canvas.SetTextColor(is_available ? COLOR_BLACK : COLOR_GRAY);
    canvas.Select(look.text_font);

    const auto text_size = canvas.CalcTextSize(text);
    const int x = (rc.GetWidth() - (int)text_size.width) / 2;
    const int y = (rc.GetHeight() - (int)text_size.height) / 2;
    canvas.DrawText({x, y}, text);
  }

private:
  bool is_available = true;

public:
  void SetAvailable(bool avail) noexcept {
    is_available = avail;
    if (IsDefined())
      Invalidate();
  }
};

/**
 * Container window with two rows and a separator line:
 *   Row 1: < layer_label >    (altitude layer stepper)
 *   ------- separator -------
 *   Row 2: < time_label  >    (forecast time stepper)
 */
class XCThermControlsWidget::ControlsWindow final : public ContainerWindow {
  const DialogLook &look;
  static constexpr int SEPARATOR_H = 1;

  unsigned current_layer = 4; /* default: 5000m AMSL */
  unsigned current_time = 6;  /* default: 12:00 UTC (index 6 = 12h) */
  bool layer_available[N_LAYERS] = {};
  bool time_available[N_HOURS] = {};

  /* Row 1: layer */
  Button layer_prev, layer_next;
  XCThermLabel layer_label;

  /* Row 2: time */
  Button time_prev, time_next;
  XCThermLabel time_label;

  /* Auto-switch manager */
  XCThermAutoSwitch auto_switch;

public:
  explicit ControlsWindow(const DialogLook &_look) noexcept
    :look(_look), layer_label(_look), time_label(_look) {}

  void Create(ContainerWindow &parent, const PixelRect &rc) noexcept {
    WindowStyle style;
    style.Hide();
    style.ControlParent();
    ContainerWindow::Create(parent, rc, style);

    /* Check file availability */
    ScanAvailableLayers();

    const int total_h = rc.GetHeight();
    const int row_h = (total_h - SEPARATOR_H) / 2;
    const int row2_y = row_h + SEPARATOR_H;
    const int btn_w = row_h * 2;
    const int w = (int)rc.GetWidth();

    WindowStyle child_style;

    /* Row 1: Layer stepper */
    layer_prev.Create(*this, look.button, "<",
                      {0, 0, btn_w, row_h},
                      child_style, [this]{ ManualStepLayer(-1); });
    layer_next.Create(*this, look.button, ">",
                      {w - btn_w, 0, w, row_h},
                      child_style, [this]{ ManualStepLayer(+1); });
    {
      WindowStyle ls;
      layer_label.Create(*this, {btn_w, 0, w - btn_w, row_h}, ls);
    }

    /* Row 2: Time stepper */
    time_prev.Create(*this, look.button, "<",
                     {0, row2_y, btn_w, row2_y + row_h},
                     child_style, [this]{ ManualStepTime(-1); });
    time_next.Create(*this, look.button, ">",
                     {w - btn_w, row2_y, w, row2_y + row_h},
                     child_style, [this]{ ManualStepTime(+1); });
    {
      WindowStyle ts;
      time_label.Create(*this, {btn_w, row2_y, w - btn_w, row2_y + row_h}, ts);
    }

    /* Initialize auto-switch */
    InitAutoSwitch();

    UpdateLayerLabel();
    UpdateTimeLabel();
  }

  void ManualStepLayer(int delta) noexcept {
    auto_switch.OnManualLayerStep();

    int next = (int)current_layer + delta;
    if (next < 0) next = (int)N_LAYERS - 1;
    if (next >= (int)N_LAYERS) next = 0;
    current_layer = (unsigned)next;

    LoadLayerOverlay(LAYERS[current_layer].file_suffix,
                     LAYERS[current_layer].short_label);
    UpdateLayerLabel();
  }

  void ManualStepTime(int delta) noexcept {
    auto_switch.OnManualTimeStep();

    int next = (int)current_time + delta;
    if (next < 0) next = (int)N_HOURS - 1;
    if (next >= (int)N_HOURS) next = 0;
    current_time = (unsigned)next;

    UpdateTimeLabel();
    /* TODO: load the time-specific forecast when API is available */
  }

  /**
   * Called periodically to run auto-switch logic.
   */
  void TimerUpdate() noexcept {
    const auto &settings =
      CommonInterface::GetComputerSettings().weather.xctherm;
    auto_switch.SetEnabled(settings.auto_switch);

    if (!auto_switch.IsEnabled())
      return;

    /* Get current flight data */
    const auto &basic = CommonInterface::Basic();
    double gps_alt = basic.gps_altitude_available
      ? basic.gps_altitude : -1.0;
    double baro_alt = basic.baro_altitude_available
      ? basic.baro_altitude : -1.0;

    /* Get UTC time */
    unsigned utc_hour = 12, utc_minute = 0;
    if (basic.date_time_utc.IsPlausible()) {
      utc_hour = basic.date_time_utc.hour;
      utc_minute = basic.date_time_utc.minute;
    }

    auto_switch.Update(gps_alt, baro_alt, utc_hour, utc_minute,
                       basic.time);
  }

  void LayoutChildren(const PixelRect &rc) noexcept {
    const int total_h = rc.GetHeight();
    const int row_h = (total_h - SEPARATOR_H) / 2;
    const int row2_y = row_h + SEPARATOR_H;
    const int btn_w = row_h * 2;
    const int w = rc.GetWidth();

    /* Row 1 */
    if (layer_prev.IsDefined())
      layer_prev.Move({0, 0, btn_w, row_h});
    if (layer_next.IsDefined())
      layer_next.Move({w - btn_w, 0, w, row_h});
    if (layer_label.IsDefined())
      layer_label.Move({btn_w, 0, w - btn_w, row_h});

    /* Row 2 */
    if (time_prev.IsDefined())
      time_prev.Move({0, row2_y, btn_w, row2_y + row_h});
    if (time_next.IsDefined())
      time_next.Move({w - btn_w, row2_y, w, row2_y + row_h});
    if (time_label.IsDefined())
      time_label.Move({btn_w, row2_y, w - btn_w, row2_y + row_h});
  }

private:
  void ScanAvailableLayers() noexcept {
    for (unsigned i = 0; i < N_LAYERS; ++i)
      layer_available[i] = IsLayerFileAvailable(LAYERS[i].file_suffix);

    /* No time-specific files yet — mark all as unavailable */
    for (unsigned i = 0; i < N_HOURS; ++i)
      time_available[i] = false;
  }

  void InitAutoSwitch() noexcept {
    /* Register only available layers */
    std::vector<XCThermAutoSwitch::LayerInfo> layers;
    for (unsigned i = 0; i < N_LAYERS; ++i) {
      if (layer_available[i])
        layers.push_back({i, LAYERS[i].altitude_m, LAYERS[i].is_agl});
    }
    auto_switch.SetLoadedLayers(std::move(layers));
    auto_switch.SetCurrentLayerPos((int)current_layer);

    /* Register available times */
    std::vector<unsigned> times;
    for (unsigned i = 0; i < N_HOURS; ++i) {
      if (time_available[i])
        times.push_back(FORECAST_HOURS[i]);
    }
    auto_switch.SetLoadedTimes(std::move(times));
    auto_switch.SetCurrentTimePos((int)current_time);

    /* Set callbacks */
    auto_switch.SetLayerSwitchCallback([this](unsigned layer_index) {
      if (layer_index < N_LAYERS && layer_available[layer_index]) {
        current_layer = layer_index;
        LoadLayerOverlay(LAYERS[current_layer].file_suffix,
                         LAYERS[current_layer].short_label);
        UpdateLayerLabel();
      }
    });

    auto_switch.SetTimeSwitchCallback([this](unsigned time_pos) {
      if (time_pos < N_HOURS) {
        current_time = time_pos;
        UpdateTimeLabel();
      }
    });

    const auto &settings =
      CommonInterface::GetComputerSettings().weather.xctherm;
    auto_switch.SetEnabled(settings.auto_switch);
  }

  void UpdateLayerLabel() noexcept {
    StaticString<64> text;
    bool avail = layer_available[current_layer];

    if (!avail)
      text.Format("%s  [N/A]", LAYERS[current_layer].short_label);
    else if (auto_switch.IsAltitudeAutoActive())
      text.Format("AUTO: %s", LAYERS[current_layer].short_label);
    else
      text.Format("%s", LAYERS[current_layer].short_label);

    layer_label.SetText(text);
    layer_label.SetAvailable(avail);
  }

  void UpdateTimeLabel() noexcept {
    StaticString<64> text;
    bool avail = time_available[current_time];

    if (!avail)
      text.Format("%02u:00 UTC  [N/A]", FORECAST_HOURS[current_time]);
    else if (auto_switch.IsTimeAutoActive())
      text.Format("AUTO: %02u:00 UTC", FORECAST_HOURS[current_time]);
    else
      text.Format("%02u:00 UTC", FORECAST_HOURS[current_time]);

    time_label.SetText(text);
    time_label.SetAvailable(avail);
  }

protected:
  void OnPaint(Canvas &canvas) noexcept override {
    ContainerWindow::OnPaint(canvas);

    /* Draw separator line between rows */
    const int total_h = (int)GetClientRect().GetHeight();
    const int row_h = (total_h - SEPARATOR_H) / 2;
    const int w = (int)GetClientRect().GetWidth();

    canvas.SelectBlackPen();
    canvas.DrawLine({0, row_h}, {w, row_h});
  }
};

PixelSize
XCThermControlsWidget::GetMinimumSize() const noexcept
{
  return {100, 80};
}

PixelSize
XCThermControlsWidget::GetMaximumSize() const noexcept
{
  return {4096, 80};
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
  auto &w = static_cast<ControlsWindow &>(GetWindow());
  w.Move(rc);
  w.LayoutChildren({0, 0, (int)rc.GetWidth(), (int)rc.GetHeight()});

  WindowWidget::Show(rc);
}

void
XCThermControlsWidget::Hide() noexcept
{
  WindowWidget::Hide();
}

#endif /* HAVE_HTTP */
