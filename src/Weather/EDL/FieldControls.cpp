// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FieldControls.hpp"

#include "Dialogs/ComboPicker.hpp"
#include "Form/DataField/Enum.hpp"
#include "Glue.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Levels.hpp"
#include "PageSettings.hpp"
#include "StateController.hpp"
#include "TileStore.hpp"
#include "UIState.hpp"
#include "Weather/MapOverlay/ControlsWidget.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"
#include "Weather/MapOverlay/PageCursor.hpp"
#include "Weather/MapOverlay/TimePicker.hpp"
#include "system/FileUtil.hpp"
#include "util/Compiler.h"
#include "util/StaticString.hxx"

#include <array>
#include <chrono>
#include <limits>

namespace EDL {

static constexpr unsigned FORECAST_CHOICES = FORECAST_HOURS_PER_DAY;

static int
EncodePageTime(bool auto_advance, BrokenDateTime forecast) noexcept
{
  if (auto_advance)
    return PageLayout::EDL_TIME_AUTO;

  if (!forecast.IsPlausible())
    return PageLayout::EDL_TIME_NOW;

  const auto hours = std::chrono::duration_cast<std::chrono::hours>(
    forecast.ToTimePoint().time_since_epoch()).count();
  if (hours < 0 || hours > std::numeric_limits<int>::max())
    return PageLayout::EDL_TIME_AUTO;

  return int(hours);
}

static int
EncodePageTime(const EDLWeatherUIState &edl) noexcept
{
  return EncodePageTime(edl.forecast_auto_advance, edl.forecast_datetime);
}

static BrokenDateTime
DecodePageTime(int value) noexcept
{
  if (value < 0)
    return BrokenDateTime::Invalid();

  return BrokenDateTime::FromUnixTime(int64_t(value) * 60 * 60)
    .FloorToHour();
}

void
ApplyTimeFromPageLayout(const PageLayout &layout) noexcept
{
  auto &edl = CommonInterface::SetUIState().weather.edl;

  if (layout.edl_time == PageLayout::EDL_TIME_AUTO) {
    edl.forecast_datetime = BrokenDateTime::Invalid();
    edl.forecast_auto_advance = true;
  } else if (layout.edl_time == PageLayout::EDL_TIME_NOW) {
    edl.forecast_datetime = BrokenDateTime::Invalid();
    edl.forecast_auto_advance = false;
  } else {
    const BrokenDateTime decoded = DecodePageTime(layout.edl_time);
    if (decoded.IsPlausible()) {
      edl.forecast_datetime = decoded;
      edl.forecast_auto_advance = false;
    } else {
      edl.forecast_datetime = BrokenDateTime::Invalid();
      edl.forecast_auto_advance = true;
    }
  }
}

void
PersistTimeToPage(unsigned page_index) noexcept
{
  WeatherMapOverlay::MutateOverlayPage(
    page_index, PageLayout::Overlay::EDL,
    [](PageLayout &page) noexcept {
      const int encoded =
        EncodePageTime(CommonInterface::GetUIState().weather.edl);
      if (page.edl_time == encoded)
        return false;

      page.edl_time = encoded;
      return true;
    });
}

void
PersistTimeToCurrentPage() noexcept
{
  PersistTimeToPage(CommonInterface::GetUIState().pages.current_index);
}

static void
PersistPageEdlIsobar(int edl_isobar, unsigned page_index) noexcept
{
  WeatherMapOverlay::MutateOverlayPage(
    page_index, PageLayout::Overlay::EDL,
    [edl_isobar](PageLayout &page) noexcept {
      if (page.edl_isobar == edl_isobar)
        return false;

      page.edl_isobar = edl_isobar;
      return true;
    });
}

static void
AfterCursorChange() noexcept
{
  WeatherMapOverlay::NotifyLiveCursorChange();
  if (OverlayEnabled())
    RequestOverlayRefresh();
}

static BrokenDateTime
ResolvePageForecastTime(const PageLayout &page) noexcept
{
  if (page.edl_time != PageLayout::EDL_TIME_AUTO &&
      page.edl_time != PageLayout::EDL_TIME_NOW &&
      page.edl_time >= 0) {
    const BrokenDateTime decoded = DecodePageTime(page.edl_time);
    if (decoded.IsPlausible())
      return decoded;
  }

  BrokenDateTime utc = BrokenDateTime::NowUTC();
  const auto &basic = CommonInterface::Basic();
  if (basic.date_time_utc.IsPlausible())
    utc = basic.date_time_utc;
  return GetTrackedForecastTime(utc);
}

static unsigned
ResolvePageIsobar(const PageLayout &page) noexcept
{
  if (page.edl_isobar > 0 &&
      IsSupportedIsobar(unsigned(page.edl_isobar)))
    return unsigned(page.edl_isobar);

  return ResolveLevelBelow();
}

static bool
PageHasOverlayCache(BrokenDateTime forecast, unsigned isobar) noexcept
{
  return File::ExistsAny(TileRequest(forecast, isobar).BuildCachePath());
}

static void
RebuildForecastTimes(std::array<BrokenDateTime, FORECAST_CHOICES> &times,
                     BrokenDateTime selected_time) noexcept
{
  EnsureInitialised();

  if (!selected_time.IsPlausible())
    selected_time = GetTrackedForecastTime(BrokenDateTime::NowUTC());

  if (!selected_time.IsPlausible())
    return;

  const auto base_time = selected_time + std::chrono::hours{-11};
  for (unsigned i = 0; i < FORECAST_CHOICES; ++i)
    times[i] = base_time + std::chrono::hours{i};
}

static void
RebuildForecastTimes(std::array<BrokenDateTime, FORECAST_CHOICES> &times) noexcept
{
  RebuildForecastTimes(times, GetForecastTime());
}

static unsigned
FindForecastIndex(const std::array<BrokenDateTime, FORECAST_CHOICES> &times,
                  BrokenDateTime selected) noexcept
{
  for (unsigned i = 0; i < FORECAST_CHOICES; ++i)
    if (times[i] == selected)
      return i;

  const BrokenDateTime tracked =
    GetTrackedForecastTime(BrokenDateTime::NowUTC());
  for (unsigned i = 0; i < FORECAST_CHOICES; ++i)
    if (times[i] == tracked)
      return i;

  return 0;
}

static unsigned
FindForecastIndex(const std::array<BrokenDateTime, FORECAST_CHOICES> &times) noexcept
{
  return FindForecastIndex(times, GetForecastTime());
}

void
FormatTimeCursorLabel(StaticString<64> &text, bool auto_advance) noexcept
{
  StaticString<64> base;
  FormatForecastCursorLabel(base, auto_advance);
  WeatherMapOverlay::AssignLabeledData(text, base, HasOverlayCache());
}

void
FormatLevelCursorLabel(StaticString<64> &text, bool auto_advance) noexcept
{
  EnsureInitialised();

  const unsigned isobar = GetIsobar();
  const int altitude = GetAltitudeForIsobar(isobar);

  StaticString<64> base;
  if (auto_advance)
    base.Format(_("AUTO: %u hPa (%d m)"), isobar / 100, altitude);
  else
    base.Format(_("%u hPa (%d m)"), isobar / 100, altitude);

  WeatherMapOverlay::AssignLabeledData(text, base, HasOverlayCache());
}

static void
SetForecastTime(const BrokenDateTime &time) noexcept
{
  if (!time.IsPlausible())
    return;

  EnsureInitialised();
  auto &edl = CommonInterface::SetUIState().weather.edl;
  edl.forecast_datetime = time;
  edl.forecast_auto_advance = false;
  edl.session.cursor_initialized = true;
}

static void
SetForecastAuto() noexcept
{
  auto &edl = CommonInterface::SetUIState().weather.edl;
  edl.forecast_datetime = BrokenDateTime::Invalid();
  edl.forecast_auto_advance = true;
  edl.session.cursor_initialized = !edl.level_auto_advance;
}

void
SelectForecastTime(const BrokenDateTime &time) noexcept
{
  SetForecastTime(time);
  PersistTimeToCurrentPage();
}

void
SelectLevel(unsigned isobar) noexcept
{
  EnsureInitialised();
  auto &edl = CommonInterface::SetUIState().weather.edl;
  edl.SelectIsobar(isobar);
  edl.level_auto_advance = false;
  edl.session.cursor_initialized = true;
  PersistPageEdlIsobar(int(isobar),
                       CommonInterface::GetUIState().pages.current_index);
}

void
EnableForecastAutoFromInput() noexcept
{
  SetForecastAuto();

  const auto &basic = CommonInterface::Basic();
  if (basic.date_time_utc.IsPlausible())
    OnTimeUpdate(basic.date_time_utc);

  PersistTimeToCurrentPage();
}

void
EnableLevelAutoFromInput() noexcept
{
  auto &edl = CommonInterface::SetUIState().weather.edl;
  edl.level_auto_advance = true;
  edl.session.cursor_initialized = !edl.forecast_auto_advance;
  PersistPageEdlIsobar(0,
                       CommonInterface::GetUIState().pages.current_index);
  UpdateCurrentLevel();
}

bool
StepForecastTime(int delta) noexcept
{
  if (delta == 0)
    return false;

  std::array<BrokenDateTime, FORECAST_CHOICES> forecast_times{};
  RebuildForecastTimes(forecast_times);
  if (!forecast_times[0].IsPlausible())
    return false;

  const int index = int(FindForecastIndex(forecast_times)) + delta;
  if (index < 0 || index >= int(FORECAST_CHOICES))
    return false;

  SelectForecastTime(forecast_times[unsigned(index)]);
  return true;
}

bool
StepLevel(int delta) noexcept
{
  if (delta == 0)
    return false;

  EnsureInitialised();

  const unsigned current = GetIsobar();
  int index = -1;
  for (unsigned i = 0; i < NUM_ISOBARS; ++i) {
    if (ISOBARS[i] == current) {
      index = int(i);
      break;
    }
  }

  if (index < 0)
    index = 0;

  /* ISOBARS are ascending pressure (descending altitude); invert delta
     so "<" steps down and ">" steps up in height. */
  const int new_index = index - delta;
  if (new_index < 0 || new_index >= int(NUM_ISOBARS))
    return false;

  SelectLevel(ISOBARS[unsigned(new_index)]);
  return true;
}

bool
EditTimeOnLayout(PageLayout &page) noexcept
{
  EnsureInitialised();

  std::array<BrokenDateTime, FORECAST_CHOICES> forecast_times{};
  const BrokenDateTime selected = ResolvePageForecastTime(page);
  RebuildForecastTimes(forecast_times, selected);
  if (!forecast_times[0].IsPlausible())
    return false;

  DataFieldEnum field;
  for (unsigned i = 0; i < FORECAST_CHOICES; ++i) {
    const auto &t = forecast_times[i];
    StaticString<24> label;
    label.Format("%04u-%02u-%02u %02u:00",
                 unsigned(t.year), unsigned(t.month), unsigned(t.day),
                 unsigned(t.hour));
    field.addEnumText(label.c_str(), i);
  }
  field.SetValue(FindForecastIndex(forecast_times, selected));

  const ComboList combo_list = field.CreateComboList(nullptr);

  StaticString<64> caption;
  caption.Format("%s %s (UTC)", "EDL", _("Time"));

  const WeatherMapOverlay::TimePickerResult result =
    WeatherMapOverlay::RunTimePicker(caption.c_str(), combo_list);

  switch (result.selection) {
  case WeatherMapOverlay::TimePickerSelection::CANCEL:
    return false;

  case WeatherMapOverlay::TimePickerSelection::AUTO:
    page.edl_time = PageLayout::EDL_TIME_AUTO;
    break;

  case WeatherMapOverlay::TimePickerSelection::NOW: {
    /* Select the current tracked forecast date/time (not only the
       hour), so Auto/Now land on today's slot in the list. */
    BrokenDateTime utc = BrokenDateTime::NowUTC();
    const auto &basic = CommonInterface::Basic();
    if (basic.date_time_utc.IsPlausible())
      utc = basic.date_time_utc;
    page.edl_time =
      EncodePageTime(false, GetTrackedForecastTime(utc));
    break;
  }

  case WeatherMapOverlay::TimePickerSelection::MANUAL: {
    const unsigned index = combo_list[result.manual_index].int_value;
    if (index >= FORECAST_CHOICES)
      return false;
    page.edl_time = EncodePageTime(false, forecast_times[index]);
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
        page_index, PageLayout::Overlay::EDL,
        [](PageLayout &page) noexcept {
          return EditTimeOnLayout(page);
        }))
    return;

  const auto &page =
    CommonInterface::GetUISettings().pages.pages[page_index];
  ApplyTimeFromPageLayout(page);
  auto &edl = CommonInterface::SetUIState().weather.edl;
  edl.session.cursor_initialized = !edl.level_auto_advance ||
    !edl.forecast_auto_advance;
  AfterCursorChange();
}

LevelPickerResult
EditLevelOnLayout(PageLayout &page, bool offer_setup) noexcept
{
  EnsureInitialised();

  DataFieldEnum field;
  field.AddChoice(-1, _("Auto"));

  const bool manual =
    page.edl_isobar > 0 &&
    IsSupportedIsobar(unsigned(page.edl_isobar));
  unsigned selected_index = 0;

  for (unsigned i = 0; i < NUM_ISOBARS; ++i) {
    const unsigned isobar = ISOBARS[i];
    const int altitude = GetAltitudeForIsobar(isobar);

    StaticString<32> label;
    label.Format(_("%u hPa (%d m)"), isobar / 100, altitude);
    field.addEnumText(label.c_str(), int(i));

    if (manual && isobar == unsigned(page.edl_isobar))
      selected_index = i;
  }

  field.SetValue(manual ? unsigned(selected_index) : unsigned(-1));

  bool setup = false;
  const char *setup_caption = offer_setup ? _("Setup") : nullptr;
  if (!ComboPicker(_("EDL Level"), field, nullptr, setup_caption,
                   &setup))
    return setup ? LevelPickerResult::OPEN_SETUP
                 : LevelPickerResult::NONE;

  const unsigned value = field.GetValue();
  const int selected = value == unsigned(-1) ? -1 : int(value);
  if (selected >= int(NUM_ISOBARS))
    return LevelPickerResult::NONE;

  if (selected < 0)
    page.edl_isobar = 0;
  else
    page.edl_isobar = int(ISOBARS[unsigned(selected)]);

  return LevelPickerResult::CHANGED;
}

LevelPickerResult
OpenLevelPicker(bool offer_setup) noexcept
{
  LevelPickerResult result = LevelPickerResult::NONE;
  const unsigned page_index =
    CommonInterface::GetUIState().pages.current_index;
  if (!WeatherMapOverlay::MutateOverlayPage(
        page_index, PageLayout::Overlay::EDL,
        [offer_setup, &result](PageLayout &page) noexcept {
          result = EditLevelOnLayout(page, offer_setup);
          return result == LevelPickerResult::CHANGED;
        }))
    return result;

  const auto &page =
    CommonInterface::GetUISettings().pages.pages[page_index];
  if (page.edl_isobar <= 0) {
    EnableLevelAutoFromInput();
    AfterCursorChange();
  } else {
    auto &edl = CommonInterface::SetUIState().weather.edl;
    edl.SelectIsobar(unsigned(page.edl_isobar));
    edl.level_auto_advance = false;
    edl.session.cursor_initialized = true;
    AfterCursorChange();
  }
  return result;
}

void
FormatTimeLabelForPage(StaticString<64> &text,
                       const PageLayout &page) noexcept
{
  EnsureInitialised();

  const bool auto_advance =
    page.edl_time == PageLayout::EDL_TIME_AUTO;
  const BrokenDateTime forecast = ResolvePageForecastTime(page);
  const unsigned isobar = ResolvePageIsobar(page);

  StaticString<64> base;
  FormatForecastCursorLabel(base, auto_advance, forecast);
  WeatherMapOverlay::AssignLabeledData(text, base,
                                       PageHasOverlayCache(forecast.FloorToHour(),
                                                           isobar));
}

void
FormatLevelLabelForPage(StaticString<64> &text,
                        const PageLayout &page) noexcept
{
  EnsureInitialised();

  const bool auto_advance =
    !(page.edl_isobar > 0 &&
      IsSupportedIsobar(unsigned(page.edl_isobar)));
  const unsigned isobar = ResolvePageIsobar(page);
  const int altitude = GetAltitudeForIsobar(isobar);

  StaticString<64> base;
  if (auto_advance)
    base.Format(_("AUTO: %u hPa (%d m)"), isobar / 100, altitude);
  else
    base.Format(_("%u hPa (%d m)"), isobar / 100, altitude);

  WeatherMapOverlay::AssignLabeledData(
    text, base,
    PageHasOverlayCache(ResolvePageForecastTime(page), isobar));
}

} // namespace EDL
