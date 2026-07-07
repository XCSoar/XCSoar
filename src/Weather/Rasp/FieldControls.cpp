// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FieldControls.hpp"

#include "ActionInterface.hpp"
#include "RaspStore.hpp"
#include "DataGlobals.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "PageActions.hpp"
#include "PageSettings.hpp"
#include "Profile/Current.hpp"
#include "Profile/PageProfile.hpp"
#include "UIState.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"

#ifdef HAVE_DOWNLOAD_MANAGER
#include "Weather/Rasp/DownloadGlue.hpp"
#endif

#include <algorithm>
#include <cstdio>
#include <fmt/format.h>
#include <vector>

namespace Rasp {

static BrokenTime GetAutoAdvanceLocalTime() noexcept;

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
    const char *label = item.label != nullptr
      ? gettext(item.label)
      : item.name;
    const char *help = item.help != nullptr
      ? gettext(item.help)
      : nullptr;

    field.AddChoice(i, item.name, label, help);
  }
}

void
InitTimeChoices(DataFieldEnum &field) noexcept
{
  field.ClearChoices();
  field.addEnumText(_("Now"));
}

void
FillTimeChoices(DataFieldEnum &field, const RaspStore *rasp,
                unsigned field_index, BrokenTime selected_time) noexcept
{
  InitTimeChoices(field);

  if (rasp == nullptr || field_index >= rasp->GetItemCount())
    return;

  for (unsigned i = 0; i < RaspStore::MAX_WEATHER_TIMES; ++i) {
    if (!rasp->IsTimeAvailable(field_index, i))
      continue;

    const BrokenTime t = RaspStore::IndexToTime(i);
    char timetext[8];
    const auto result = fmt::format_to_n(timetext, sizeof(timetext) - 1,
                                         "{:02}:{:02}", t.hour, t.minute);
    const size_t length = std::min(result.size, sizeof(timetext) - 1);
    timetext[length] = '\0';
    field.addEnumText(timetext, t.GetMinuteOfDay());
  }

  field.SetValue(MinuteOfDayFromTime(selected_time));
}

BrokenTime
TimeFromMinuteOfDay(unsigned minute_of_day) noexcept
{
  return minute_of_day > 0
    ? BrokenTime::FromMinuteOfDay(minute_of_day)
    : BrokenTime::Invalid();
}

unsigned
MinuteOfDayFromTime(BrokenTime time) noexcept
{
  return time.IsPlausible() ? time.GetMinuteOfDay() : 0U;
}

