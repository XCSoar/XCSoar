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
#include "MapWindow/GlueMapWindow.hpp"
#include "Weather/xctherm/XCThermGeoJSON.hpp"
#include "Weather/xctherm/XCThermGeoJSONOverlay.hpp"
#include "Weather/xctherm/XCThermAutoSwitch.hpp"
#include "Weather/xctherm/XCThermAPI.hpp"
#include "Weather/xctherm/XCThermCatalog.hpp"
#include "Weather/Settings.hpp"
#include "Asset.hpp"
#include "Language/Language.hpp"
#include "time/BrokenDateTime.hpp"
#include "LogFile.hpp"
#include "util/StaticString.hxx"

#include <algorithm>
#include <cstdlib>
#include <memory>

namespace {

/**
 * Apply a GeoJSON string as the map overlay.
 *
 * @param parameter API parameter (e.g. "vertical_wind_5000amsl") so the
 *   overlay can resolve download/run metadata for the map-item popup.
 * @param forecast_utc the forecast's valid UTC hour.
 */
static void
ApplyGeoJSONOverlay(const std::string &geojson, const char *label,
                    const char *parameter = nullptr,
                    unsigned forecast_utc = 0)
{
  auto *map = UIGlobals::GetMap();
  if (map == nullptr)
    return;

  auto forecast = XCThermGeoJSON::Parse(geojson, true);
  if (forecast.IsEmpty()) {
    LogFmt("xctherm: parse failed for {}", label);
    return;
  }

  forecast.layer_name = label;

  auto overlay = std::make_unique<XCThermGeoJSONOverlay>();
  overlay->SetForecast(std::move(forecast), label, parameter, forecast_utc);
  map->SetOverlay(std::move(overlay));
}

} // anonymous namespace

/**
 * A label that draws centered text with availability coloring.
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
class XCThermControlsWidget::ControlsWindow final : public SolidContainerWindow {
  const DialogLook &look;
  static constexpr int SEPARATOR_H = 1;

  unsigned current_layer = 0;
  unsigned current_time_index = 0;
  bool layer_available[XCTherm::MaxLayerCount()] = {};

  /* Cached forecast hours for the current layer (from API cache) */
  std::vector<unsigned> cached_hours;
  /* All available hours from index.json (kept for auto-switch) */
  std::vector<unsigned> available_hours;

  /* Row 1: layer */
  Button layer_prev, layer_next;
  XCThermLabel layer_label;

  /* Row 2: time */
  Button time_prev, time_next;
  XCThermLabel time_label;

  /* Auto-switch manager */
  XCThermAutoSwitch auto_switch;

  /* Track if we've loaded the index */
  bool index_loaded = false;

  /**
   * Index into LAYERS of the active layer the last time
   * SyncCurrentLayerFromSettings() actually wrote @c current_layer.
   * -1 means "no sync yet" — first call always proceeds.
   *
   * Used by the GPS-tick path to skip re-syncing when nothing about the
   * dialog's Activate setting has changed; otherwise the per-tick sync
   * would silently undo manual ◀/▶ stepping and auto-switch decisions.
   */
  int last_synced_active_layer = -1;

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

