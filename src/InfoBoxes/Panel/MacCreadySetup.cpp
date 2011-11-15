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

#include "MacCreadySetup.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Form/TabBar.hpp"
#include "Form/XMLWidget.hpp"
#include "Form/Button.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Interface.hpp"

class WndButton;

class MacCreadySetupPanel : public XMLWidget {
  unsigned id;

public:
  MacCreadySetupPanel(unsigned _id):id(_id) {}

  void QuickAccess(const TCHAR *value) {
    InfoBoxManager::ProcessQuickAccess(id, value);
  }

  void Setup();

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual void Show(const PixelRect &rc);
};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static MacCreadySetupPanel *instance;

void
MacCreadySetupPanel::Setup()
{
  InfoBoxManager::SetupFocused(id);
  dlgInfoBoxAccess::OnClose();
}

static void
PnlSetupOnSetup(gcc_unused WndButton &Sender) {
  instance->Setup();
}

static void
PnlSetupOnMode(gcc_unused WndButton &Sender)
{
  if (XCSoarInterface::SettingsComputer().task.auto_mc)
    Sender.SetCaption(_("AUTO"));
  else
    Sender.SetCaption(_("MANUAL"));

  instance->QuickAccess(_T("mode"));
}

static gcc_constexpr_data CallBackTableEntry call_back_table[] = {
  DeclareCallBackEntry(PnlSetupOnSetup),
  DeclareCallBackEntry(PnlSetupOnMode),
  DeclareCallBackEntry(NULL)
};

void
MacCreadySetupPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(call_back_table, parent, _T("IDR_XML_INFOBOXMACCREADYSETUP"));
}

void
MacCreadySetupPanel::Show(const PixelRect &rc)
{
  if (XCSoarInterface::SettingsComputer().task.auto_mc)
    ((WndButton *)form.FindByName(_T("cmdMode")))->SetCaption(_("MANUAL"));
  else
    ((WndButton *)form.FindByName(_T("cmdMode")))->SetCaption(_("AUTO"));

  XMLWidget::Show(rc);
}

Widget *
LoadMacCreadySetupPanel(unsigned id)
{
  return instance = new MacCreadySetupPanel(id);
}
