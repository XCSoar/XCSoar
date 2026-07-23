// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FieldControls.hpp"

#include "ActionInterface.hpp"
#include "RaspStore.hpp"
#include "DataGlobals.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "PageActions.hpp"
#include "PageSettings.hpp"
#include "UIState.hpp"
#include "Weather/MapOverlay/ControlsWidget.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"
#include "Weather/MapOverlay/PageCursor.hpp"
#include "Weather/MapOverlay/TimePicker.hpp"
#include "util/Compiler.h"
#include "util/StaticString.hxx"

#ifdef HAVE_DOWNLOAD_MANAGER
#include "Weather/Rasp/DownloadGlue.hpp"
#endif

#include <cstdio>
#include <vector>

namespace Rasp {

static BrokenTime GetAutoAdvanceLocalTime() noexcept;
static BrokenTime GetEffectiveLocalTime(bool auto_advance) noexcept;
static void FormatTimeLabel(StaticString<64> &text, bool auto_advance,
                            BrokenTime forecast, bool has_data) noexcept;
static bool FieldHasAnyTime(const RaspStore &rasp,
                            unsigned field_index) noexcept;
static constexpr unsigned NOW_CHOICE_MINUTE_OF_DAY = 24U * 60U;

[[gnu::pure]]
static const char *
GetFieldLabel(const RaspStore::MapItem &item) noexcept
{
  return item.label != nullptr
    ? gettext(item.label)
    : item.name.c_str();
}

void
FillFieldChoices(DataFieldEnum &field, const RaspStore *rasp,
                 FieldChoicesOptions options) noexcept
{
  field.ClearChoices();

  if (options.include_none)
    field.AddChoice(-1, "none", _("None"), nullptr);

  if (rasp == nullptr)
    return;

  for (unsigned i = 0; i < rasp->GetItemCount(); ++i) {
    const auto &item = rasp->GetItemInfo(i);
    const char *help = item.help != nullptr
      ? gettext(item.help)
      : nullptr;

    field.AddChoice(i, item.name, GetFieldLabel(item), help);
  }
}

BrokenTime
TimeFromMinuteOfDay(unsigned minute_of_day) noexcept
{
  if (minute_of_day == NOW_CHOICE_MINUTE_OF_DAY || minute_of_day >= 24U * 60U)
    return BrokenTime::Invalid();

  return BrokenTime::FromMinuteOfDay(minute_of_day);
}

unsigned
MinuteOfDayFromTime(BrokenTime time) noexcept
{
  return time.IsPlausible() ? time.GetMinuteOfDay()
                            : NOW_CHOICE_MINUTE_OF_DAY;
}

int
GetFieldIndex(const PageLayout &layout) noexcept
{
  if (layout.overlay != PageLayout::Overlay::RASP)
    return -1;

  if (layout.rasp_field < 0)
    return -1;

  const auto rasp = DataGlobals::GetRasp();
  if (rasp == nullptr || rasp->GetItemCount() == 0)
    return -1;

  if (layout.rasp_field >= 0 &&
      unsigned(layout.rasp_field) < rasp->GetItemCount())
    return layout.rasp_field;

  return 0;
}

int
GetActiveFieldIndex() noexcept
{
  return GetFieldIndex(PageActions::GetCurrentLayout());
}

int
GetEffectiveFieldIndex() noexcept
{
  const int field_index = GetActiveFieldIndex();
  if (field_index >= 0)
    return field_index;

  const int map = CommonInterface::GetUIState().weather.map;
  return map >= 0 ? map : -1;
}

void
SyncCursorFromPageLayout() noexcept
{
  const auto &layout = PageActions::GetCurrentLayout();
  if (layout.overlay != PageLayout::Overlay::RASP)
    return;

  auto &weather = CommonInterface::SetUIState().weather;
  weather.map = -1;

  const int field_index = GetActiveFieldIndex();
  if (field_index >= 0)
    weather.map = field_index;
}

int
EncodePageTime(bool time_auto_advance, BrokenTime time) noexcept
{
  if (time_auto_advance)
    return PageLayout::RASP_TIME_AUTO;

  if (!time.IsPlausible())
    return PageLayout::RASP_TIME_NOW;

  return int(time.GetMinuteOfDay());
}

/**
 * Decode #PageLayout::rasp_time into session-shaped auto/manual values.
 */
static void
DecodePageTime(const PageLayout &layout, bool &auto_advance,
               BrokenTime &manual) noexcept
{
  if (layout.overlay != PageLayout::Overlay::RASP ||
      layout.rasp_time == PageLayout::RASP_TIME_AUTO) {
    auto_advance = true;
    manual = BrokenTime::Invalid();
    return;
  }

  auto_advance = false;
  if (layout.rasp_time == PageLayout::RASP_TIME_NOW)
    manual = BrokenTime::Invalid();
  else
    manual = BrokenTime::FromMinuteOfDay(unsigned(layout.rasp_time));
}

void
ApplyTimeFromPageLayout(const PageLayout &layout) noexcept
{
  auto &weather = CommonInterface::SetUIState().weather;
  DecodePageTime(layout, weather.time_auto_advance, weather.time);
}

void
PersistTimeToPage(unsigned page_index) noexcept
{
  WeatherMapOverlay::MutateOverlayPage(
    page_index, PageLayout::Overlay::RASP,
    [](PageLayout &page) noexcept {
      const auto &weather = CommonInterface::GetUIState().weather;
      const int encoded =
        EncodePageTime(weather.time_auto_advance, weather.time);
      if (page.rasp_time == encoded)
        return false;

      page.rasp_time = encoded;
      return true;
    });
}

void
PersistTimeToCurrentPage() noexcept
{
  PersistTimeToPage(CommonInterface::GetUIState().pages.current_index);
}

void
SetCursorTime(unsigned minute_of_day) noexcept
{
  auto &weather = CommonInterface::SetUIState().weather;
  weather.time = TimeFromMinuteOfDay(minute_of_day);
  weather.time_auto_advance = false;
  weather.rasp.cursor_initialized = true;
  PersistTimeToCurrentPage();

  ActionInterface::SendUIState(true);
}

void
EnableTimeAutoFromInput() noexcept
{
  SetTimeAutoAdvance(true);
  ApplyAutoAdvanceTime();
  PersistTimeToCurrentPage();

#ifdef HAVE_DOWNLOAD_MANAGER
  if (!HasSelectedTimeData(true))
    RequestConfiguredRaspUpdateIfOutOfDate();
#endif
}

bool
EditTimeOnLayout(PageLayout &page) noexcept
{
  const auto rasp = DataGlobals::GetRasp();
  const int field_index = GetFieldIndex(page);
  if (rasp == nullptr || field_index < 0 ||
      unsigned(field_index) >= rasp->GetItemCount())
    return false;

  DataFieldEnum field;
  for (unsigned i = 0; i < RaspStore::MAX_WEATHER_TIMES; ++i) {
    const BrokenTime t = RaspStore::IndexToTime(i);
    StaticString<24> label;
    label.Format("%02u:%02u %s",
                 unsigned(t.hour), unsigned(t.minute),
                 rasp->IsTimeAvailable(unsigned(field_index), i)
                 ? "[x]" : "[ ]");
    field.addEnumText(label.c_str(), t.GetMinuteOfDay());
  }

  bool auto_advance;
  BrokenTime manual;
  DecodePageTime(page, auto_advance, manual);
  const BrokenTime effective =
    auto_advance || !manual.IsPlausible()
      ? GetAutoAdvanceLocalTime()
      : manual;
  if (effective.IsPlausible())
    field.SetValue(effective.GetMinuteOfDay());

  const ComboList combo_list = field.CreateComboList(nullptr);
  const WeatherMapOverlay::TimePickerResult result =
    WeatherMapOverlay::RunTimePicker(_("RASP Time"), combo_list);

  switch (result.selection) {
  case WeatherMapOverlay::TimePickerSelection::CANCEL:
    return false;

  case WeatherMapOverlay::TimePickerSelection::AUTO:
    page.rasp_time = PageLayout::RASP_TIME_AUTO;
    break;

  case WeatherMapOverlay::TimePickerSelection::NOW: {
    /* Select the current local quarter-hour slot (same as picking
       that row in the list), not only the Now mode sentinel. */
    const BrokenTime now = GetAutoAdvanceLocalTime();
    if (now.IsPlausible())
      page.rasp_time = int(now.GetMinuteOfDay());
    else
      page.rasp_time = PageLayout::RASP_TIME_NOW;
    break;
  }

  case WeatherMapOverlay::TimePickerSelection::MANUAL:
    page.rasp_time = combo_list[result.manual_index].int_value;
    break;

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
        page_index, PageLayout::Overlay::RASP,
        [](PageLayout &page) noexcept {
          return EditTimeOnLayout(page);
        }))
    return;

