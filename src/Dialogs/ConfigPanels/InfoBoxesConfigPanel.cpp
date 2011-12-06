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
#include "Form/Form.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs/dlgTools.h"
#include "Dialogs/XML.hpp"
#include "ConfigPanel.hpp"


class InfoBoxesConfigPanel : public XMLWidget {

private:
  WndButton *buttons[InfoBoxSettings::MAX_PANELS];
public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
  void OnInfoBoxesButton(WndButton &button);
  unsigned ButtonIndex(const WndButton *button);
};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static InfoBoxesConfigPanel *instance;

void
InfoBoxesConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}

void
InfoBoxesConfigPanel::Hide()
{
  XMLWidget::Hide();
}

unsigned
InfoBoxesConfigPanel::ButtonIndex(const WndButton *button)
{
  for (unsigned i = 0; i < InfoBoxSettings::MAX_PANELS; i++)
    if (button == buttons[i])
      return i;
  // Not reached
  return 0;
}

void
InfoBoxesConfigPanel::OnInfoBoxesButton(WndButton &button)
{
  InfoBoxSettings &settings = CommonInterface::SetUISettings().info_boxes;

  unsigned i = ButtonIndex(&button);
  InfoBoxSettings::Panel &data = settings.panels[i];

  bool changed =
    dlgConfigInfoboxesShowModal(ConfigPanel::GetForm().GetMainWindow(),
                                ConfigPanel::GetForm().GetLook(),
                                InfoBoxLayout::InfoBoxGeometry, data,
                                i >= InfoBoxSettings::PREASSIGNED_PANELS);
  if (changed) {
    Profile::Save(data, i);
    Profile::Save();
    LogDebug(_T("InfoBox configuration: Changes saved"));
    buttons[i]->SetCaption(gettext(data.name));
  }
}

static void
OnInfoBoxesButton(WndButton &button)
{
  instance->OnInfoBoxesButton(button);
}

void
InfoBoxesConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  instance = this;
  LoadWindow(NULL, parent,
             Layout::landscape ? _T("IDR_XML_INFOBOXESCONFIGPANEL") :
                               _T("IDR_XML_INFOBOXESCONFIGPANEL_L"));

  const InfoBoxSettings &settings = CommonInterface::GetUISettings().info_boxes;

  for (unsigned i = 0; i < InfoBoxSettings::MAX_PANELS; i++) {
    const InfoBoxSettings::Panel &data = settings.panels[i];

    StaticString<32> buffer;
    buffer.Format(_T("cmdInfoBoxesPanel%u"), i);
    buttons[i] = (WndButton*) form.FindByName(buffer);
    if (buttons[i]) {
      buttons[i]->SetOnClickNotify(::OnInfoBoxesButton);
      buttons[i]->SetCaption(gettext(data.name));
    }
  }
}

bool
InfoBoxesConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  return true;
}

Widget *
CreateInfoBoxesConfigPanel()
{
  return new InfoBoxesConfigPanel();
}
