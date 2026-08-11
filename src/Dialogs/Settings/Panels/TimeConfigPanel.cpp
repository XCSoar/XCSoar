// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TimeConfigPanel.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/Time.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "time/SystemTimeZone.hpp"

using namespace std::chrono;

enum ControlIndex {
  AutoUTCOffset,
  UTCOffset,
  LocalTime,
  SystemTimeFromGPS
};

class TimeConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  TimeConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void SetLocalTime(RoughTimeDelta utc_offset);

  /**
   * Enable/disable the manual UTC offset field and, while the
   * automatic mode is enabled, show the offset we would use.
   */
  void UpdateAutoUTCOffset(bool automatic);

  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
TimeConfigPanel::SetLocalTime(RoughTimeDelta utc_offset)
{
  SetText(LocalTime,
          FormatLocalTimeHHMM(CommonInterface::Basic().time, utc_offset));
}

void
TimeConfigPanel::UpdateAutoUTCOffset(bool automatic)
{
  SetRowEnabled(UTCOffset, !automatic);

  if (automatic) {
    const auto utc_offset =
      RoughTimeDelta::FromSeconds(GetCurrentTimeZoneOffset());
    LoadValueDuration(UTCOffset, utc_offset.ToDuration());
    SetLocalTime(utc_offset);
  }
}

void
TimeConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(UTCOffset, df)) {
    const auto &tdf = static_cast<const DataFieldTime &>(df);
    SetLocalTime(RoughTimeDelta::FromDuration(tdf.GetValue()));
  } else if (IsDataField(AutoUTCOffset, df)) {
    UpdateAutoUTCOffset(static_cast<const DataFieldBoolean &>(df).GetValue());
  }
}

void
TimeConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  const RoughTimeDelta utc_offset = settings_computer.utc_offset;

  AddBoolean(_("Automatic UTC offset"),
             _("Use the time zone configured in the operating system, and keep "
               "following it across daylight saving time changes and when "
               "travelling to another time zone. Disable this to enter the UTC "
               "offset manually."),
             settings_computer.auto_utc_offset, this);

  AddDuration(_("UTC offset"),
          _("The UTC offset field allows the UTC local time offset to be specified. The local "
            "time is displayed below in order to make it easier to verify the correct offset "
            "has been entered."),
              hours{-13},
              hours{13},
              minutes{30},
              utc_offset.ToDuration(),
              2, this);

  Add(_("Local time"), 0, true);
  SetLocalTime(utc_offset);

  UpdateAutoUTCOffset(settings_computer.auto_utc_offset);

  AddBoolean(_("Use GPS time"),
             _("If enabled sets the clock of the computer to the GPS time once a fix "
               "is set. This is only necessary if your computer does not have a "
               "real-time clock with battery backup or your computer frequently runs "
               "out of battery power or otherwise loses time."),
             settings_computer.set_system_time_from_gps);
  SetExpertRow(SystemTimeFromGPS);
}

bool
TimeConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();

  changed |= SaveValue(AutoUTCOffset, ProfileKeys::AutoUTCOffset,
                       settings_computer.auto_utc_offset);

  if (settings_computer.auto_utc_offset) {
    /* in automatic mode, the UTC offset is owned by
       UTCOffsetProcessTimer(); apply it right away instead of the
       (disabled) form value, so the change is visible immediately */
    if (const auto new_utc_offset =
          RoughTimeDelta::FromSeconds(GetCurrentTimeZoneOffset());
        new_utc_offset != settings_computer.utc_offset) {
      settings_computer.utc_offset = new_utc_offset;
      changed = true;
    }
  } else {
    const auto ival = GetValueTime(UTCOffset);

    if (const auto new_utc_offset = RoughTimeDelta::FromDuration(ival);
        new_utc_offset != settings_computer.utc_offset) {
      settings_computer.utc_offset = new_utc_offset;
      changed = true;
    }

    /* always store the manual offset, so switching back from automatic
       mode restores what the user had configured */
    Profile::Set(ProfileKeys::UTCOffsetSigned, ival);
  }

  changed |= SaveValue(SystemTimeFromGPS, ProfileKeys::SetSystemTimeFromGPS,
                       settings_computer.set_system_time_from_gps);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateTimeConfigPanel()
{
  return std::make_unique<TimeConfigPanel>();
}
