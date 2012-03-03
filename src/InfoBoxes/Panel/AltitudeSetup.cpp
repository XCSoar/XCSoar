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

#include "AltitudeSetup.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Simulator.hpp"
#include "DataField/Float.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Form/Util.hpp"
#include "Form/Edit.hpp"
#include "Form/TabBar.hpp"
#include "Form/XMLWidget.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Operation/MessageOperationEnvironment.hpp"

class WndButton;

class AltitudeSetupPanel : public XMLWidget {
  unsigned id;

public:
  AltitudeSetupPanel(unsigned _id):id(_id) {}

  void Setup();

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual void Show(const PixelRect &rc);
};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static AltitudeSetupPanel *instance;

static void
PnlSetupOnQNH(DataField *_Sender, DataField::DataAccessMode Mode)
{
  DataFieldFloat *Sender = (DataFieldFloat *)_Sender;
  ComputerSettings &settings_computer =
    CommonInterface::SetComputerSettings();

  switch (Mode) {
  case DataField::daChange:
    settings_computer.pressure = Units::FromUserPressure(Sender->GetAsFixed());
    settings_computer.pressure_available.Update(CommonInterface::Basic().clock);

    {
      MessageOperationEnvironment env;
      device_blackboard->SetQNH(Units::FromUserPressure(Sender->GetAsFixed()),
                                env);
    }
    break;

  case DataField::daSpecial:
    return;
  }
}

void
AltitudeSetupPanel::Setup()
{
  InfoBoxManager::ShowInfoBoxPicker(id);
  dlgInfoBoxAccess::OnClose();
}

static void
PnlSetupOnSetup(gcc_unused WndButton &Sender)
{
  instance->Setup();
}

static gcc_constexpr_data
CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(PnlSetupOnQNH),
  DeclareCallBackEntry(PnlSetupOnSetup),
  DeclareCallBackEntry(NULL)
};

void
AltitudeSetupPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(CallBackTable, parent, _T("IDR_XML_INFOBOXALTITUDESETUP"));
}

void
AltitudeSetupPanel::Show(const PixelRect &rc)
{
  LoadFormProperty(form, _T("prpQNH"),
                   Units::ToUserPressure(CommonInterface::GetComputerSettings().pressure));

  WndProperty* wp;
  wp = (WndProperty*)form.FindByName(_T("prpQNH"));
  if (wp) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();

    df.SetMin(Units::ToUserPressure(Units::ToSysUnit(fixed(850), Unit::HECTOPASCAL)));
    df.SetMax(Units::ToUserPressure(Units::ToSysUnit(fixed(1300), Unit::HECTOPASCAL)));
    df.SetStep(Units::ToUserPressure(Units::ToSysUnit(fixed_one, Unit::HECTOPASCAL)));
    df.SetUnits(Units::GetPressureName());
    df.SetStep(GetUserPressureStep());
    df.SetFormat(GetUserPressureFormat());
    wp->RefreshDisplay();
}

  XMLWidget::Show(rc);
}

Widget *
LoadAltitudeSetupPanel(unsigned id)
{
  return instance = new AltitudeSetupPanel(id);
}
