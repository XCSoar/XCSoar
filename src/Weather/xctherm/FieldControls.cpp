// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FieldControls.hpp"

#include "Dialogs/ComboPicker.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "NMEA/Info.hpp"
#include "PageSettings.hpp"
#include "UIState.hpp"
#include "Weather/MapOverlay/ControlsWidget.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"
#include "Weather/MapOverlay/PageCursor.hpp"
#include "Weather/MapOverlay/TimePicker.hpp"
#include "XCThermAPI.hpp"
#include "XCThermCatalog.hpp"
#include "XCThermForecastTime.hpp"
#include "XCThermMapOverlay.hpp"
#include "util/Compiler.h"

namespace XCTherm {

static bool
HasManualLayer(const PageLayout &page,
               const RegionDef &region) noexcept
{
  return page.xctherm_layer >= 0 &&
    unsigned(page.xctherm_layer) < region.layer_count;
}

static bool
LayerUsableForAuto(const Layer &layer) noexcept
{
  const auto &api = XCThermAPI::Instance();
  if (!api.GetCachedHours(layer.api_parameter).empty())
    return true;

  for (const auto &parameter : api.GetAvailableParameters())
    if (parameter.name == layer.api_parameter)
      return true;

  return false;
}

static int
ResolveAutoLayer(const RegionDef &region) noexcept
{
  const auto &basic = CommonInterface::Basic();
  double altitude = -1;
  if (basic.gps_altitude_available)
    altitude = basic.gps_altitude;
  else if (basic.baro_altitude_available)
    altitude = basic.baro_altitude;

  if (altitude < 0)
    return -1;

  int selected = -1;
  unsigned previous_altitude = 0;
  for (unsigned i = 0; i < region.layer_count; ++i) {
    const auto &layer = region.layers[i];
    if (layer.is_agl || !LayerUsableForAuto(layer))
      continue;

    if (selected < 0) {
      selected = int(i);
      previous_altitude = layer.altitude_m;
      continue;
    }

    const double threshold =
      (double(previous_altitude) + double(layer.altitude_m)) / 2;
    if (altitude < threshold)
      break;

    selected = int(i);
    previous_altitude = layer.altitude_m;
  }

  return selected;
}

static unsigned
ResolveLayer(const PageLayout &page) noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = GetRegion(settings.model);
  if (region.layer_count == 0)
    return 0;

  if (HasManualLayer(page, region))
    return unsigned(page.xctherm_layer);

  const int automatic = ResolveAutoLayer(region);
  if (automatic >= 0)
    return unsigned(automatic);

  const int active = FindActiveLayerIndex(settings);
  if (active >= 0)
    return unsigned(active);

  const auto &cursor =
    CommonInterface::GetUIState().weather.xctherm_cursor;
  return cursor.layer < region.layer_count ? cursor.layer : 0;
}

static unsigned
ResolveTime(const PageLayout &page) noexcept
{
  if (page.xctherm_time >= 0 && page.xctherm_time < 24)
    return unsigned(page.xctherm_time);

  const auto utc = GetUtcTimeParts();
  return PickAutoTargetUtcHour(utc.hour, utc.minute);
}

void
ApplyCursorFromPageLayout(const PageLayout &page) noexcept
{
  auto &weather = CommonInterface::SetUIState().weather;
  auto &cursor = weather.xctherm_cursor;

  cursor.layer = ResolveLayer(page);
  cursor.forecast_utc_hour = ResolveTime(page);
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  cursor.altitude_manual_override =
    HasManualLayer(page, GetRegion(settings.model));
  cursor.time_manual_override =
    page.xctherm_time != PageLayout::XCTHERM_TIME_AUTO;
  weather.xctherm.cursor_initialized = true;
}

void
PersistCursorToPage(unsigned page_index) noexcept
{
  WeatherMapOverlay::MutateOverlayPage(
    page_index, PageLayout::Overlay::XCTHERM,
    [](PageLayout &page) noexcept {
      const auto &cursor =
        CommonInterface::GetUIState().weather.xctherm_cursor;
      const int layer = cursor.altitude_manual_override
        ? int(cursor.layer)
        : PageLayout::XCTHERM_LAYER_AUTO;
      const int time = cursor.time_manual_override
        ? int(cursor.forecast_utc_hour % 24)
        : PageLayout::XCTHERM_TIME_AUTO;
      if (page.xctherm_layer == layer && page.xctherm_time == time)
        return false;

      page.xctherm_layer = layer;
      page.xctherm_time = time;
      return true;
    });
}

void
PersistCursorToCurrentPage() noexcept
{
  PersistCursorToPage(CommonInterface::GetUIState().pages.current_index);
}

static void
AfterLiveChange(const PageLayout &page) noexcept
{
  ApplyCursorFromPageLayout(page);
  ApplyCursorOverlayFromSession();
  WeatherMapOverlay::NotifyLiveCursorChange();
}

