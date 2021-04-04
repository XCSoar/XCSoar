/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
  explicit AltitudeInfoPanel(const DialogLook &look) noexcept
    :TwoWidgets(std::make_unique<RowFormWidget>(look),
                std::make_unique<RowFormWidget>(look),
                false) {}

  void Refresh() noexcept;

  void Initialise(ContainerWindow &parent,
                  const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

  void OnGPSUpdate(const MoreData &basic) override;
};

void
AltitudeInfoPanel::Refresh() noexcept
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
AltitudeInfoPanel::Initialise(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  TwoWidgets::Initialise(parent, rc);

  RowFormWidget &first = (RowFormWidget &)GetFirst();
  first.AddReadOnly(_("Alt GPS"));
  first.AddReadOnly(_("Alt Baro"));

  RowFormWidget &second = (RowFormWidget &)GetSecond();
  second.AddReadOnly(_("H AGL"));
  second.AddReadOnly(_("Terrain"));
}

void
AltitudeInfoPanel::Show(const PixelRect &rc) noexcept
{
  Refresh();
  TwoWidgets::Show(rc);

  CommonInterface::GetLiveBlackboard().AddListener(*this);
}

void
AltitudeInfoPanel::Hide() noexcept
{
  CommonInterface::GetLiveBlackboard().RemoveListener(*this);

  TwoWidgets::Hide();
}

void
AltitudeInfoPanel::OnGPSUpdate(const MoreData &basic)
{
  Refresh();
}

std::unique_ptr<Widget>
LoadAltitudeInfoPanel(unsigned id)
{
  return std::make_unique<AltitudeInfoPanel>(UIGlobals::GetDialogLook());
}
