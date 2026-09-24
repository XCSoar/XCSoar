// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TimeConfigPanel.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/Time.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Profile/ComputerProfile.hpp"
#include "Profile/Current.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "time/BrokenDateTime.hpp"
#include "time/SystemTimeZone.hpp"
#include "time/TimeZones.hpp"
#include "util/StaticString.hxx"

#include <cstdlib>

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

enum ControlIndex {
  LOCAL_TIME_SOURCE,
  TIME_ZONE,
  UTC_OFFSET,
  LOCAL_TIME,
  SYSTEM_TIME_FROM_GPS
};

class TimeConfigPanel final
  : public RowFormWidget, DataFieldListener {
  RoughTimeDelta manual_utc_offset;
  bool manual_utc_offset_modified = false;

  /** is #manual_utc_offset_list part of the #LOCAL_TIME_SOURCE field? */
  bool manual_utc_offset_offered;

  UI::PeriodicTimer local_time_timer{[this]{ UpdateLocalTime(); }};

public:
  TimeConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void SetLocalTime(RoughTimeDelta utc_offset);

  /**
   * Enable the fields which the given source uses, and show the UTC
   * offset it currently yields.
   */
  void UpdateLocalTimeSource(LocalTimeSource source);

  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /**
   * Recalculate the local time for the source which is selected in the
   * form.
   */
  void UpdateLocalTime() noexcept {
    SetLocalTime(GetUTCOffset(GetLocalTimeSource()));
  }

  /**
   * Add or remove the manual UTC offset, which is an expert setting.
   */
  void UpdateSourceChoices() noexcept;

  /**
   * Returns the time zone which is selected in the form.
   */
  [[gnu::pure]]
  const char *GetTimeZone() const noexcept;

  /**
   * Calculate the UTC offset which the given source currently yields.
   */
  [[gnu::pure]]
  RoughTimeDelta GetUTCOffset(LocalTimeSource source) const noexcept;

  [[gnu::pure]]
  LocalTimeSource GetLocalTimeSource() const noexcept {
    return (LocalTimeSource)
      ((const DataFieldEnum &)GetDataField(LOCAL_TIME_SOURCE)).GetValue();
  }

  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

const char *
TimeConfigPanel::GetTimeZone() const noexcept
{
  const char *id =
    ((const DataFieldEnum &)GetDataField(TIME_ZONE)).GetAsString();
  return id != nullptr ? id : "UTC";
}

RoughTimeDelta
TimeConfigPanel::GetUTCOffset(LocalTimeSource source) const noexcept
{
  switch (source) {
  case LocalTimeSource::AUTOMATIC:
    return RoughTimeDelta::FromSeconds(GetCurrentTimeZoneOffset());

  case LocalTimeSource::TIME_ZONE:
    if (const auto offset = FindTimeZoneOffset(GetTimeZone(),
                                               system_clock::now()))
      return RoughTimeDelta::FromSeconds(offset->count());

    return RoughTimeDelta::FromSeconds(0);

  case LocalTimeSource::MANUAL_UTC_OFFSET:
    break;
  }

  return manual_utc_offset;
}

void
TimeConfigPanel::SetLocalTime(RoughTimeDelta utc_offset)
{
  const NMEAInfo &basic = CommonInterface::Basic();

  /* without a GPS fix, the blackboard holds no time at all, and the
     preview would show the UTC offset instead of a time of day */
  const auto time = basic.time_available
    ? basic.time
    : TimeStamp{BrokenDateTime::NowUTC().DurationSinceMidnight()};

  /* the offset which is actually in effect goes with the local time:
     the field above shows the value of the source it belongs to, which
     is not the one in use unless that source is selected */
  const int seconds = utc_offset.AsSeconds();
  StaticString<32> buffer;
  buffer.Format("%s (UTC%c%s)",
                FormatLocalTimeHHMM(time, utc_offset).c_str(),
                seconds < 0 ? '-' : '+',
                FormatSignedTimeHHMM(std::chrono::seconds{std::abs(seconds)}).c_str());

  SetText(LOCAL_TIME, buffer);
}

void
TimeConfigPanel::UpdateLocalTimeSource(LocalTimeSource source)
{
  SetRowEnabled(TIME_ZONE, source == LocalTimeSource::TIME_ZONE);
  SetRowEnabled(UTC_OFFSET, source == LocalTimeSource::MANUAL_UTC_OFFSET);

  /* the field keeps what the user entered, whichever source is
     selected; the offset which is in effect is shown with the local
     time below */
  LoadValueDuration(UTC_OFFSET, manual_utc_offset.ToDuration());

  SetLocalTime(GetUTCOffset(source));
}

void
TimeConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(UTC_OFFSET, df)) {
    const auto &tdf = static_cast<const DataFieldTime &>(df);
    manual_utc_offset = RoughTimeDelta::FromDuration(tdf.GetValue());
    manual_utc_offset_modified = true;
    SetLocalTime(manual_utc_offset);
  } else if (IsDataField(LOCAL_TIME_SOURCE, df) || IsDataField(TIME_ZONE, df)) {
    UpdateLocalTimeSource(GetLocalTimeSource());
  }
}