  const auto &page =
    CommonInterface::GetUISettings().pages.pages[page_index];
  auto &weather = CommonInterface::SetUIState().weather;
  weather.map = GetFieldIndex(page);
  ApplyTimeFromPageLayout(page);
  weather.rasp.cursor_initialized = true;

#ifdef HAVE_DOWNLOAD_MANAGER
  if (weather.time_auto_advance && !HasSelectedTimeData(true))
    RequestConfiguredRaspUpdateIfOutOfDate();
#endif
  WeatherMapOverlay::NotifyLiveCursorChange();
}

void
FormatTimeLabelForPage(StaticString<64> &text,
                       const PageLayout &page) noexcept
{
  bool auto_advance;
  BrokenTime manual;
  DecodePageTime(page, auto_advance, manual);

  const BrokenTime auto_local = GetAutoAdvanceLocalTime();
  const BrokenTime forecast =
    auto_advance || !manual.IsPlausible() ? auto_local : manual;

  FormatTimeLabel(text, auto_advance, forecast,
                  HasSelectedTimeData(auto_advance, GetFieldIndex(page),
                                      manual, auto_local));
}

bool
GetTimeAutoAdvance() noexcept
{
  return CommonInterface::GetUIState().weather.time_auto_advance;
}

void
SetTimeAutoAdvance(bool auto_advance) noexcept
{
  CommonInterface::SetUIState().weather.time_auto_advance = auto_advance;
}

