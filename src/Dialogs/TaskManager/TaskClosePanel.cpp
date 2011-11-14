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

#include "TaskClosePanel.hpp"
#include "Internal.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Form/Button.hpp"
#include "Form/Draw.hpp"
#include "Form/Frame.hpp"
#include "Form/TabBar.hpp"
#include "Screen/Layout.hpp"
#include "Screen/SingleWindow.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "Language/Language.hpp"

#include <assert.h>
#include <stdio.h> //debug

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static TaskClosePanel *instance;

void
TaskClosePanel::RefreshStatus()
{
  wStatus->SetText(*task_modified ?
                   _("Task has been modified") : _("Task unchanged"));

  cmdRevert->set_visible(*task_modified);
}

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  dlgTaskManager::OnClose();
}

static void
OnRevertClicked(gcc_unused WndButton &Sender)
{
  dlgTaskManager::RevertTask();
  instance->RefreshStatus();
}

static gcc_constexpr_data CallBackTableEntry task_close_callbacks[] = {
  DeclareCallBackEntry(dlgTaskManager::OnTaskPaint),

  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnRevertClicked),

  DeclareCallBackEntry(NULL)
};

void
TaskClosePanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  instance = this;

  LoadWindow(task_close_callbacks, parent,
             Layout::landscape
             ? _T("IDR_XML_TASKMANAGERCLOSE_L") : _T("IDR_XML_TASKMANAGERCLOSE"));

  wTaskView = (WndOwnerDrawFrame *)form.FindByName(_T("frmTaskViewClose"));
  assert(wTaskView != NULL);
  wTaskView->SetOnMouseDownNotify(dlgTaskManager::OnTaskViewClick);

  wStatus = (WndFrame *)form.FindByName(_T("frmStatus"));
  assert(wStatus);

  cmdRevert = (WndButton *)form.FindByName(_T("cmdRevert"));
  assert(cmdRevert);

  cmdClose = (WndButton *)form.FindByName(_T("cmdClose"));
  assert(cmdClose);

  wStatus->SetAlignCenter();
}

bool
TaskClosePanel::Click()
{
  if (!(*task_modified)) {
    dlgTaskManager::OnClose();
    return false;
  }

  return true;
}

void
TaskClosePanel::ReClick()
{
  dlgTaskManager::OnClose();
}

void
TaskClosePanel::Show(const PixelRect &rc)
{
  dlgTaskManager::TaskViewRestore(wTaskView);
  RefreshStatus();

  XMLWidget::Show(rc);
}