public:
  explicit ControlsWindow(const DialogLook &_look) noexcept
    :look(_look), layer_label(_look), time_label(_look) {}

  /**
   * Cursor-bar layout helper. On touch devices the user taps these
   * with a finger; ~2× row height isn't enough hit area for accurate
   * thumb operation in turbulence, so we widen by 50% when XCSoar
   * reports a touch screen (Android/iOS/Kobo).
   *
   * On mouse-driven platforms (Linux desktop, macOS sim, Windows) the
   * narrower buttons leave more space for the label text.
   */
  [[gnu::pure]]
  static int CalcButtonWidth(int row_h) noexcept {
    return HasTouchScreen() ? row_h * 3 : row_h * 2;
  }

  [[gnu::pure]]
  static int CursorBarHeight() noexcept {
    return HasTouchScreen() ? 110 : 80;
  }

  /**
   * Compute child control geometry for the two-row cursor bar.
   *
   * MainWindow::Prepare() passes the full main map rectangle (tall and
   * often narrow between InfoBoxes). Button width must be derived from
   * the bar's real height (~80–110 px), not the full main height, and
   * must not exceed half the available width.
   */
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

    /* Row 1: Layer stepper */
    layer_prev.Create(*this, {0, 0, btn_w, row_h}, child_style,
                      std::make_unique<SymbolButtonRenderer>(look.button, "<"),
                      [this]{ ManualStepLayer(-1); });
    layer_next.Create(*this, {w - btn_w, 0, w, row_h}, child_style,
                      std::make_unique<SymbolButtonRenderer>(look.button, ">"),
                      [this]{ ManualStepLayer(+1); });
    {
      WindowStyle ls;
      layer_label.Create(*this, {btn_w, 0, w - btn_w, row_h}, ls);
    }

    /* Row 2: Time stepper */
    time_prev.Create(*this, {0, row2_y, btn_w, row2_y + row_h}, child_style,
                    std::make_unique<SymbolButtonRenderer>(look.button, "<"),
                    [this]{ ManualStepTime(-1); });
    time_next.Create(*this, {w - btn_w, row2_y, w, row2_y + row_h},
                    child_style,
                    std::make_unique<SymbolButtonRenderer>(look.button, ">"),
                    [this]{ ManualStepTime(+1); });
    {
      WindowStyle ts;
      time_label.Create(*this, {btn_w, row2_y, w - btn_w, row2_y + row_h}, ts);
    }

    /* Initialize from API */
    InitFromAPI();

    UpdateLayerLabel();
    UpdateTimeLabel();
  }

  void ManualStepLayer(int delta) noexcept {
    auto_switch.OnManualLayerStep();

    auto &api = XCThermAPI::Instance();

    /* Collect layer indices that have cached data */
    std::vector<unsigned> cached;
    for (unsigned i = 0; i < RegionLayers().layer_count; ++i) {
      if (api.IsLayerCached(LayerAt(i).api_parameter,
                             GetCurrentForecastHour()))
        cached.push_back(i);
    }

    if (cached.empty()) {
      layer_label.SetText(_("No data – use Info → Weather"));
      layer_label.SetAvailable(false);
      return;
    }

    /* Find position of current_layer in cached list, then step */
    auto it = std::find(cached.begin(), cached.end(), current_layer);
    int pos = (it != cached.end()) ? (int)(it - cached.begin()) : 0;
    pos += delta;
    if (pos < 0) pos = (int)cached.size() - 1;
    if (pos >= (int)cached.size()) pos = 0;
    current_layer = cached[pos];

    /* Apply from cache — no download */
    ApplyCachedLayer(current_layer, GetCurrentForecastHour());
    UpdateLayerLabel();
  }

  void ManualStepTime(int delta) noexcept {
    auto_switch.OnManualTimeStep();

    auto &api = XCThermAPI::Instance();
    const std::string &param = LayerAt(current_layer).api_parameter;

    /* Collect hours cached for current layer */
    cached_hours = api.GetCachedHours(param);

    if (cached_hours.empty()) {
      time_label.SetText(_("No data – use Info → Weather"));
      time_label.SetAvailable(false);
      return;
    }

    int next = (int)current_time_index + delta;
    if (next < 0) next = (int)cached_hours.size() - 1;
    if (next >= (int)cached_hours.size()) next = 0;
    current_time_index = (unsigned)next;

    /* Apply from cache */
    ApplyCachedLayer(current_layer, cached_hours[current_time_index]);
    UpdateTimeLabel();
  }

  /**
   * Pick the cached-hour index that should be shown in auto-time mode,
   * following the XCTHERM.md rule:
   *
   *   - Before :45 of the current UTC hour, display the forecast valid
   *     for this hour (utc_h).
   *   - At/after :45, display the forecast for the next hour (utc_h+1).
   *
   * The selected hour must never be in the past. If the exact target
   * hour is not cached, prefer the smallest cached hour ≥ target
   * (within 12 h forward). Only if there is no future hour at all do
   * we fall back to the latest cached past hour.
   *
   * @return index into @p cached, or -1 if @p cached is empty.
   */
  static int
  PickAutoTimeIndex(const std::vector<unsigned> &cached,
                    unsigned utc_h, unsigned utc_min) noexcept {
    if (cached.empty())
      return -1;

    const unsigned target = (utc_min >= 45) ? (utc_h + 1) % 24 : utc_h;

    int best_future = -1;
    unsigned best_future_dist = 25;
    int best_past = -1;
    unsigned best_past_dist = 25;

    for (size_t i = 0; i < cached.size(); ++i) {
      const unsigned fwd = (cached[i] + 24 - target) % 24;
      if (fwd <= 12) {
        /* future (0 = exact match) */
        if (fwd < best_future_dist) {
          best_future_dist = fwd;
          best_future = (int)i;
        }
      } else {
        const unsigned back = 24 - fwd;
        if (back < best_past_dist) {
          best_past_dist = back;
          best_past = (int)i;
        }
      }
    }

    return best_future >= 0 ? best_future : best_past;
  }

  /**
   * Called periodically (driven by GPS updates) to run auto-switch
   * logic for both altitude and time.
   */
  void TimerUpdate() noexcept {
    const auto &settings =
      CommonInterface::GetComputerSettings().weather.xctherm;
    auto_switch.SetEnabled(settings.auto_switch);

    /* Keep the widget's notion of the current layer in sync with the
       dialog's Activate state and with whichever layer has cached
       data. Cheap; runs once per GPS tick. */
    SyncCurrentLayerFromSettings();
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

    /* Altitude auto-switch still flows through XCThermAutoSwitch
       (midpoint thresholds + 20 s hysteresis). */
    auto_switch.Update(gps_alt, baro_alt, utc_hour, utc_minute,
                       basic.time);

    /* Time auto-switch: keep the displayed forecast hour at the
       current/next hour per the :45 rule, and never let it lag into
       the past as time progresses. */
    if (auto_switch.IsTimeAutoActive()) {
      RefreshCachedHours();
      const int desired = PickAutoTimeIndex(cached_hours,
                                            utc_hour, utc_minute);
      if (desired >= 0 && (unsigned)desired != current_time_index) {
        current_time_index = (unsigned)desired;
        ApplyCachedLayer(current_layer, cached_hours[current_time_index]);
      }
      UpdateTimeLabel();
    }
  }

  /**
   * Re-apply the cached overlay for the currently selected layer / time
   * to the map. Used by the outer widget on Show() when SEPARATE_MAP
   * mode is active, so the forecast reappears when the XCTherm page
   * comes back into view.
   */
  void RestoreOverlayFromCache() noexcept {
    auto &api = XCThermAPI::Instance();
    RefreshCachedHours();
    if (cached_hours.empty())
      return;
    const unsigned hour = cached_hours[
      current_time_index < cached_hours.size() ? current_time_index : 0];
    const std::string &cached =
      api.GetCachedGeoJSON(LayerAt(current_layer).api_parameter, hour);
    if (!cached.empty())
      ApplyGeoJSONOverlay(cached, LayerAt(current_layer).short_label,
                          LayerAt(current_layer).api_parameter, hour);
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
  /**
   * Sync @c current_layer with the active layer set by the dialog
   * (Info → Weather → XCTherm → Activate).
   *
   * Idempotent across GPS ticks: it only writes to @c current_layer
   * when the dialog's active-layer setting has actually changed since
   * the last sync. Otherwise it returns immediately, so manual ◀/▶
   * stepping on the cursor bar — and auto-switch decisions from
   * XCThermAutoSwitch — are preserved between settings changes.
   *
   * On a real change (or the first call) we also pick the layer with
   * cached data as a fallback if the active layer happens to have
   * nothing cached, so the cursor bar can display *something*.
   */
  void SyncCurrentLayerFromSettings() noexcept {
    const auto &settings =
      CommonInterface::GetComputerSettings().weather.xctherm;
    auto &api = XCThermAPI::Instance();

    /* Find which LAYERS row matches the dialog's current Activate
       state. -1 if no row matches (shouldn't happen for valid
       settings, but be defensive). */
    int active_layer = -1;
    for (unsigned i = 0; i < RegionLayers().layer_count; ++i) {
      if (XCTherm::IsActiveLayer(LayerAt(i), settings.parameter,
                                 settings.wave_height,
                                 settings.vertical_wind_agl)) {
        active_layer = (int)i;
        break;
      }
    }

    /* No change since last sync → leave current_layer alone. This is
       the path that runs on every GPS tick once the user has settled
       on a layer; without it, manual ◀/▶ would be undone ~1 s later. */
    if (active_layer == last_synced_active_layer)
      return;

    last_synced_active_layer = active_layer;

    if (active_layer >= 0) {
      current_layer = (unsigned)active_layer;
      /* Active layer has cached data → done. */
      if (!api.GetCachedHours(
            LayerAt(active_layer).api_parameter).empty())
        return;
    }

    /* Active layer has no cache (or no active layer found) — fall
       through to any cached layer so the cursor bar can usefully
       display *something* instead of "no data". */
    for (unsigned i = 0; i < RegionLayers().layer_count; ++i) {
      if (!api.GetCachedHours(LayerAt(i).api_parameter).empty()) {
        current_layer = i;
        return;
      }
    }
  }

  void InitFromAPI() noexcept {
    auto &api = XCThermAPI::Instance();

    /* Idempotent — first call wires up the persistent disk cache and
       reloads anything from previous sessions (within 24 h TTL). */
    api.EnableDiskCache();

    const auto &settings =
      CommonInterface::GetComputerSettings().weather.xctherm;
    api.SetCredentials(settings.credentials.email.c_str(),
                       settings.credentials.password.c_str());
    api.SetModel(RegionLayers().api_slug);

    /* Fetch index.json only if not yet loaded.

       FetchIndex now throws on network / HTTP failure — we MUST catch
       here because this method is noexcept (the widget is constructed
       during page setup and an escape would terminate the whole app).
       On any failure we silently fall into "offline mode" — the user
       just sees the layer list as unavailable, can still open the
       dialog where errors are surfaced properly via ShowError. */
    if (!api.IsIndexLoaded()) {
      try {
        api.FetchIndex();
      } catch (const std::exception &e) {
        LogFmt("xctherm: index fetch failed in controls widget: {}",
               e.what());
      } catch (...) {
        LogFmt("xctherm: index fetch failed in controls widget (unknown)");
      }
    }

    /* Repopulate layer_available from whatever the API currently knows
       about — every Prepare(), regardless of who fetched the index.
       This used to live inside the `if (!IsIndexLoaded())` block, so
       on a page reload triggered after the dialog had already loaded
       the index, layer_available stayed at its default-init all-false
       state and the cursor showed "no data" even with a full cache. */
    index_loaded = api.IsIndexLoaded();
    if (index_loaded) {
      const auto &params = api.GetAvailableParameters();
      for (unsigned i = 0; i < RegionLayers().layer_count; ++i) {
        layer_available[i] = false;
        for (const auto &p : params)
          if (p.name == LayerAt(i).api_parameter) {
            layer_available[i] = true;
            break;
          }
      }
      if (layer_available[current_layer])
        available_hours = api.GetAvailableForecastHours(
          LayerAt(current_layer).api_parameter);
    } else {
      LogFmt("xctherm: index not loaded — controls in offline mode");
      for (unsigned i = 0; i < RegionLayers().layer_count; ++i)
        layer_available[i] = false;
    }

    {
      const int def = XCTherm::FindLayerIndex(
        XCTherm::ToRegion(settings.model), 5000, false);
      if (def >= 0)
        current_layer = unsigned(def);
    }

    /* Sync current_layer with the dialog-activated layer / any layer
       that actually has cached data — fixes the "no data" message that
       used to appear when the user downloaded a layer other than the
       widget's default 5000 m AMSL. */
    SyncCurrentLayerFromSettings();

    /* Populate cached_hours from the download cache for current layer */
    RefreshCachedHours();

    /* Pick the best default time index (closest to current UTC) */
    SelectBestTimeIndex();

    InitAutoSwitch();
  }

  /**
   * Refresh cached_hours from the API cache for the current layer.
   */
  void RefreshCachedHours() noexcept {
    auto &api = XCThermAPI::Instance();
    cached_hours = api.GetCachedHours(LayerAt(current_layer).api_parameter);
  }

  /**
   * Get the UTC hour of the currently displayed forecast.
   */
  unsigned GetCurrentForecastHour() const noexcept {
    if (current_time_index < cached_hours.size())
      return cached_hours[current_time_index];
    if (!available_hours.empty())
      return available_hours[0];
    return 12;
  }

  /**
   * Select the initial time index using the same future-preferred rule
   * as the auto-switch loop (see PickAutoTimeIndex).
   */
  void SelectBestTimeIndex() noexcept {
    if (cached_hours.empty())
      return;

    const auto &basic = CommonInterface::Basic();
    unsigned utc_h = 12, utc_min = 0;
    if (basic.date_time_utc.IsPlausible()) {
      utc_h = basic.date_time_utc.hour;
      utc_min = basic.date_time_utc.minute;
    }

    const int idx = PickAutoTimeIndex(cached_hours, utc_h, utc_min);
    if (idx >= 0)
      current_time_index = (unsigned)idx;
  }

  /**
   * Apply a layer from the API cache (no download).
   */
  void ApplyCachedLayer(unsigned layer_index, unsigned utc_hour) noexcept {
    auto &api = XCThermAPI::Instance();
    const std::string &cached =
      api.GetCachedGeoJSON(LayerAt(layer_index).api_parameter, utc_hour);
    if (!cached.empty())
      ApplyGeoJSONOverlay(cached, LayerAt(layer_index).short_label,
                          LayerAt(layer_index).api_parameter, utc_hour);
    else
      LogFmt("xctherm: cache miss {}@{}h",
             LayerAt(layer_index).api_parameter, utc_hour);
  }


  void InitAutoSwitch() noexcept {
    std::vector<XCThermAutoSwitch::LayerInfo> layers;
    for (unsigned i = 0; i < RegionLayers().layer_count; ++i) {
      if (layer_available[i])
        layers.push_back({i, LayerAt(i).altitude_m, LayerAt(i).is_agl});
    }
    auto_switch.SetLoadedLayers(std::move(layers));
    auto_switch.SetCurrentLayerPos((int)current_layer);

    auto_switch.SetLoadedTimes(available_hours);
    if (!available_hours.empty())
      auto_switch.SetCurrentTimePos(0);

    auto_switch.SetLayerSwitchCallback([this](unsigned layer_index) {
      if (layer_index < RegionLayers().layer_count && layer_available[layer_index]) {
        current_layer = layer_index;
        DownloadAndApplyLayer(current_layer);
        UpdateLayerLabel();
      }
    });

    /* Time auto-switch is driven directly by TimerUpdate() picking the
       correct cached hour every GPS tick; we don't register a time-switch
       callback on XCThermAutoSwitch because that path would also try to
       download on a miss (UI-thread blocking is unacceptable in flight). */

    const auto &settings =
      CommonInterface::GetComputerSettings().weather.xctherm;
    auto_switch.SetEnabled(settings.auto_switch);
  }

  /**
   * Download a layer via API and apply as overlay.
   * Uses the current time selection.
   */
  void DownloadAndApplyLayer(unsigned layer_index) noexcept {
    RefreshCachedHours();
    if (!cached_hours.empty()) {
      ApplyCachedLayer(layer_index,
                       cached_hours[current_time_index < cached_hours.size()
                                    ? current_time_index : 0]);
    } else {
      LogFmt("xctherm: no cached data for layer {}",
             LayerAt(layer_index).short_label);
    }
  }

  void UpdateLayerLabel() noexcept {
    auto &api = XCThermAPI::Instance();
    const unsigned fcast_h = GetCurrentForecastHour();
    const bool has_cache = api.IsLayerCached(
      LayerAt(current_layer).api_parameter, fcast_h);

    StaticString<80> text;
    if (!has_cache)
      text.Format("%s  %s", LayerAt(current_layer).short_label,
                  _("[no data]"));
    else if (auto_switch.IsAltitudeAutoActive())
      text.Format("%s %s", _("AUTO:"),
                  LayerAt(current_layer).short_label);
    else
      text.Format("%s", LayerAt(current_layer).short_label);

    layer_label.SetText(text);
    layer_label.SetAvailable(has_cache);
  }

  void UpdateTimeLabel() noexcept {
    RefreshCachedHours();

    if (cached_hours.empty()) {
      time_label.SetText(_("No forecast – download first"));
      time_label.SetAvailable(false);
      return;
    }

    const unsigned fcast_h = cached_hours[
      current_time_index < cached_hours.size() ? current_time_index : 0];

    /* Compute the real signed offset against current UTC using the
       slice's absolute datetime (run_date + run_hour + step) — needed
       because a span longer than 12 h crosses midnight, and the old
       ±12 h modular clamp would flip "tomorrow 02:00" to "-11:15"
       instead of "+12:45". */
    int offset_min = 0;
    bool has_real_offset = false;
    auto &api = XCThermAPI::Instance();
    const auto slice = api.GetSliceMeta(
      LayerAt(current_layer).api_parameter, fcast_h);

    if (slice.has_value() && slice->run_date.size() == 8 &&
        slice->run_hour.size() == 2 &&
        BrokenDateTime::NowUTC().IsPlausible()) {
      const unsigned year  = (unsigned)std::atoi(slice->run_date.substr(0, 4).c_str());
      const unsigned month = (unsigned)std::atoi(slice->run_date.substr(4, 2).c_str());
      const unsigned day   = (unsigned)std::atoi(slice->run_date.substr(6, 2).c_str());
      const unsigned run_h = (unsigned)std::atoi(slice->run_hour.c_str());
      const BrokenDateTime run_dt(year, month, day, run_h, 0, 0);
      const BrokenDateTime forecast_dt =
        run_dt + std::chrono::hours{slice->step};
      const auto delta = forecast_dt - BrokenDateTime::NowUTC();
      offset_min = (int)std::chrono::duration_cast<std::chrono::minutes>(delta).count();
      has_real_offset = true;
    } else if (CommonInterface::Basic().date_time_utc.IsPlausible()) {
      /* Fallback (no slice metadata yet): use the same-day approximation
         so we still show *something*. Only really matters for a brief
         window during the first download. */
      const auto &basic = CommonInterface::Basic();
      int cur_min = (int)basic.date_time_utc.hour * 60
                  + (int)basic.date_time_utc.minute;
      int fc_min  = (int)fcast_h * 60;
      offset_min  = fc_min - cur_min;
      if (offset_min < 0) offset_min += 1440;
      has_real_offset = true;
    }

    char offset_buf[16] = {0};
    if (has_real_offset) {
      const int abs_min = std::abs(offset_min);
      const int oh = abs_min / 60;
      const int om = abs_min % 60;
      std::snprintf(offset_buf, sizeof(offset_buf),
                    "%s%d:%02d",
                    offset_min >= 0 ? "+" : "-", oh, om);
    }

    /* Issued-time / run timestamp intentionally omitted — too much info
       for the small bottom cursor. The full run is still visible in the
       Info → Weather → XCTherm dialog row. */
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

    /* Subtle row divider — same border tone as read-only form fields. */
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

/* Touch devices need a taller strip so the buttons + two label rows
   stay readable when scaled up for finger taps. On mouse-driven
   platforms we keep the lean 80 px to leave more screen for the map. */

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

  /* Listen for GPS updates so the auto-switch logic actually runs.
     Without this, the widget would only reflect state from Prepare(). */
  CommonInterface::GetLiveBlackboard().AddListener(*this);

  /* In SEPARATE_MAP mode, the overlay is bound to this widget's
     visibility (Hide() clears it). Restore it now so opening the
     XCTherm page re-shows the forecast from cache. */
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

  /* If the overlay is gated to this page, drop it on hide so it
     doesn't appear on the regular map pages. */
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  if (settings.overlay_location ==
      XCThermSettings::OverlayLocation::SEPARATE_MAP) {
    if (auto *map = UIGlobals::GetMap())
      map->SetOverlay(nullptr);
  }

  WindowWidget::Hide();
}

void
XCThermControlsWidget::OnGPSUpdate([[maybe_unused]] const MoreData &basic)
{
  static_cast<ControlsWindow &>(GetWindow()).TimerUpdate();
}

#endif /* HAVE_HTTP */
