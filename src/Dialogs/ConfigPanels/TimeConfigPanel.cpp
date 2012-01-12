/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "DataField/Float.hpp"
#include "Form/Form.hpp"
#include "Form/Frame.hpp"
#include "Units/UnitsFormatter.hpp"
#include "LocalTime.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Form/RowFormWidget.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  UTCOffset,
  LocalTime
};

class TimeConfigPanel : public RowFormWidget {
public:
  TimeConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook(), Layout::Scale(150)) {}

private:
  bool loading;

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  void SetLocalTime(int utc_offset);
};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static TimeConfigPanel *instance;

void
TimeConfigPanel::SetLocalTime(int utc_offset)
{
  TCHAR temp[20];
  int time(XCSoarInterface::Basic().time);
  Units::TimeToTextHHMMSigned(temp, TimeLocal(time, utc_offset));

  WndProperty &wp = GetControl(LocalTime);

  wp.SetText(temp);
  wp.RefreshDisplay();
}

static void
OnUTCData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch(Mode) {
  case DataField::daChange:
  {
    DataFieldFloat &df = *(DataFieldFloat *)Sender;
    int ival = iround(df.GetAsFixed() * 3600);
    instance->SetLocalTime(ival);
    break;
  }
  case DataField::daSpecial:
    return;
  }
}


void
TimeConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  instance = this;

  RowFormWidget::Prepare(parent, rc);

  int utc_offset = XCSoarInterface::GetComputerSettings().utc_offset;
  AddFloat(_("UTC offset"),
           _("The UTC offset field allows the UTC local time offset to be specified.  The local "
               "time is displayed below in order to make it easier to verify the correct offset "
               "has been entered."),
           _T("%.1f h"), _T("%.1f h"), fixed(-13), fixed(13), fixed(0.5), false,
           fixed(iround(fixed(utc_offset) / 1800)) / 2, OnUTCData);
#ifdef WIN32
  if (IsEmbedded() && !IsAltair())
    GetControl(UTCOffset).set_enabled(false);
#endif

  Add(_("Local time"), 0, true);
  SetLocalTime(utc_offset);
}

bool
TimeConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  ComputerSettings &settings_computer = XCSoarInterface::SetComputerSettings();

  fixed tmp_ival;
  SaveValue(UTCOffset, tmp_ival);
  int ival = iround(tmp_ival * 3600);

  if (settings_computer.utc_offset != ival) {
    settings_computer.utc_offset = ival;

    Profile::Set(szProfileUTCOffsetSigned, ival);
    changed = true;
  }
  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateTimeConfigPanel()
{
  return new TimeConfigPanel();
}
