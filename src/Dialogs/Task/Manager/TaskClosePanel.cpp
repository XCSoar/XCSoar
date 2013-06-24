/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Dialogs/CallBackTable.hpp"
#include "Form/Button.hpp"
#include "Form/Frame.hpp"
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

  cmdRevert->SetVisible(*task_modified);
}

void
TaskClosePanel::CommitAndClose()
{
  if (dialog.Commit())
    dialog.SetModalResult(mrOK);
}

static void
OnCloseClicked()
{
  instance->CommitAndClose();
}

static void
OnRevertClicked()
{
  instance->dialog.Revert();
  instance->RefreshStatus();
}

static constexpr CallBackTableEntry task_close_callbacks[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnRevertClicked),

  DeclareCallBackEntry(NULL)
};

void
TaskClosePanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  instance = this;

  LoadWindow(task_close_callbacks, parent, rc, _T("IDR_XML_TASKMANAGERCLOSE"));

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
    CommitAndClose();
    return false;
  }

  return true;
}

void
TaskClosePanel::ReClick()
{
  CommitAndClose();
}

void
TaskClosePanel::Show(const PixelRect &rc)
{
  dialog.ShowTaskView();

  RefreshStatus();

  XMLWidget::Show(rc);
}

void
TaskClosePanel::Hide()
{
  dialog.ResetTaskView();

  XMLWidget::Hide();
}

bool
TaskClosePanel::SetFocus()
{
  cmdClose->SetFocus();
  return true;
}
