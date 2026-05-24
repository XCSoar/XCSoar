// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FieldControls.hpp"

#include "RaspStore.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"

#include <algorithm>
#include <fmt/format.h>
#include <vector>

namespace Rasp {

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

bool
StepTime(const RaspStore *rasp, unsigned field_index,
         BrokenTime current_time, int delta,
         unsigned &minute_of_day) noexcept
{
  if (rasp == nullptr || field_index >= rasp->GetItemCount())
    return false;

  std::vector<unsigned> minutes;
  minutes.push_back(0);
  for (unsigned i = 0; i < RaspStore::MAX_WEATHER_TIMES; ++i) {
    if (!rasp->IsTimeAvailable(field_index, i))
      continue;

    minutes.push_back(RaspStore::IndexToTime(i).GetMinuteOfDay());
  }

  if (minutes.size() <= 1)
    return false;

  const unsigned current_minute = MinuteOfDayFromTime(current_time);

  auto it = std::find(minutes.begin(), minutes.end(), current_minute);
  unsigned index = it == minutes.end()
    ? 0
    : unsigned(std::distance(minutes.begin(), it));

  if (delta > 0 && index > 0)
    --index;
  else if (delta < 0 && index + 1 < minutes.size())
    ++index;
  else
    return false;

  minute_of_day = minutes[index];
  return true;
}

} // namespace Rasp