void
TimeConfigPanel::UpdateSourceChoices() noexcept
{
  auto &df = (DataFieldEnum &)GetDataField(LOCAL_TIME_SOURCE);
  const auto source = (LocalTimeSource)df.GetValue();

  const bool offer = UIGlobals::GetDialogSettings().expert ||
    source == LocalTimeSource::MANUAL_UTC_OFFSET;
  if (offer == manual_utc_offset_offered)
    return;

  manual_utc_offset_offered = offer;

  df.ClearChoices();
  df.AddChoices(local_time_source_list);
  if (offer)
    df.AddChoices(manual_utc_offset_list);

  df.SetValue(source);
  GetControl(LOCAL_TIME_SOURCE).RefreshDisplay();
}

void
TimeConfigPanel::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);

  /* the local time is a clock, and the dialog may stay open for a
     while: without this, it would keep showing the time the page was
     opened */
  UpdateLocalTime();
  local_time_timer.Schedule(seconds{1});
}

void
TimeConfigPanel::Hide() noexcept
{
  local_time_timer.Cancel();

  RowFormWidget::Hide();
}

void
TimeConfigPanel::Move(const PixelRect &rc) noexcept
{
  RowFormWidget::Move(rc);

  /* toggling "Expert" in the configuration dialog only forces a layout
     update, so this is where the choice list has to follow */
  UpdateSourceChoices();
}

void
TimeConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  manual_utc_offset = settings_computer.utc_offset;
  Profile::LoadUTCOffset(Profile::map, manual_utc_offset);

  auto local_time_source = settings_computer.local_time_source;

#ifdef KOBO
  if (local_time_source == LocalTimeSource::AUTOMATIC)
    /* the profile was written on another platform */
    local_time_source = LocalTimeSource::TIME_ZONE;
#endif

  WndProperty *wp = AddEnum(_("Local time source"),
                            _("Selects where XCSoar gets the offset between "
                              "UTC and local time from."),
                            this);
  {
    auto &df = *(DataFieldEnum *)wp->GetDataField();
    df.EnableItemHelp(true);
    df.AddChoices(local_time_source_list);

    manual_utc_offset_offered = UIGlobals::GetDialogSettings().expert ||
      local_time_source == LocalTimeSource::MANUAL_UTC_OFFSET;
    if (manual_utc_offset_offered)
      df.AddChoices(manual_utc_offset_list);

    df.SetValue(local_time_source);
    wp->RefreshDisplay();
  }

  wp = AddEnum(_("Time zone"),
                            _("The time zone of the airfield you are flying "
                              "at."),
                            this);
  {
    auto &df = *(DataFieldEnum *)wp->GetDataField();

    for (const auto &i : GetTimeZones())
      df.addEnumText(i.id);

    if (!df.SetValue(settings_computer.time_zone.c_str()))
      /* a time zone which is not in our table: fall back to UTC */
      df.SetValue("UTC");

    wp->RefreshDisplay();
  }

  AddDuration(_("Manual UTC offset"),
          _("The UTC offset field allows the UTC local time offset to be specified. It keeps "
            "the value you entered even while another local time source is selected. The "
            "local time is displayed below, along with the UTC offset which is currently in "
            "effect."),
              Profile::MIN_UTC_OFFSET,
              Profile::MAX_UTC_OFFSET,
              UTC_OFFSET_STEP,
              manual_utc_offset.ToDuration(),
              2, this);

  Add(_("Local time"), 0, true);

  UpdateLocalTimeSource(local_time_source);

  AddBoolean(_("Use GPS time"),
             _("If enabled sets the clock of the computer to the GPS time once a fix "
               "is set. This is only necessary if your computer does not have a "
               "real-time clock with battery backup or your computer frequently runs "
               "out of battery power or otherwise loses time."),
             settings_computer.set_system_time_from_gps);
  SetExpertRow(SYSTEM_TIME_FROM_GPS);
}

bool
TimeConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();

  changed |= SaveValueEnum(LOCAL_TIME_SOURCE, settings_computer.local_time_source);
  changed |= SaveValue(TIME_ZONE, ProfileKeys::TimeZone,
                       settings_computer.time_zone);

  /* the source is written even if it did not change: without this key, a
     stored UTC offset means "manual" to Profile::Load(), because that
     is what it meant in older versions */
  Profile::SetEnum(ProfileKeys::LocalTimeSource, settings_computer.local_time_source);

  if (settings_computer.local_time_source == LocalTimeSource::MANUAL_UTC_OFFSET) {
    const auto ival = GetValueTime(UTC_OFFSET);

    if (const auto new_utc_offset = RoughTimeDelta::FromDuration(ival);
        new_utc_offset != settings_computer.utc_offset) {
      settings_computer.utc_offset = new_utc_offset;
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

  if (settings_computer.local_time_source == LocalTimeSource::MANUAL_UTC_OFFSET ||
      manual_utc_offset_modified) {
    /* remember the manual offset even while another source is active, so
       the user does not have to enter it again */
    Profile::Set(ProfileKeys::UTCOffsetSigned, manual_utc_offset.AsSeconds());
    manual_utc_offset_modified = false;
    changed = true;
  }

  changed |= SaveValue(SYSTEM_TIME_FROM_GPS, ProfileKeys::SetSystemTimeFromGPS,
                       settings_computer.set_system_time_from_gps);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateTimeConfigPanel()
{
  return std::make_unique<TimeConfigPanel>();
}
