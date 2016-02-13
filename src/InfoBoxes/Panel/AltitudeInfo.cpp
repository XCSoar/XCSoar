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

#include "AltitudeInfo.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Formatter/UserUnits.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Blackboard/BlackboardListener.hpp"

class AltitudeInfoPanel : public TwoWidgets, NullBlackboardListener {
public:
  AltitudeInfoPanel():TwoWidgets(false) {}

  void Refresh();

  virtual void Initialise(ContainerWindow &parent,
                          const PixelRect &rc) override;
  virtual void Show(const PixelRect &rc) override;
  virtual void Hide() override;

  virtual void OnGPSUpdate(const MoreData &basic) override;
};

void
AltitudeInfoPanel::Refresh()
{
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const NMEAInfo &basic = CommonInterface::Basic();

  RowFormWidget &first = (RowFormWidget &)GetFirst();
  RowFormWidget &second = (RowFormWidget &)GetSecond();

  second.SetText(0, calculated.altitude_agl_valid
                 ? FormatUserAltitude(calculated.altitude_agl).c_str()
                 : _("N/A"));

  first.SetText(1, basic.baro_altitude_available
                ? FormatUserAltitude(basic.baro_altitude).c_str()
                : _("N/A"));

  first.SetText(0, basic.gps_altitude_available
                ? FormatUserAltitude(basic.gps_altitude).c_str()
                : _("N/A"));

  second.SetText(1, calculated.terrain_valid
                 ? FormatUserAltitude(calculated.terrain_altitude).c_str()
                 : _("N/A"));
}

void
AltitudeInfoPanel::Initialise(ContainerWindow &parent, const PixelRect &rc)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  RowFormWidget *first = new RowFormWidget(look);
  RowFormWidget *second = new RowFormWidget(look);
  TwoWidgets::Set(first, second);
  TwoWidgets::Initialise(parent, rc);

  first->AddReadOnly(_("Alt GPS"));
  first->AddReadOnly(_("Alt Baro"));
  second->AddReadOnly(_("H AGL"));
  second->AddReadOnly(_("Terrain"));
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
