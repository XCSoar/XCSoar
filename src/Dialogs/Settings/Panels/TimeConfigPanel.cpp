// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TimeConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Profile/ComputerProfile.hpp"
#include "Profile/Current.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "time/BrokenDateTime.hpp"
#include "time/SystemTimeZone.hpp"
#include "time/TimeZones.hpp"

#include <cstdlib>
#include <vector>

using namespace std::chrono;

static constexpr auto UTC_OFFSET_STEP = minutes{15};

static constexpr StaticEnumChoice local_time_source_list[] = {
#ifndef KOBO
  /* the Kobo has no time zone configuration which we could follow */
  { LocalTimeSource::AUTOMATIC, N_("Automatic"),
    N_("Use the time zone which is configured in the operating system, and "
       "keep following it across daylight saving time changes and when "
       "travelling to another time zone.") },
#endif
  { LocalTimeSource::TIME_ZONE, N_("Time zone"),
    N_("Use the time zone selected below.  XCSoar knows its daylight saving "
       "time rules and applies the transitions on its own, which means the "
       "setting does not need to be corrected twice a year.") },
  nullptr
};

/**
 * The manual offset is what the other two sources exist to avoid, so it is
 * offered to experts only; a profile which uses it must of course still
 * be able to show and keep it.
 */
static constexpr StaticEnumChoice manual_utc_offset_list[] = {
  { LocalTimeSource::MANUAL_UTC_OFFSET, N_("Manual UTC offset"),
    N_("Use the fixed UTC offset entered below.  It has to be corrected "
       "manually whenever daylight saving time begins or ends.") },
  nullptr
};

/** "+01:00" or "-03:30". */
static void
FormatUTCOffset(StaticString<32> &buffer, RoughTimeDelta offset) noexcept
{
  const int s = offset.AsSeconds();
  buffer.Format("UTC%c%s", s < 0 ? '-' : '+',
                FormatSignedTimeHHMM(seconds{std::abs(s)}).c_str());
}

/**
 * Where the local time comes from: the operating system, a time zone
 * or a fixed offset; and whether the GPS sets the clock.
 */
class TimeConfigPanel final : public ConfigListPanel {
  LocalTimeSource local_time_source;
  StaticString<64> time_zone;
  RoughTimeDelta manual_utc_offset;
  bool manual_utc_offset_modified = false;
  bool set_system_time_from_gps;

  /** the local time is a clock: it moves while the page is open */
  UI::PeriodicTimer local_time_timer{[this]{ Refresh(); }};

private:
  void PickLocalTimeSource() noexcept;
  void PickTimeZone() noexcept;
  void PickManualUTCOffset() noexcept;

  /** Calculate the UTC offset which the given source currently yields. */
  [[gnu::pure]]
  RoughTimeDelta GetUTCOffset(LocalTimeSource source) const noexcept;

