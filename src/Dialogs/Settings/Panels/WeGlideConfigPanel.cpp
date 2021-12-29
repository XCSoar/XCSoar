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

#include "WeGlideConfigPanel.hpp"
#include "Cloud/weglide/WeGlideSettings.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Listener.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"

#include <stdio.h>

// #define HAVE_WEGLIDE_PILOTNAME

enum ControlIndex {
  WeGlideEnabled,
  WeGlideAutomaticUpload,
  WeGlidePilotID,
  WeGlidePilotBirthDate,
};


class WeGlideConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  WeGlideConfigPanel() noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void SetEnabled(bool enabled) noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
WeGlideConfigPanel::SetEnabled(bool enabled) noexcept
{
  SetRowEnabled(WeGlideAutomaticUpload, enabled);
  SetRowEnabled(WeGlidePilotBirthDate, enabled);
  SetRowEnabled(WeGlidePilotID, enabled);
}

void
WeGlideConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(WeGlideEnabled, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    SetEnabled(dfb.GetValue());
  }
}

void
WeGlideConfigPanel::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  const WeGlideSettings &weglide = CommonInterface::GetComputerSettings().weglide;

  RowFormWidget::Prepare(parent, rc);

  AddBoolean(_("Enable"), nullptr,
             weglide.enabled, this);

  AddBoolean(_("Automatic Upload"),
             _("Uploads flights automatically after download from logger?"),
             weglide.automatic_upload, this);

  AddInteger(_("Pilot"),
             _("Take this from your WeGlide Profile. Or set to 0 if not used."),
             _T("%d"), _T("%d"), 1, 99999, 1, weglide.pilot_id);

  AddDate(_("Pilot date of birth"), nullptr,
          weglide.pilot_birthdate);

  SetEnabled(weglide.enabled);
}

bool
WeGlideConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  auto &weglide = CommonInterface::SetComputerSettings().weglide;

  changed |= SaveValue(WeGlideAutomaticUpload,
                       ProfileKeys::WeGlideAutomaticUpload,
                       weglide.automatic_upload);

  changed |= SaveValue(WeGlidePilotID, ProfileKeys::WeGlidePilotID,
                       weglide.pilot_id);

  changed |= SaveValue(WeGlidePilotBirthDate,
                       ProfileKeys::WeGlidePilotBirthDate,
                       weglide.pilot_birthdate);

  changed |= SaveValue(WeGlideEnabled, ProfileKeys::WeGlideEnabled,
                       weglide.enabled);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateWeGlideConfigPanel() noexcept
{
  return std::make_unique<WeGlideConfigPanel>();
}
