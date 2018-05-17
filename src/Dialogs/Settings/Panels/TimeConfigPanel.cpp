/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "TimeConfigPanel.hpp"
#include "Form/DataField/Listener.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"

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
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;

private:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;
};

void
TimeConfigPanel::SetLocalTime(RoughTimeDelta utc_offset)
{
  SetText(LocalTime,
          FormatLocalTimeHHMM((int)CommonInterface::Basic().time, utc_offset));
}

void
TimeConfigPanel::OnModified(DataField &df)
{
  if (IsDataField(UTCOffset, df))
    SetLocalTime(RoughTimeDelta::FromSeconds(df.GetAsInteger()));
}

void
TimeConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  const RoughTimeDelta utc_offset = settings_computer.utc_offset;
  AddTime(_("UTC offset"),
          _("The UTC offset field allows the UTC local time offset to be specified.  The local "
            "time is displayed below in order to make it easier to verify the correct offset "
            "has been entered."),
           -13 * 60 * 60, 13  * 60 * 60, 30 * 60,
          utc_offset.AsSeconds(), 2, this);

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
TimeConfigPanel::Save(bool &_changed)
{
  bool changed = false;

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();

  int ival = GetValueInteger(UTCOffset);
  const RoughTimeDelta new_utc_offset = RoughTimeDelta::FromSeconds(ival);
  if (new_utc_offset != settings_computer.utc_offset) {
    settings_computer.utc_offset = new_utc_offset;

    Profile::Set(ProfileKeys::UTCOffsetSigned, ival);
    changed = true;
  }

  changed |= SaveValue(SystemTimeFromGPS, ProfileKeys::SetSystemTimeFromGPS,
                       settings_computer.set_system_time_from_gps);

  _changed |= changed;

  return true;
}

Widget *
CreateTimeConfigPanel()
{
  return new TimeConfigPanel();
}
