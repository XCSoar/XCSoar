/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Dialogs/Internal.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Components.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "LocalPath.hpp"
#include "OS/FileUtil.hpp"

#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"

#include "Form/TabBar.hpp"
#include "Form/Panel.hpp"
#include <assert.h>
#include <stdio.h>

static SingleWindow *parent_window;
static WndForm *wf = NULL;

static TabBarControl* wTabBar = NULL;

void
dlgInfoBoxAccessShowModal(SingleWindow &parent, const int id)
{
  dlgInfoBoxAccess::dlgInfoBoxAccessShowModal(parent, id);
}

void
dlgInfoBoxAccess::dlgInfoBoxAccessShowModal(SingleWindow &parent, const int id)
{
  parent_window = &parent;

  InfoBoxContent::InfoBoxDlgContent *dlgContent;
  dlgContent = InfoBoxManager::GetInfoBoxDlgContent(id);

  if (!dlgContent)
    return;

  const RECT targetRect = InfoBoxLayout::GetRemainingRect(parent.get_client_rect());

  wf = LoadDialog(dlgContent->CallBackTable, parent,
                   Layout::landscape ?
                  _T("IDR_XML_INFOBOXACCESS_L") : _T("IDR_XML_INFOBOXACCESS"), &targetRect);

  assert(wf != NULL);

  // Load tabs
  wTabBar = (TabBarControl*)wf->FindByName(_T("TabBar"));
  assert(wTabBar != NULL);

// we have four tabs:
// edit, info, setup, close

  if (dlgContent->pnlEdit.load) {
    Window* wEdit =
      dlgContent->pnlEdit.load(parent, wTabBar, wf, id);

    assert(wEdit);
    wTabBar->AddClient(wEdit, _T("Edit"), false, NULL, (*dlgContent->pnlEdit.preHide), (*dlgContent->pnlEdit.preShow),
                                                       (*dlgContent->pnlEdit.postShow), (*dlgContent->pnlEdit.reClick));
  }

  if (dlgContent->pnlInfo.load) {
    Window* wInfo =
      dlgContent->pnlInfo.load(parent, wTabBar, wf, id);

    assert(wInfo);
    wTabBar->AddClient(wInfo, _T("Info"), false, NULL, (*dlgContent->pnlInfo.preHide), (*dlgContent->pnlInfo.preShow),
                                                       (*dlgContent->pnlInfo.postShow), (*dlgContent->pnlInfo.reClick));
  }

  if (dlgContent->pnlSetup.load) {
    Window* wSetup =
      dlgContent->pnlSetup.load(parent, wTabBar, wf, id);

    assert(wSetup);
    wTabBar->AddClient(wSetup, _T("Setup"), false, NULL, (*dlgContent->pnlSetup.preHide), (*dlgContent->pnlSetup.preShow),
                                                         (*dlgContent->pnlSetup.postShow), (*dlgContent->pnlSetup.reClick));
  }

  Window* wClose =
    pnlInfoBoxAccessClose::Load(parent, wTabBar, wf, dlgContent->CallBackTable);
  assert(wClose);
  wTabBar->AddClient(wClose, _T("Close"), false, NULL, NULL,  pnlInfoBoxAccessClose::OnTabPreShow,
                                                 NULL, pnlInfoBoxAccessClose::OnTabReClick);


  wTabBar->SetCurrentPage(0);

  wf->ShowModal();

  delete wf;
}

bool
dlgInfoBoxAccess::OnClose()
{
  wf->SetModalResult(mrOK);
  return true;
}


// panel close

void
pnlInfoBoxAccessClose::OnCloseClicked(WndButton &Sender)
{
  (void)Sender;
  dlgInfoBoxAccess::OnClose();
}

void
pnlInfoBoxAccessClose::OnTabReClick()
{
  dlgInfoBoxAccess::OnClose();
}

bool
pnlInfoBoxAccessClose::OnTabPreShow(TabBarControl::EventType EventType)
{
  if (EventType == TabBarControl::MouseOrButton) {
    dlgInfoBoxAccess::OnClose();
    return true;
  }
  return true;
}

Window*
pnlInfoBoxAccessClose::Load(SingleWindow &parent, TabBarControl* wTabBar,
                            WndForm* _wf, CallBackTableEntry* CallBackTable)
{
  assert(wTabBar);

  assert(_wf);
  wf = _wf;

  Window *wInfoBoxAccessClose =
      LoadWindow(CallBackTable, wf, *wTabBar,
                 Layout::landscape ?
                     _T("IDR_XML_INFOBOXACCESSCLOSE_L") :
                     _T("IDR_XML_INFOBOXACCESSCLOSE"));
  assert(wInfoBoxAccessClose);

  return wInfoBoxAccessClose;
}
