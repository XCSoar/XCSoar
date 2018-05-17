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

#include "InfoBoxesConfigPanel.hpp"
#include "../dlgConfigInfoboxes.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Current.hpp"
#include "Profile/InfoBoxConfig.hpp"
#include "Form/Button.hpp"
#include "Form/ActionListener.hpp"
#include "Interface.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"

class InfoBoxesConfigPanel final
  : public RowFormWidget, public ActionListener {
public:
  InfoBoxesConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;
  virtual void OnAction(int id) override;
};

void
InfoBoxesConfigPanel::OnAction(int id)
{
  InfoBoxSettings &settings = CommonInterface::SetUISettings().info_boxes;

  unsigned i = (unsigned)id;
  InfoBoxSettings::Panel &data = settings.panels[i];

  bool changed =
    dlgConfigInfoboxesShowModal(UIGlobals::GetMainWindow(),
                                UIGlobals::GetDialogLook(),
                                UIGlobals::GetLook().info_box,
                                InfoBoxManager::layout.geometry, data,
                                i >= InfoBoxSettings::PREASSIGNED_PANELS);
  if (changed) {
    Profile::Save(Profile::map, data, i);
    Profile::Save();
    ((Button &)GetRow(i)).SetCaption(gettext(data.name));
  }
}

void
InfoBoxesConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const InfoBoxSettings &settings = CommonInterface::GetUISettings().info_boxes;

  RowFormWidget::Prepare(parent, rc);

  for (unsigned i = 0; i < InfoBoxSettings::MAX_PANELS; i++) {
    const InfoBoxSettings::Panel &data = settings.panels[i];

    AddButton(gettext(data.name), *this, i);
    if (i>2)
      SetExpertRow(i);
  }

  AddBoolean(_("Use final glide mode"),
             _("Controls whether the \"final glide\" InfoBox mode should be used on \"auto\" pages."),
             settings.use_final_glide);
}

bool
InfoBoxesConfigPanel::Save(bool &_changed)
{
  InfoBoxSettings &settings = CommonInterface::SetUISettings().info_boxes;
  SaveValue(InfoBoxSettings::MAX_PANELS, ProfileKeys::UseFinalGlideDisplayMode,
            settings.use_final_glide);

  return true;
}

Widget *
CreateInfoBoxesConfigPanel()
{
  return new InfoBoxesConfigPanel();
}
