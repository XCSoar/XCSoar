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

#include "AltitudeInfo.hpp"
#include "Screen/Layout.hpp"
#include "Util/Macros.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Formatter/UserUnits.hpp"
#include "Form/TwoWidgets.hpp"
#include "Form/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Blackboard/BlackboardListener.hpp"

class AltitudeInfoPanel : public TwoWidgets, NullBlackboardListener {
public:
  AltitudeInfoPanel():TwoWidgets(false) {}

  void Refresh();

  virtual void Initialise(ContainerWindow &parent,
                          const PixelRect &rc) gcc_override;
  virtual void Show(const PixelRect &rc) gcc_override;
  virtual void Hide() gcc_override;

  virtual void OnGPSUpdate(const MoreData &basic) gcc_override;
};

void
AltitudeInfoPanel::Refresh()
{
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const NMEAInfo &basic = CommonInterface::Basic();
  TCHAR sTmp[32];

  RowFormWidget &first = (RowFormWidget &)GetFirst();
  RowFormWidget &second = (RowFormWidget &)GetSecond();

  if (!calculated.altitude_agl_valid) {
    second.SetText(0, _("N/A"));
  } else {
    // Set Value
    FormatUserAltitude(calculated.altitude_agl, sTmp, ARRAY_SIZE(sTmp));
    second.SetText(0, sTmp);
  }

  if (!basic.baro_altitude_available) {
    first.SetText(1, _("N/A"));
  } else {
    // Set Value
    FormatUserAltitude(basic.baro_altitude, sTmp, ARRAY_SIZE(sTmp));
    first.SetText(1, sTmp);
  }

  if (!basic.gps_altitude_available) {
    first.SetText(0, _("N/A"));
  } else {
    // Set Value
    FormatUserAltitude(basic.gps_altitude, sTmp, ARRAY_SIZE(sTmp));
    first.SetText(0, sTmp);
  }

  if (!calculated.terrain_valid){
    second.SetText(1, _("N/A"));
  } else {
    // Set Value
    FormatUserAltitude(calculated.terrain_altitude,
                              sTmp, ARRAY_SIZE(sTmp));
    second.SetText(1, sTmp);
  }
}

void
AltitudeInfoPanel::Initialise(ContainerWindow &parent, const PixelRect &rc)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  RowFormWidget *first = new RowFormWidget(look);
  RowFormWidget *second = new RowFormWidget(look);
  TwoWidgets::Set(first, second);
  TwoWidgets::Initialise(parent, rc);

  first->AddReadOnly(_("H GPS"));
  first->AddReadOnly(_("H Baro"));
  second->AddReadOnly(_("H AGL"));
  second->AddReadOnly(_("H GND"));
}

void
AltitudeInfoPanel::Show(const PixelRect &rc)
{
  Refresh();
  TwoWidgets::Show(rc);

  CommonInterface::GetLiveBlackboard().AddListener(*this);
}

void
AltitudeInfoPanel::Hide()
{
  CommonInterface::GetLiveBlackboard().RemoveListener(*this);

  TwoWidgets::Hide();
}

void
AltitudeInfoPanel::OnGPSUpdate(const MoreData &basic)
{
  Refresh();
}

Widget *
LoadAltitudeInfoPanel(unsigned id)
{
  return new AltitudeInfoPanel();
}
