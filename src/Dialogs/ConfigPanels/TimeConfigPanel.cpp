/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Form/DataField/Float.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/Form.hpp"
#include "Form/Frame.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "LocalTime.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Form/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Asset.hpp"

enum ControlIndex {
  UTCOffset,
  LocalTime,
  SystemTimeFromGPS
};

class TimeConfigPanel
  : public RowFormWidget, DataFieldListener {
public:
  TimeConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void SetLocalTime(int utc_offset);

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);

private:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df);
};

void
TimeConfigPanel::SetLocalTime(int utc_offset)
{
  TCHAR temp[20];
  int time(XCSoarInterface::Basic().time);
  FormatSignedTimeHHMM(temp, TimeLocal(time, utc_offset));

  WndProperty &wp = GetControl(LocalTime);

  wp.SetText(temp);
  wp.RefreshDisplay();
}

void
TimeConfigPanel::OnModified(DataField &df)
{
  if (IsDataField(UTCOffset, df))
    SetLocalTime(df.GetAsInteger());
}

void
TimeConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  const ComputerSettings &settings_computer = XCSoarInterface::GetComputerSettings();

  int utc_offset = settings_computer.utc_offset;
  AddTime(_("UTC offset"),
          _("The UTC offset field allows the UTC local time offset to be specified.  The local "
            "time is displayed below in order to make it easier to verify the correct offset "
            "has been entered."),
           -13 * 60 * 60, 13  * 60 * 60, 30 * 60, utc_offset, 2, this);

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
TimeConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  ComputerSettings &settings_computer = XCSoarInterface::SetComputerSettings();

  int ival = GetValueInteger(UTCOffset);
  if (settings_computer.utc_offset != ival) {
    settings_computer.utc_offset = ival;

    Profile::Set(ProfileKeys::UTCOffsetSigned, ival);
    changed = true;
  }

  changed |= SaveValue(SystemTimeFromGPS, ProfileKeys::SetSystemTimeFromGPS,
                       settings_computer.set_system_time_from_gps);

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateTimeConfigPanel()
{
  return new TimeConfigPanel();
}