  void AddLocalTimeItem() noexcept;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
TimeConfigPanel::LoadSettings() noexcept
{
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  manual_utc_offset = settings_computer.utc_offset;
  Profile::LoadUTCOffset(Profile::map, manual_utc_offset);

  local_time_source = settings_computer.local_time_source;

#ifdef KOBO
  if (local_time_source == LocalTimeSource::AUTOMATIC)
    /* the profile was written on another platform */
    local_time_source = LocalTimeSource::TIME_ZONE;
#endif

  /* a time zone which is not in our table: fall back to UTC */
  time_zone = FindTimeZone(settings_computer.time_zone.c_str()) != nullptr
    ? settings_computer.time_zone.c_str()
    : "UTC";

  set_system_time_from_gps = settings_computer.set_system_time_from_gps;
}

RoughTimeDelta
TimeConfigPanel::GetUTCOffset(LocalTimeSource source) const noexcept
{
  switch (source) {
  case LocalTimeSource::AUTOMATIC:
    return RoughTimeDelta::FromSeconds(GetCurrentTimeZoneOffset());

  case LocalTimeSource::TIME_ZONE:
    if (const auto offset = FindTimeZoneOffset(time_zone.c_str(),
                                               system_clock::now()))
      return RoughTimeDelta::FromSeconds(offset->count());

    return RoughTimeDelta::FromSeconds(0);

  case LocalTimeSource::MANUAL_UTC_OFFSET:
    break;
  }

  return manual_utc_offset;
}

void
TimeConfigPanel::PickLocalTimeSource() noexcept
{
  /* the manual offset is offered to experts, and kept for a profile
     which uses it */
  std::vector<PickerChoice> choices;
  std::vector<LocalTimeSource> sources;
  int current = -1;

  const auto add = [&](const StaticEnumChoice *list){
    for (auto i = list; i->display_string != nullptr; ++i) {
      if (LocalTimeSource(i->id) == local_time_source)
        current = choices.size();

      choices.push_back({gettext(i->display_string), gettext(i->help)});
      sources.push_back(LocalTimeSource(i->id));
    }
  };

  add(local_time_source_list);
  if (IsExpert() || local_time_source == LocalTimeSource::MANUAL_UTC_OFFSET)
    add(manual_utc_offset_list);

  const int picked =
    PickChoice(_("Local time source"),
               _("Selects where XCSoar gets the offset between "
                 "UTC and local time from."),
               choices, current);
  if (picked < 0 || picked == current)
    return;

  local_time_source = sources[picked];
  Refresh();
}

void
TimeConfigPanel::PickTimeZone() noexcept
{
  const auto zones = GetTimeZones();

  std::vector<PickerChoice> choices;
  choices.reserve(zones.size());
  int current = -1;

  for (const auto &i : zones) {
    if (time_zone == i.id)
      current = choices.size();

    choices.push_back({i.id});
  }

  const int picked =
    PickChoice(_("Time zone"),
               _("The time zone of the airfield you are flying at."),
               choices, current);
  if (picked < 0 || picked == current)
    return;

  time_zone = zones[picked].id;
  Refresh();
}

void
TimeConfigPanel::PickManualUTCOffset() noexcept
{
  /* one choice per quarter hour */
  int value = manual_utc_offset.AsSeconds() / 60;
  if (!PickNumber(_("Manual UTC offset"),
                  _("The UTC offset field allows the UTC local time offset to be specified. It keeps "
                    "the value you entered even while another local time source is selected. The "
                    "local time is displayed below, along with the UTC offset which is currently in "
                    "effect."),
                  duration_cast<minutes>(Profile::MIN_UTC_OFFSET).count(),
                  duration_cast<minutes>(Profile::MAX_UTC_OFFSET).count(),
                  UTC_OFFSET_STEP.count(), value,
                  [](StaticString<32> &s, int v){
                    FormatUTCOffset(s,
                                    RoughTimeDelta::FromDuration(minutes{v}));
                  }))
    return;

  manual_utc_offset = RoughTimeDelta::FromDuration(minutes{value});
  manual_utc_offset_modified = true;
  Refresh();
}

void
TimeConfigPanel::AddLocalTimeItem() noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();

  /* without a GPS fix, the blackboard holds no time at all, and the
     preview would show the UTC offset instead of a time of day */
  const auto time = basic.time_available
    ? basic.time
    : TimeStamp{BrokenDateTime::NowUTC().DurationSinceMidnight()};

  /* the offset which is actually in effect goes with the local time:
     the item above shows the value of the source it belongs to, which
     is not the one in use unless that source is selected */
  const RoughTimeDelta utc_offset = GetUTCOffset(local_time_source);

  StaticString<32> offset;
  FormatUTCOffset(offset, utc_offset);

  StaticString<64> buffer;
  buffer.Format("%s (%s)", FormatLocalTimeHHMM(time, utc_offset).c_str(),
                offset.c_str());

  AddItem(_("Local time"), {.value = buffer.c_str()});
}

