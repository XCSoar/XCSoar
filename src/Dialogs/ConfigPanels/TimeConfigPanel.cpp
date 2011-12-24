/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "DataField/Enum.hpp"
#include "DataField/ComboList.hpp"
#include "DataField/Float.hpp"
#include "Form/Form.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Form/Frame.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Units/UnitsFormatter.hpp"
#include "LocalTime.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Asset.hpp"
#include "Language/Language.hpp"
#include "DataField/Base.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"

class TimeConfigPanel : public XMLWidget {
private:
  bool loading;

public:
  TimeConfigPanel() : loading(false) {
  }
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
  void SetLocalTime(int utc_offset);
};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static TimeConfigPanel *instance;

void
TimeConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}

void
TimeConfigPanel::Hide()
{
  XMLWidget::Hide();
}

void
TimeConfigPanel::SetLocalTime(int utc_offset)
{
  WndProperty* wp;
  TCHAR temp[20];
  int time(XCSoarInterface::Basic().time);
  Units::TimeToTextHHMMSigned(temp, TimeLocal(time, utc_offset));

  wp = (WndProperty*)form.FindByName(_T("prpLocalTime"));
  assert(wp != NULL);

  wp->SetText(temp);
  wp->RefreshDisplay();
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

gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnUTCData),
  DeclareCallBackEntry(NULL)
};

void
TimeConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  instance = this;
  LoadWindow(CallBackTable, parent,
             Layout::landscape ? _T("IDR_XML_TIMECONFIGPANEL") :
                               _T("IDR_XML_TIMECONFIGPANEL_L"));

  loading = true;

  int utc_offset = XCSoarInterface::GetComputerSettings().utc_offset;
  LoadFormProperty(form, _T("prpUTCOffset"),
                   fixed(iround(fixed(utc_offset) / 1800)) / 2);
#ifdef WIN32
  if (IsEmbedded() && !IsAltair())
    ((WndProperty*)form.FindByName(_T("prpUTCOffset")))->set_enabled(false);
#endif
  SetLocalTime(utc_offset);

  loading = false;
}

bool
TimeConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  ComputerSettings &settings_computer = XCSoarInterface::SetComputerSettings();
  int ival = iround(GetFormValueFixed(form, _T("prpUTCOffset")) * 3600);
  if (settings_computer.utc_offset != ival) {
    settings_computer.utc_offset = ival;

    // have to do this because registry variables can't be negative!
    if (ival < 0)
      ival += 24 * 3600;

    Profile::Set(szProfileUTCOffset, ival);
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