void
ApplyAutoAdvanceTime() noexcept
{
  CommonInterface::SetUIState().weather.time = BrokenTime::Invalid();
  ActionInterface::SendUIState(true);
}

bool
StepCursorTime(int delta) noexcept
{
  const int field_index = GetActiveFieldIndex();
  if (field_index < 0 || delta == 0)
    return false;

  const auto &weather = CommonInterface::GetUIState().weather;
  unsigned minute_of_day = 0;
  if (!StepTime(weather.time_auto_advance, field_index, delta,
                minute_of_day))
    return false;

  SetCursorTime(minute_of_day);
  return true;
}

unsigned
GetFieldCount() noexcept
{
  const auto rasp = DataGlobals::GetRasp();
  return rasp != nullptr ? rasp->GetItemCount() : 0U;
}

void
FormatFieldCursorLabel(StaticString<64> &text) noexcept
{
  const auto rasp = DataGlobals::GetRasp();
  const int field_index = GetEffectiveFieldIndex();
  if (rasp == nullptr || field_index < 0) {
    text = _("No layer");
    return;
  }

  text = GetFieldLabel(rasp->GetItemInfo(unsigned(field_index)));
}

bool
SelectField(unsigned field_index) noexcept
{
  const auto rasp = DataGlobals::GetRasp();
  if (rasp == nullptr || field_index >= rasp->GetItemCount())
    return false;

  if (!FieldHasAnyTime(*rasp, field_index))
    return false;

  const unsigned page_index =
    CommonInterface::GetUIState().pages.current_index;
  if (!WeatherMapOverlay::MutateOverlayPage(
        page_index, PageLayout::Overlay::RASP,
        [field_index](PageLayout &page) noexcept {
          page.rasp_field = int(field_index);
          return true;
        }))
    return false;

  auto &weather = CommonInterface::SetUIState().weather;
  weather.map = int(field_index);
  weather.rasp.cursor_initialized = true;

  if (!weather.time_auto_advance &&
      !rasp->HasSelectedTimeData(field_index, false,
                                 weather.time, GetAutoAdvanceLocalTime())) {
    const BrokenTime effective =
      GetEffectiveLocalTime(false);
    const unsigned desired = effective.IsPlausible()
      ? RaspStore::TimeToIndex(effective)
      : 0;
    const unsigned nearest = rasp->GetNearestTime(field_index, desired);
    if (nearest < RaspStore::MAX_WEATHER_TIMES) {
      weather.time = RaspStore::IndexToTime(nearest);
    }
  }

  ActionInterface::UpdateDisplayMode();
  ActionInterface::SendUIState(true);
  return true;
}