void
TimeConfigPanel::Fill() noexcept
{
  AddGroup();

  AddItem(_("Local time source"), [this](){ PickLocalTimeSource(); },
          {.value = local_time_source == LocalTimeSource::MANUAL_UTC_OFFSET
           ? GetEnumCaption(manual_utc_offset_list,
                            (unsigned)local_time_source)
           : GetEnumCaption(local_time_source_list,
                            (unsigned)local_time_source),
           .chevron = true});

  AddItem(_("Time zone"), [this](){ PickTimeZone(); },
          {.value = time_zone.c_str(),
           .chevron = true,
           .disabled = local_time_source != LocalTimeSource::TIME_ZONE});

  /* the offset keeps what the user entered, whichever source is
     selected; the one in effect is shown with the local time */
  StaticString<32> offset;
  FormatUTCOffset(offset, manual_utc_offset);

  AddItem(_("Manual UTC offset"), [this](){ PickManualUTCOffset(); },
          {.value = offset.c_str(),
           .chevron = true,
           .disabled =
           local_time_source != LocalTimeSource::MANUAL_UTC_OFFSET});

  AddLocalTimeItem();

  if (IsExpert()) {
    AddGroup();

    AddToggleItem(_("Use GPS time"),
                  _("If enabled sets the clock of the computer to the GPS time once a fix "
                    "is set. This is only necessary if your computer does not have a "
                    "real-time clock with battery backup or your computer frequently runs "
                    "out of battery power or otherwise loses time."),
                  set_system_time_from_gps);
  }
}

void
TimeConfigPanel::Show(const PixelRect &rc) noexcept
{
  ConfigListPanel::Show(rc);

  /* the local time is a clock, and the dialog may stay open for a
     while: without this, it would keep showing the time the page was
     opened */
  Refresh();
  local_time_timer.Schedule(seconds{1});
}

void
TimeConfigPanel::Hide() noexcept
{
  local_time_timer.Cancel();

  ConfigListPanel::Hide();
}

bool
TimeConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();

  if (settings_computer.local_time_source != local_time_source) {
    settings_computer.local_time_source = local_time_source;
    changed = true;
  }

  changed |= Profile::Update(ProfileKeys::TimeZone,
                             settings_computer.time_zone, time_zone);

  /* the source is written even if it did not change: without this key, a
     stored UTC offset means "manual" to Profile::Load(), because that
     is what it meant in older versions */
  Profile::SetEnum(ProfileKeys::LocalTimeSource,
                   settings_computer.local_time_source);

  if (settings_computer.local_time_source ==
      LocalTimeSource::MANUAL_UTC_OFFSET) {
    if (manual_utc_offset != settings_computer.utc_offset) {
      settings_computer.utc_offset = manual_utc_offset;
      changed = true;
    }
  } else {
    /* with the automatic sources, the UTC offset is owned by
       UTCOffsetProcessTimer(); apply it right away instead of the
       (disabled) form value, so the change is visible immediately */
    if (const auto new_utc_offset = settings_computer.GetCurrentUTCOffset();
        new_utc_offset != settings_computer.utc_offset) {
      settings_computer.utc_offset = new_utc_offset;
      changed = true;
    }
  }

  if (settings_computer.local_time_source ==
      LocalTimeSource::MANUAL_UTC_OFFSET ||
      manual_utc_offset_modified) {
    /* remember the manual offset even while another source is active, so
       the user does not have to enter it again */
    Profile::Set(ProfileKeys::UTCOffsetSigned, manual_utc_offset.AsSeconds());
    manual_utc_offset_modified = false;
    changed = true;
  }

  changed |= Profile::Update(ProfileKeys::SetSystemTimeFromGPS,
                             settings_computer.set_system_time_from_gps,
                             set_system_time_from_gps);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateTimeConfigPanel()
{
  return std::make_unique<TimeConfigPanel>();
}
