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

#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Button.hpp"
#include "Form/Util.hpp"
#include "DataField/Enum.hpp"
#include "Dialogs/Dialogs.h"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "InfoBoxesConfigPanel.hpp"
#include "LogFile.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Dialogs/dlgConfigInfoboxes.hpp"

static WndForm* wf = NULL;
static WndButton *buttons[InfoBoxSettings::MAX_PANELS];


static unsigned
buttonIndex(const WndButton *button)
{
  for (unsigned i = 0; i < InfoBoxSettings::MAX_PANELS; i++)
    if (button == buttons[i])
      return i;
  // Not reached
  return 0;
}


static void
OnInfoBoxesButton(WndButton &button)
{
  InfoBoxSettings &settings = CommonInterface::SetUISettings().info_boxes;

  unsigned i = buttonIndex(&button);
  InfoBoxSettings::Panel &data = settings.panels[i];

  bool changed =
    dlgConfigInfoboxesShowModal(wf->GetMainWindow(), wf->GetLook(),
                                InfoBoxLayout::InfoBoxGeometry, data,
                                i >= InfoBoxSettings::PREASSIGNED_PANELS);
  if (changed) {
    data.modified = true;
    Profile::Save(settings);
    Profile::Save();
    LogDebug(_T("InfoBox configuration: Changes saved"));
    buttons[i]->SetCaption(gettext(data.name));
  }
}


void
InfoBoxesConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  const InfoBoxSettings &settings = CommonInterface::GetUISettings().info_boxes;

  for (unsigned i = 0; i < InfoBoxSettings::MAX_PANELS; i++) {
    const InfoBoxSettings::Panel &data = settings.panels[i];

    TCHAR buffer[32];
    _stprintf(buffer, _T("cmdInfoBoxesPanel%u"), i);
    buttons[i] = (WndButton*) wf->FindByName(buffer);
    if (buttons[i]) {
      buttons[i]->SetOnClickNotify(OnInfoBoxesButton);
      buttons[i]->SetCaption(gettext(data.name));
    }
  }
}


bool
InfoBoxesConfigPanel::Save(bool &requirerestart)
{
  return false;
}