bool
EditTimeOnLayout(PageLayout &page) noexcept
{
  const auto &xctherm =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = GetRegion(xctherm.model);
  if (region.layer_count == 0)
    return false;

  const unsigned layer = ResolveLayer(page);
  DataFieldEnum field;
  const auto &api = XCThermAPI::Instance();
  for (unsigned hour = 0; hour < 24; ++hour) {
    StaticString<20> label;
    label.Format("%02u:00 %s", hour,
                 api.IsLayerCached(region.layers[layer].api_parameter,
                                   hour)
                   ? "[x]"
                   : "[ ]");
    field.addEnumText(label.c_str(), hour);
  }
  field.SetValue(ResolveTime(page));

  const ComboList combo_list = field.CreateComboList(nullptr);
  StaticString<64> caption;
  caption.Format("%s %s (UTC)", "XCTherm", _("Time"));
  const auto result =
    WeatherMapOverlay::RunTimePicker(caption.c_str(), combo_list);

  switch (result.selection) {
  case WeatherMapOverlay::TimePickerSelection::CANCEL:
    return false;

  case WeatherMapOverlay::TimePickerSelection::AUTO:
    page.xctherm_time = PageLayout::XCTHERM_TIME_AUTO;
    break;

  case WeatherMapOverlay::TimePickerSelection::NOW: {
    const auto utc = GetUtcTimeParts();
    /* Pick the current UTC hour (same as selecting that row). */
    page.xctherm_time =
      int(PickAutoTargetUtcHour(utc.hour, utc.minute));
    break;
  }

  case WeatherMapOverlay::TimePickerSelection::MANUAL: {
    const unsigned hour = combo_list[result.manual_index].int_value;
    if (hour >= 24)
      return false;
    page.xctherm_time = int(hour);
    break;
  }

  case WeatherMapOverlay::TimePickerSelection::COUNT:
    gcc_unreachable();
  }

  return true;
}

void
OpenTimePicker() noexcept
{
  const unsigned page_index =
    CommonInterface::GetUIState().pages.current_index;
  if (!WeatherMapOverlay::MutateOverlayPage(
        page_index, PageLayout::Overlay::XCTHERM,
        [](PageLayout &page) noexcept {
          return EditTimeOnLayout(page);
        }))
    return;

  AfterLiveChange(
    CommonInterface::GetUISettings().pages.pages[page_index]);
}

LayerPickerResult
EditLayerOnLayout(PageLayout &page, bool offer_setup) noexcept
{
  const auto &xctherm =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = GetRegion(xctherm.model);
  if (region.layer_count == 0)
    return LayerPickerResult::NONE;

  DataFieldEnum field;
  field.AddChoice(-1, _("Auto"));
  for (unsigned i = 0; i < region.layer_count; ++i)
    field.addEnumText(gettext(region.layers[i].short_label), int(i));

  field.SetValue(HasManualLayer(page, region)
                 ? unsigned(page.xctherm_layer)
                 : unsigned(-1));

  bool setup = false;
  const char *setup_caption = offer_setup ? _("Setup") : nullptr;
  StaticString<64> caption;
  caption.Format("%s %s", "XCTherm", _("Altitude"));
  if (!ComboPicker(caption.c_str(), field, nullptr,
                   setup_caption, &setup))
    return setup ? LayerPickerResult::OPEN_SETUP
                 : LayerPickerResult::NONE;

  const unsigned value = field.GetValue();
  const int selected = value == unsigned(-1) ? -1 : int(value);
  if (selected >= int(region.layer_count))
    return LayerPickerResult::NONE;

  page.xctherm_layer = selected;
  return LayerPickerResult::CHANGED;
}

LayerPickerResult
OpenLayerPicker(bool offer_setup) noexcept
{
  LayerPickerResult result = LayerPickerResult::NONE;
  const unsigned page_index =
    CommonInterface::GetUIState().pages.current_index;
  if (!WeatherMapOverlay::MutateOverlayPage(
        page_index, PageLayout::Overlay::XCTHERM,
        [offer_setup, &result](PageLayout &page) noexcept {
          result = EditLayerOnLayout(page, offer_setup);
          return result == LayerPickerResult::CHANGED;
        }))
    return result;

  AfterLiveChange(
    CommonInterface::GetUISettings().pages.pages[page_index]);
  return result;
}

void
FormatTimeLabelForPage(StaticString<64> &text,
                       const PageLayout &page) noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = GetRegion(settings.model);
  if (region.layer_count == 0) {
    text.clear();
    return;
  }

  const unsigned layer = ResolveLayer(page);
  const unsigned hour = ResolveTime(page);
  StaticString<64> base;
  XCTherm::FormatTimeLabel(
    base, region.layers[layer].api_parameter, hour,
    page.xctherm_time == PageLayout::XCTHERM_TIME_AUTO);

  WeatherMapOverlay::AssignLabeledData(
    text, base,
    XCThermAPI::Instance().IsLayerCached(
      region.layers[layer].api_parameter, hour));
}

void
FormatLayerLabelForPage(StaticString<64> &text,
                        const PageLayout &page) noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = GetRegion(settings.model);
  if (region.layer_count == 0) {
    text.clear();
    return;
  }

  const unsigned layer = ResolveLayer(page);
  StaticString<64> base;
  WeatherMapOverlay::FormatAutoNamedLabel(
    base, !HasManualLayer(page, region),
    gettext(region.layers[layer].short_label));

  WeatherMapOverlay::AssignLabeledData(
    text, base,
    XCThermAPI::Instance().IsLayerCached(
      region.layers[layer].api_parameter, ResolveTime(page)));
}

} // namespace XCTherm