bool
ClearSelectedField() noexcept
{
  const unsigned page_index =
    CommonInterface::GetUIState().pages.current_index;
  if (!WeatherMapOverlay::MutateOverlayPage(
        page_index, PageLayout::Overlay::RASP,
        [](PageLayout &page) noexcept {
          page.rasp_field = -1;
          return true;
        }))
    return false;

  auto &weather = CommonInterface::SetUIState().weather;
  weather.map = -1;
  weather.rasp.cursor_initialized = true;

  ActionInterface::UpdateDisplayMode();
  ActionInterface::SendUIState(true);
  return true;
}

LayerPickerResult
OpenLayerPicker(bool offer_setup) noexcept
{
  const auto rasp = DataGlobals::GetRasp();
  if (rasp == nullptr || rasp->GetItemCount() == 0)
    return offer_setup ? LayerPickerResult::OPEN_SETUP
                       : LayerPickerResult::NONE;

  DataFieldEnum field;
  FieldChoicesOptions options;
  options.include_none = true;
  FillFieldChoices(field, rasp.get(), options);

  const int current = GetEffectiveFieldIndex();
  field.SetValue(current >= 0 ? current : -1);

  bool setup = false;
  const char *setup_caption = offer_setup ? _("Setup") : nullptr;
  if (!ComboPicker(_("RASP Layer"), field, nullptr, setup_caption,
                   &setup))
    return setup ? LayerPickerResult::OPEN_SETUP
                 : LayerPickerResult::NONE;

  const int selected = field.GetValue();
  if (selected < 0) {
    ClearSelectedField();
    return LayerPickerResult::CHANGED;
  }

  if (!SelectField(unsigned(selected)))
    return LayerPickerResult::NONE;

  return LayerPickerResult::CHANGED;
}

bool
StepField(int delta) noexcept
{
  const unsigned count = GetFieldCount();
  if (count == 0 || delta == 0)
    return false;

  const auto rasp = DataGlobals::GetRasp();
  if (rasp == nullptr)
    return false;

  int index = GetEffectiveFieldIndex();
  if (index < 0)
    index = 0;

  int current = index;
  int remaining = delta;
  while (remaining != 0) {
    const int direction = remaining > 0 ? 1 : -1;
    bool found = false;
    for (unsigned i = 0; i < count; ++i) {
      current += direction;
      current %= int(count);
      if (current < 0)
        current += int(count);

      if (FieldHasAnyTime(*rasp, unsigned(current))) {
        found = true;
        break;
      }
    }

    if (!found)
      return false;

    remaining -= direction;
  }

  return SelectField(unsigned(current));
}

bool
HasSelectedField() noexcept
{
  return GetFieldCount() > 0 && GetEffectiveFieldIndex() >= 0;
}

static BrokenTime
GetLocalTimeNow() noexcept
{
  const auto &basic = CommonInterface::Basic();
  if (!basic.date_time_utc.IsPlausible())
    return BrokenTime::Invalid();

  const auto local = basic.date_time_utc.ToLocal();
  return BrokenTime(local.hour, local.minute);
}

static BrokenTime
GetAutoAdvanceLocalTime() noexcept
{
  const auto &basic = CommonInterface::Basic();
  if (!basic.date_time_utc.IsPlausible())
    return BrokenTime::Invalid();

  const auto local = basic.date_time_utc.ToLocal().FloorToQuarterHour();
  return BrokenTime(local.hour, local.minute);
}

static BrokenTime
GetEffectiveLocalTime(bool auto_advance) noexcept
{
  const auto &weather = CommonInterface::GetUIState().weather;
  if (auto_advance || !weather.time.IsPlausible())
    return GetAutoAdvanceLocalTime();

  return weather.time;
}

[[gnu::pure]]
static bool
FieldHasAnyTime(const RaspStore &rasp, unsigned field_index) noexcept
{
  for (unsigned i = 0; i < RaspStore::MAX_WEATHER_TIMES; ++i)
    if (rasp.IsTimeAvailable(field_index, i))
      return true;

  return false;
}

[[gnu::pure]]
static unsigned
StepToAvailableTimeIndex(const RaspStore &rasp, unsigned field_index,
                         unsigned from, int delta) noexcept
{
  if (!FieldHasAnyTime(rasp, field_index))
    return RaspStore::MAX_WEATHER_TIMES;

  int current = int(from);
  int remaining = delta;
  while (remaining != 0) {
    const int direction = remaining > 0 ? 1 : -1;
    bool found = false;
    for (int next = current + direction;
         next >= 0 && next < int(RaspStore::MAX_WEATHER_TIMES);
         next += direction) {
      if (!rasp.IsTimeAvailable(field_index, unsigned(next)))
        continue;

      current = next;
      found = true;
      break;
    }

    if (!found)
      return RaspStore::MAX_WEATHER_TIMES;

    remaining -= direction;
  }

  return unsigned(current);
}

