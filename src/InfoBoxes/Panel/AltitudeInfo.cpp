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

#include "AltitudeInfo.hpp"
#include "Util/Macros.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Simulator.hpp"
#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Form/Util.hpp"
#include "Form/TabBar.hpp"
#include "Form/Form.hpp"
#include "Form/XMLWidget.hpp"

class AltitudeInfoPanel : public XMLWidget {
public:
  void Refresh();

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static AltitudeInfoPanel *instance;

void
AltitudeInfoPanel::Refresh()
{
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const NMEAInfo &basic = CommonInterface::Basic();
  TCHAR sTmp[32];

  if (!calculated.altitude_agl_valid) {
    SetFormValue(form, _T("prpAltAGL"), _("N/A"));
  } else {
    // Set Value
    Units::FormatUserAltitude(calculated.altitude_agl, sTmp, ARRAY_SIZE(sTmp));
    SetFormValue(form, _T("prpAltAGL"), sTmp);
  }

  if (!basic.baro_altitude_available) {
    SetFormValue(form, _T("prpAltBaro"), _("N/A"));
  } else {
    // Set Value
    Units::FormatUserAltitude(basic.baro_altitude, sTmp, ARRAY_SIZE(sTmp));
    SetFormValue(form, _T("prpAltBaro"), sTmp);
  }

  if (!basic.gps_altitude_available) {
    SetFormValue(form, _T("prpAltGPS"), _("N/A"));
  } else {
    // Set Value
    Units::FormatUserAltitude(basic.gps_altitude, sTmp, ARRAY_SIZE(sTmp));
    SetFormValue(form, _T("prpAltGPS"), sTmp);
  }

  if (!calculated.terrain_valid){
    SetFormValue(form, _T("prpTerrain"), _("N/A"));
  } else {
    // Set Value
    Units::FormatUserAltitude(calculated.terrain_altitude,
                              sTmp, ARRAY_SIZE(sTmp));
    SetFormValue(form, _T("prpTerrain"), sTmp);
  }
}

static void
OnTimerNotify(gcc_unused WndForm &Sender)
{
  instance->Refresh();
}

void
AltitudeInfoPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent, _T("IDR_XML_INFOBOXALTITUDEINFO"));
}

void
AltitudeInfoPanel::Show(const PixelRect &rc)
{
  WndForm *wf = dlgInfoBoxAccess::GetWindowForm();
  wf->SetTimerNotify(OnTimerNotify);
  Refresh();
  XMLWidget::Show(rc);
}

void
AltitudeInfoPanel::Hide()
{
  XMLWidget::Hide();

  WndForm *wf = dlgInfoBoxAccess::GetWindowForm();
  wf->SetTimerNotify(NULL);
}

Widget *
LoadAltitudeInfoPanel(unsigned id)
{
  return instance = new AltitudeInfoPanel();
}
