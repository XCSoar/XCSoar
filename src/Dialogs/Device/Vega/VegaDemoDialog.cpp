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

#include "VegaDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Device/device.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/Float.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Time/PeriodClock.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "Math/Util.hpp"

static double VegaDemoW = 0;
static double VegaDemoV = 0;
static bool VegaDemoAudioClimb = true;

static void
VegaWriteDemo()
{
  static PeriodClock last_time;
  if (!last_time.CheckUpdate(250))
    return;

  TCHAR dbuf[100];
  _stprintf(dbuf, _T("PDVDD,%d,%d"),
            iround(VegaDemoW * 10),
            iround(VegaDemoV * 10));

  PopupOperationEnvironment env;
  VarioWriteNMEA(dbuf, env);
}

class VegaDemoWidget final
  : public RowFormWidget, private DataFieldListener {
  enum Controls {
    VARIO,
    AIRSPEED,
    CIRCLING,
  };

public:
  VegaDemoWidget(const DialogLook &_look)
    :RowFormWidget(_look) {}

  /* virtual methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;

private:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;
};

void
VegaDemoWidget::OnModified(DataField &df)
{
  if (IsDataField(VARIO, df)) {
    const DataFieldFloat &dff = (const DataFieldFloat &)df;
    VegaDemoW = Units::ToSysVSpeed(dff.GetAsFixed());
  } else if (IsDataField(AIRSPEED, df)) {
    const DataFieldFloat &dff = (const DataFieldFloat &)df;
    VegaDemoV = Units::ToSysSpeed(dff.GetAsFixed());
  } else if (IsDataField(CIRCLING, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    VegaDemoAudioClimb = dfb.GetAsBoolean();
  }

  VegaWriteDemo();
}

void
VegaDemoWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  AddFloat(_("TE vario"),
           _("This produces a fake TE vario gross vertical velocity.  It can be used when in circling mode to demonstrate the lift tones.  When not in circling mode, set this to a realistic negative value so speed command tones are produced."),
           _T("%.1f %s"), _T("%.1f"),
           Units::ToUserVSpeed(-20), Units::ToUserVSpeed(20),
           GetUserVerticalSpeedStep(),
           false, UnitGroup::VERTICAL_SPEED, VegaDemoW, this);

  AddFloat(_("Airspeed"),
           _("This produces a fake airspeed.  It can be used when not in circling mode to demonstrate the speed command tones."),
           _T("%.0f %s"), _T("%.0f"), 0, 200, 2,
           false, UnitGroup::HORIZONTAL_SPEED, VegaDemoV, this);

  AddBoolean(_("Circling"),
             _("This forces the variometer into circling or cruise mode"),
             VegaDemoAudioClimb, this);
}

void
dlgVegaDemoShowModal()
{
  PopupOperationEnvironment env;
  VarioWriteNMEA(_T("PDVSC,S,DemoMode,0"), env);
  VarioWriteNMEA(_T("PDVSC,S,DemoMode,3"), env);

  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(look);
  VegaDemoWidget widget(look);
  dialog.CreateAuto(UIGlobals::GetMainWindow(), _("Vario Demo"), &widget);
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
  dialog.StealWidget();

  // deactivate demo.
  VarioWriteNMEA(_T("PDVSC,S,DemoMode,0"), env);
}