bool
StepTime(bool time_auto_advance, int field_index, int delta,
         unsigned &minute_of_day) noexcept
{
  if (field_index < 0 || delta == 0)
    return false;

  const auto rasp = DataGlobals::GetRasp();
  if (rasp == nullptr || unsigned(field_index) >= rasp->GetItemCount())
    return false;

  unsigned quarter = 0;
  const BrokenTime effective = GetEffectiveLocalTime(time_auto_advance);
  if (effective.IsPlausible())
    quarter = RaspStore::TimeToIndex(effective);

  const unsigned next = StepToAvailableTimeIndex(*rasp, unsigned(field_index),
                                                 quarter, delta);
  if (next >= RaspStore::MAX_WEATHER_TIMES)
    return false;

  minute_of_day = RaspStore::IndexToTime(next).GetMinuteOfDay();
  return true;
}

bool
HasSelectedTimeData(bool auto_advance, int field_index,
                    BrokenTime manual_time,
                    BrokenTime auto_local_time) noexcept
{
  const auto rasp = DataGlobals::GetRasp();
  if (rasp == nullptr || field_index < 0)
    return false;

  return rasp->HasSelectedTimeData(unsigned(field_index), auto_advance,
                                   manual_time, auto_local_time);
}

bool
HasSelectedTimeData(bool auto_advance) noexcept
{
  const auto &weather = CommonInterface::GetUIState().weather;
  return HasSelectedTimeData(auto_advance, GetEffectiveFieldIndex(),
                             weather.time, GetAutoAdvanceLocalTime());
}

void
MaybeRequestConfiguredRaspUpdateOnAutoNoData() noexcept
{
#ifdef HAVE_DOWNLOAD_MANAGER
  const auto &weather = CommonInterface::GetUIState().weather;
  if (!weather.time_auto_advance)
    return;

  if (HasSelectedTimeData(true))
    return;

  if (GetActiveFieldIndex() < 0)
    return;

  const auto &basic = CommonInterface::Basic();
  if (!basic.date_time_utc.IsPlausible())
    return;

  static unsigned last_attempt_quarter = unsigned(-1);
  const auto local = basic.date_time_utc.ToLocal().FloorToQuarterHour();
  const unsigned quarter =
    RaspStore::TimeToIndex(BrokenTime(local.hour, local.minute));

  if (quarter == last_attempt_quarter)
    return;

  last_attempt_quarter = quarter;
  RequestConfiguredRaspUpdateIfOutOfDate();
#endif
}

unsigned
GetCursorBarMinuteOfDay() noexcept
{
  const BrokenTime effective =
    GetEffectiveLocalTime(GetTimeAutoAdvance());
  if (!effective.IsPlausible())
    return 0;

  return effective.GetMinuteOfDay();
}

void
FormatTimeCursorLabel(StaticString<64> &text, bool auto_advance) noexcept
{
  FormatTimeLabel(text, auto_advance, GetEffectiveLocalTime(auto_advance),
                  HasSelectedTimeData(auto_advance));
}

static void
FormatTimeLabel(StaticString<64> &text, bool auto_advance,
                BrokenTime forecast, bool has_data) noexcept
{
  const BrokenTime now = GetLocalTimeNow();

  char offset_buf[16] = {};
  if (forecast.IsPlausible() && now.IsPlausible()) {
    int offset_min = int(forecast.GetMinuteOfDay()) -
                     int(now.GetMinuteOfDay());
    offset_min = WeatherMapOverlay::NormaliseMinuteOffsetAroundNow(offset_min);
    WeatherMapOverlay::FormatSignedMinuteOffset(offset_buf, sizeof(offset_buf),
                                         offset_min);
  }

  StaticString<64> base;
  WeatherMapOverlay::FormatAutoLocalTimeLabel(base, auto_advance,
                                       forecast.IsPlausible(),
                                       unsigned(forecast.hour),
                                       unsigned(forecast.minute),
                                       offset_buf);

  WeatherMapOverlay::AssignLabeledData(text, base, has_data);
}

} // namespace Rasp
