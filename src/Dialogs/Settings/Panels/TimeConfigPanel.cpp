// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TimeConfigPanel.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/Time.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"

using namespace std::chrono;

enum ControlIndex {
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
TimeConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(UTCOffset, df)) {
    const auto &tdf = static_cast<const DataFieldTime &>(df);
    SetLocalTime(RoughTimeDelta::FromDuration(tdf.GetValue()));
  }
}

void
TimeConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  const RoughTimeDelta utc_offset = settings_computer.utc_offset;
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

  const auto ival = GetValueTime(UTCOffset);
  if (const auto new_utc_offset = RoughTimeDelta::FromDuration(ival);
      new_utc_offset != settings_computer.utc_offset) {
    settings_computer.utc_offset = new_utc_offset;

    Profile::Set(ProfileKeys::UTCOffsetSigned, ival);
    changed = true;
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
