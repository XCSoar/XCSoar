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

#include "Dialogs/Internal.hpp"
#include "Dialogs/dlgTaskManager.hpp"
#include "Screen/Layout.hpp"
#include "Screen/SingleWindow.hpp"

#include <assert.h>
#include <stdio.h> //debug

WndForm* wf = NULL;
WndFrame* wStatus = NULL;
WndButton* cmdRevert = NULL;
WndButton* cmdClose = NULL;
static bool* task_modified = NULL;

static void
RefreshStatus()
{
  if (*task_modified)
    wStatus->SetText(_T("Task has been modified"));
  else
    wStatus->SetText(_T("Task unchanged"));

  cmdRevert->set_visible(*task_modified);
}

void
dlgTaskManagerClose::OnCloseClicked(WndButton &Sender)
{
  (void)Sender;
  dlgTaskManager::OnClose();
}

void
dlgTaskManagerClose::OnRevertClicked(WndButton &Sender)
{
  (void)Sender;
  dlgTaskManager::RevertTask();
  RefreshStatus();
}

bool
dlgTaskManagerClose::OnTabPreShow()
{
  RefreshStatus();
  return true;
}

Window*
dlgTaskManagerClose::Load(SingleWindow &parent, TabBarControl* wTabBar,
                          WndForm* _wf, OrderedTask** task, bool* _task_modified)
{
  assert(wTabBar);

  assert(_wf);
  wf = _wf;

  assert(_task_modified);
  task_modified = _task_modified;

  Window *wTaskManagerClose =
      LoadWindow(dlgTaskManager::CallBackTable, wf, *wTabBar,
                 _T("IDR_XML_TASKMANAGERCLOSE"));
  assert(wTaskManagerClose);

  wStatus = (WndFrame *)wf->FindByName(_T("frmStatus"));
  assert(wStatus);

  cmdRevert = (WndButton *)wf->FindByName(_T("cmdRevert"));
  assert(cmdRevert);

  cmdClose = (WndButton *)wf->FindByName(_T("cmdClose"));
  assert(cmdClose);

  // center for portrait or for landscape
  const RECT r = wTabBar->get_client_rect();
  const unsigned CloseTop = (r.bottom - r.top -
      (Layout::landscape ? 0 : wTabBar->GetBarHeight())) / 2 - cmdClose->get_height();
  const unsigned CloseLeft = (r.right - r.left - cmdClose->get_width() +
      (Layout::landscape ? wTabBar->GetBarWidth() : 0)) / 2;
  cmdClose->move(CloseLeft, CloseTop);
  wStatus->move(CloseLeft, CloseTop - wStatus->get_height());
  cmdRevert->move(CloseLeft, CloseTop + 2* cmdClose->get_height());

  wStatus->SetAlignCenter();
  return wTaskManagerClose;
}