int
GetFieldIndex(const PageLayout &layout) noexcept
{
  if (layout.overlay != PageLayout::Overlay::RASP)
    return -1;

  const auto rasp = DataGlobals::GetRasp();
  if (rasp == nullptr || rasp->GetItemCount() == 0)
    return -1;

  if (layout.rasp_field >= 0 &&
      unsigned(layout.rasp_field) < rasp->GetItemCount())
    return layout.rasp_field;

  return rasp->GetItemCount() > 0 ? 0 : -1;
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

void
SetCursorTime(unsigned minute_of_day) noexcept
{
  auto &weather = CommonInterface::SetUIState().weather;
  weather.time = TimeFromMinuteOfDay(minute_of_day);
  weather.time_auto_advance = false;
  weather.rasp.cursor_initialized = true;

  ActionInterface::SendUIState(true);
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

void
ResumeAutoAdvance() noexcept
{
  if (GetTimeAutoAdvance())
    return;

  SetTimeAutoAdvance(true);
  ApplyAutoAdvanceTime();
}

bool
StepCursorTime(int delta) noexcept
{
  const auto rasp = DataGlobals::GetRasp();
  const int field_index = GetActiveFieldIndex();
  if (rasp == nullptr || field_index < 0 || delta == 0)
    return false;

  const auto &weather = CommonInterface::GetUIState().weather;
  unsigned minute_of_day = 0;
  if (!StepTime(rasp.get(), unsigned(field_index),
                weather.time, weather.time_auto_advance,
                delta, minute_of_day))
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

  const auto &info = rasp->GetItemInfo(unsigned(field_index));
  const char *field_label = info.label != nullptr
    ? gettext(info.label)
    : info.name.c_str();
  text = field_label;
}

bool
SelectField(unsigned field_index) noexcept
{
  const auto rasp = DataGlobals::GetRasp();
  if (rasp == nullptr || field_index >= rasp->GetItemCount())
    return false;

  PageSettings &settings = CommonInterface::SetUISettings().pages;
  const PagesState &pages = CommonInterface::GetUIState().pages;
  const unsigned page_index = pages.current_index;

  PageLayout &page = settings.pages[page_index];
  if (page.overlay != PageLayout::Overlay::RASP)
    return false;

  page.rasp_field = int(field_index);
  page.Normalise();
  Profile::Save(Profile::map, page, page_index);

  auto &weather = CommonInterface::SetUIState().weather;
  weather.map = int(field_index);
  weather.rasp.cursor_initialized = true;

  ActionInterface::UpdateDisplayMode();
  ActionInterface::SendUIState(true);
  return true;
}

bool
StepField(int delta) noexcept
{
  const unsigned count = GetFieldCount();
  if (count == 0 || delta == 0)
    return false;

  int index = GetEffectiveFieldIndex();
  if (index < 0)
    index = 0;

  int next = (int(index) + delta) % int(count);
  if (next < 0)
    next += int(count);

  return SelectField(unsigned(next));
}

bool
HasSelectedField() noexcept
{
  return GetFieldCount() > 0 && GetEffectiveFieldIndex() >= 0;
}

StaticString<64>
GetPanOverlayLabel(const PageLayout &configured) noexcept
{
  StaticString<64> label;

  if (configured.overlay != PageLayout::Overlay::RASP)
    return label;

  const auto rasp = DataGlobals::GetRasp();
  if (rasp == nullptr || rasp->GetItemCount() == 0)
    return label;

  const int field_index = GetFieldIndex(configured);
  if (field_index < 0)
    return label;

  const auto &info = rasp->GetItemInfo(unsigned(field_index));
  const char *field_label = info.label != nullptr
    ? gettext(info.label)
    : info.name.c_str();

  const auto &weather = CommonInterface::GetUIState().weather;
  if (weather.time_auto_advance || !weather.time.IsPlausible())
    label.Format(_("RASP %s"), field_label);
  else
    label.Format(_("RASP %s %02u:%02u"), field_label,
                 weather.time.hour, weather.time.minute);

  return label;
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

static unsigned
ToQuarterHours(BrokenTime t) noexcept
{
  return unsigned(t.hour) * 4u + unsigned(t.minute) / 15u;
}

bool
StepTime(const RaspStore *rasp, unsigned field_index,
         BrokenTime current_time, bool time_auto_advance,
         int delta, unsigned &minute_of_day) noexcept
{
  (void)rasp;
  (void)field_index;
  (void)current_time;

  if (delta == 0)
    return false;

  unsigned quarter = 0;
  const BrokenTime effective = GetEffectiveLocalTime(time_auto_advance);
  if (effective.IsPlausible())
    quarter = ToQuarterHours(effective);
  else {
    const BrokenTime auto_time = GetAutoAdvanceLocalTime();
    if (auto_time.IsPlausible())
      quarter = ToQuarterHours(auto_time);
  }

  int next = int(quarter) + delta;
  next = (next % 96 + 96) % 96;
  minute_of_day = unsigned(next) * 15;
  return true;
}

bool
HasSelectedTimeData(bool auto_advance) noexcept
{
  const auto rasp = DataGlobals::GetRasp();
  const int field_index = GetEffectiveFieldIndex();
  if (rasp == nullptr || field_index < 0)
    return false;

  const BrokenTime forecast = GetEffectiveLocalTime(auto_advance);
  if (!forecast.IsPlausible())
    return false;

  const unsigned time_index = ToQuarterHours(forecast);
  return rasp->IsTimeAvailable(unsigned(field_index), time_index);
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
  const unsigned quarter = unsigned(local.hour) * 4u +
    unsigned(local.minute) / 15u;

  if (quarter == last_attempt_quarter)
    return;

  last_attempt_quarter = quarter;
  RequestConfiguredRaspUpdateIfOutOfDate();
#endif
}

void
FormatTimeCursorLabel(StaticString<64> &text, bool auto_advance) noexcept
{
  const BrokenTime forecast = GetEffectiveLocalTime(auto_advance);
  const BrokenTime now = GetLocalTimeNow();
  const bool has_data = HasSelectedTimeData(auto_advance);

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

  if (has_data)
    text = base;
  else
    WeatherMapOverlay::AppendNoDataTag(text, base.c_str());
}

} // namespace Rasp
