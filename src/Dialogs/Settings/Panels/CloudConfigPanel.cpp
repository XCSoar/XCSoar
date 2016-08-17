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

#include "CloudConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Language/Language.hpp"
#include "Tracking/SkyLines/Key.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"

#include <stdio.h>

enum ControlIndex {
  ENABLED,
};

class CloudConfigPanel final
  : public RowFormWidget {
public:
  CloudConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  bool Save(bool &changed) override;
};

void
CloudConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const TrackingSettings &settings =
    CommonInterface::GetComputerSettings().tracking;

  RowFormWidget::Prepare(parent, rc);

  AddBoolean(_T("XCSoar Cloud"),
             _("Participate in the XCSoar Cloud field test?  This transmits your location, thermal/wave locations and other weather data to our test server."),
             settings.skylines.cloud_enabled == TriState::TRUE);
}

bool
CloudConfigPanel::Save(bool &_changed)
{
  bool changed = false;

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;

  bool cloud_enabled = settings.skylines.cloud_enabled == TriState::TRUE;
  if (SaveValue(ENABLED, ProfileKeys::CloudEnabled, cloud_enabled)) {
    settings.skylines.cloud_enabled = cloud_enabled
      ? TriState::TRUE
      : TriState::FALSE;

    if (settings.skylines.cloud_enabled == TriState::TRUE &&
        settings.skylines.cloud_key == 0) {
      settings.skylines.cloud_key = SkyLinesTracking::GenerateKey();

      char s[64];
      snprintf(s, sizeof(s), "%llx",
               (unsigned long long)settings.skylines.cloud_key);
      Profile::Set(ProfileKeys::CloudKey, s);
    }

    changed = true;
  }

  _changed |= changed;

  return true;
}

Widget *
CreateCloudConfigPanel()
{
  return new CloudConfigPanel();
}
