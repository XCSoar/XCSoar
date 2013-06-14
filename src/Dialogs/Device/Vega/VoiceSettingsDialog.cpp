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

#include "VoiceSettingsDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Interface.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Language/Language.hpp"

class VoiceSettingsPanel final : public RowFormWidget {
  enum Controls {
    CLIMB_RATE,
    TERRAIN,
    WAYPOINT_DISTANCE,
    TASK_ALTITUDE_DIFFERENCE,
    MAC_CREADY,
    NEXT_WAYPOINT,
    IN_SECTOR,
    AIRSPACE,
  };

public:
  VoiceSettingsPanel():RowFormWidget(UIGlobals::GetDialogLook()) {}

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;
};

void
VoiceSettingsPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const VoiceSettings &settings = CommonInterface::GetComputerSettings().voice;

  AddBoolean(_("Climb rate"),
             _("Enable voice read-back of the average climb rate when circling."),
             settings.voice_climb_rate_enabled);

  AddBoolean(_("Terrain"),
             _("Enable warnings for final glide through terrain."),
             settings.voice_terrain_enabled);

  AddBoolean(_("Waypoint distance"),
             _("Enable voice read-back of the distance to next waypoint when in cruise mode."),
             settings.voice_waypoint_distance_enabled);

  AddBoolean(_("Task altitude difference"),
             _("Enable voice read-back of height above/below final glide when in final glide."),
             settings.voice_task_altitude_difference_enabled);

  AddBoolean(_("MacCready"),
             _("Enable voice read-back of MacReady setting after it is adjusted."),
             settings.voice_mac_cready_enabled);

  AddBoolean(_("New waypoint"),
             _("Enable voice announcement that the task waypoint has changed."),
             settings.voice_new_waypoint_enabled);

  AddBoolean(_("In sector"),
             _("Enable voice announcement that the glider is within the task observation sector."),
             settings.voice_in_sector_enabled);

  AddBoolean(_("Airspace"),
             _("Enable voice warnings for airspace incursions."),
             settings.voice_airspace_enabled);
}

bool
VoiceSettingsPanel::Save(bool &_changed)
{
  VoiceSettings &settings = CommonInterface::SetComputerSettings().voice;
  bool changed = false;

  changed |= SaveValueEnum(CLIMB_RATE, ProfileKeys::VoiceClimbRate,
                           settings.voice_climb_rate_enabled);
  changed |= SaveValueEnum(TERRAIN, ProfileKeys::VoiceTerrain,
                           settings.voice_terrain_enabled);
  changed |= SaveValueEnum(WAYPOINT_DISTANCE,
                           ProfileKeys::VoiceWaypointDistance,
                           settings.voice_waypoint_distance_enabled);
  changed |= SaveValueEnum(TASK_ALTITUDE_DIFFERENCE,
                           ProfileKeys::VoiceTaskAltitudeDifference,
                           settings.voice_task_altitude_difference_enabled);
  changed |= SaveValueEnum(MAC_CREADY, ProfileKeys::VoiceMacCready,
                           settings.voice_mac_cready_enabled);
  changed |= SaveValueEnum(NEXT_WAYPOINT, ProfileKeys::VoiceNewWaypoint,
                           settings.voice_new_waypoint_enabled);
  changed |= SaveValueEnum(IN_SECTOR, ProfileKeys::VoiceInSector,
                           settings.voice_in_sector_enabled);
  changed |= SaveValueEnum(AIRSPACE, ProfileKeys::VoiceAirspace,
                           settings.voice_airspace_enabled);

  _changed |= changed;
  return true;
}

void dlgVoiceShowModal(){
  VoiceSettingsPanel *widget = new VoiceSettingsPanel();

  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateAuto(UIGlobals::GetMainWindow(), _("Vega Voice Extensions"),
                    widget);
  dialog.AddButton(_("OK"), mrOK);
  dialog.ShowModal();
}
