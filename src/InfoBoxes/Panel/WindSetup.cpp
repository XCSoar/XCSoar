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

#include "WindSetup.hpp"
#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Form/Util.hpp"
#include "Form/TabBar.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Boolean.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Interface.hpp"
#include "Units/Group.hpp"
#include "Form/RowFormWidget.hpp"
#include "Form/DataField/Listener.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Screen/Layout.hpp"
#include "Look/DialogLook.hpp"

enum ControlIndex {
  AutoWind,
  TrailDrift,
  SetupButton,
};

class WindSetupPanel: public RowFormWidget, DataFieldListener {
public:
  unsigned id;

public:
  WindSetupPanel(unsigned _id)
    :RowFormWidget(UIGlobals::GetDialogLook()), id(_id) {}

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);

protected:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df);

private:
  void OnAutoWind(DataFieldEnum &Sender);
  void OnTrailDrift(DataFieldBoolean &Sender);
};

void
WindSetupPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const NMEAInfo &basic = XCSoarInterface::Basic();
  const WindSettings &settings = CommonInterface::GetComputerSettings().wind;
  const bool external_wind = basic.external_wind_available &&
    settings.use_external_wind;

  RowFormWidget::Prepare(parent, rc);

  if (external_wind) {
    static constexpr StaticEnumChoice external_wind_list[] = {
      { 0, N_("External") },
      { 0 }
    };
    AddEnum(_("Auto. wind"),
            _("This allows switching on or off the automatic wind algorithm.  When the algorithm is switched off, the pilot is responsible for setting the wind estimate.&#10;[Circling] Requires only a GPS source.&#10;[ZigZag] Requires an intelligent vario with airspeed output.&#10;[Both] Use ZigZag and circling."),
            external_wind_list, settings.GetLegacyAutoWindMode(), this);
  } else {

    static constexpr StaticEnumChoice auto_wind_list[] = {
      { AUTOWIND_NONE, N_("Manual") },
      { AUTOWIND_CIRCLING, N_("Circling") },
      { AUTOWIND_ZIGZAG, N_("ZigZag") },
      { AUTOWIND_CIRCLING | AUTOWIND_ZIGZAG, N_("Both") },
      { 0 }
    };

    AddEnum(_("Auto. wind"),
            _("This allows switching on or off the automatic wind algorithm.  When the algorithm is switched off, the pilot is responsible for setting the wind estimate.&#10;[Circling] Requires only a GPS source.&#10;[ZigZag] Requires an intelligent vario with airspeed output.&#10;[Both] Use ZigZag and circling."),
            auto_wind_list, settings.GetLegacyAutoWindMode(), this);
  }

  AddBoolean(_("Trail drift"),
             _("Determines whether the snail trail is drifted with the wind when displayed in circling mode."),
             XCSoarInterface::GetMapSettings().trail.wind_drift_enabled, this);
}


void
WindSetupPanel::OnModified(DataField &df)
{
  if (IsDataField(AutoWind, df))
    OnAutoWind((DataFieldEnum&)df);

  else if (IsDataField(TrailDrift, df))
    OnTrailDrift((DataFieldBoolean&)df);
}

void
WindSetupPanel::OnAutoWind(DataFieldEnum &Sender)
{
  WindSettings &settings = CommonInterface::SetComputerSettings().wind;
  settings.SetLegacyAutoWindMode(Sender.GetAsInteger());
}

void
WindSetupPanel::OnTrailDrift(DataFieldBoolean &Sender)
{
  XCSoarInterface::SetMapSettings().trail.wind_drift_enabled = Sender.GetAsBoolean();
}

Widget *
LoadWindSetupPanel(unsigned id)
{
  return new WindSetupPanel(id);
}
