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
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Listener.hpp"

#include <stdio.h>

enum ControlIndex {
  ENABLED,
  SHOW_THERMALS,
};

class CloudConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  CloudConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void SetEnabled(bool enabled);

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  bool Save(bool &changed) override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) override;
};

void
CloudConfigPanel::SetEnabled(bool enabled)
{
  SetRowEnabled(SHOW_THERMALS, enabled);
}

void
CloudConfigPanel::OnModified(DataField &df)
{
  if (IsDataField(ENABLED, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    SetEnabled(dfb.GetAsBoolean());
  }
}

void
CloudConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  const auto &settings =
    CommonInterface::GetComputerSettings().tracking.skylines.cloud;

  AddBoolean(_T("XCSoar Cloud"),
             _("Participate in the XCSoar Cloud field test?  This transmits your location, thermal/wave locations and other weather data to our test server."),
             settings.enabled == TriState::TRUE,
             this);

  AddBoolean(_T("Show thermals"),
             _("Obtain and show thermal locations reported by others."),
             settings.show_thermals);

  SetEnabled(settings.enabled == TriState::TRUE);
}

bool
CloudConfigPanel::Save(bool &_changed)
{
  bool changed = false;

  auto &settings =
    CommonInterface::SetComputerSettings().tracking.skylines.cloud;

  bool cloud_enabled = settings.enabled == TriState::TRUE;
  if (SaveValue(ENABLED, ProfileKeys::CloudEnabled, cloud_enabled)) {
    settings.enabled = cloud_enabled
      ? TriState::TRUE
      : TriState::FALSE;

    if (settings.enabled == TriState::TRUE && settings.key == 0) {
      settings.key = SkyLinesTracking::GenerateKey();

      char s[64];
      snprintf(s, sizeof(s), "%llx",
               (unsigned long long)settings.key);
      Profile::Set(ProfileKeys::CloudKey, s);
    }

    changed = true;
  }

  changed |= SaveValue(SHOW_THERMALS, ProfileKeys::CloudShowThermals,
                       settings.show_thermals);

  _changed |= changed;

  return true;
}

Widget *
CreateCloudConfigPanel()
{
  return new CloudConfigPanel();
}
