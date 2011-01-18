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
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Screen/Layout.hpp"
#include "Task/Tasks/OrderedTask.hpp"

#include <assert.h>
#include <stdio.h>

static SingleWindow *parent_window;
static WndForm *wf=NULL;
static WndListFrame* wTaskTypes= NULL;
static OrderedTask* ordered_task= NULL;
static bool task_modified = false;

typedef std::vector<OrderedTask::Factory_t> FactoryList;

static FactoryList factory_types;

static OrderedTask::Factory_t
get_cursor_type() 
{
  return factory_types[wTaskTypes->GetCursorIndex()];
}

static void
RefreshView()
{
  WndFrame* wSummary = (WndFrame *)wf->FindByName(_T("frmSummary"));
  if (wSummary) {
    const TCHAR* text = OrderedTaskFactoryDescription(get_cursor_type());
    wSummary->SetCaption(text);
  }
}


static void
OnTaskPaintListItem(Canvas &canvas, const RECT rc, unsigned DrawListIndex)
{
  assert(DrawListIndex < factory_types.size());

  TCHAR sTmp[120];

  const TCHAR* text = OrderedTaskFactoryName(factory_types[DrawListIndex]);

  if (factory_types[DrawListIndex] == ordered_task->get_factory_type()) {
    _stprintf(sTmp, _T("*%s"), text);
  } else {
    _stprintf(sTmp, _T(" %s"), text);
  }
  canvas.text(rc.left + Layout::FastScale(2), rc.top + Layout::FastScale(2),
              sTmp);
}

static void OnCloseClicked(WndButton &Sender)
{
  (void)Sender;
  wf->SetModalResult(mrCancel);
}

static void
OnSelect()
{
  wf->SetModalResult(mrOK);
}

static void
OnTaskListEnter(unsigned ItemIndex)
{
  OnSelect();
}

static void OnSelectClicked(WndButton &Sender)
{
  OnSelect();
}

static void
OnTaskCursorCallback(unsigned i)
{
  RefreshView();
}

static CallBackTableEntry CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnSelectClicked),
  DeclareCallBackEntry(NULL)
};

/**
 *
 * @param task - not modified
 * @param task_type_returned type of task selected in UI
 * @return true if OK was clicked, false if Cancel was clicked
 */
bool
dlgTaskTypeShowModal(SingleWindow &parent, OrderedTask** task, OrderedTask::Factory_t& task_type_returned)
{
  bool bRetVal = false;
  ordered_task = *task;
  parent_window = &parent;
  task_modified = false;

  factory_types = ordered_task->get_factory_types(true);

  wf = NULL;

  if (Layout::landscape) {
    wf = LoadDialog(CallBackTable,
                        parent,
                        _T("IDR_XML_TASKTYPE_L"));
  } else {
    wf = LoadDialog(CallBackTable,
                        parent,
                        _T("IDR_XML_TASKTYPE"));
  }

  wTaskTypes = (WndListFrame*)wf->FindByName(_T("frmTaskTypes"));
  assert(wTaskTypes!=NULL);

  wTaskTypes->SetActivateCallback(OnTaskListEnter);
  wTaskTypes->SetPaintItemCallback(OnTaskPaintListItem);
  wTaskTypes->SetCursorCallback(OnTaskCursorCallback);

  wTaskTypes->SetLength(factory_types.size());

  for (unsigned i=0; i<factory_types.size(); i++) {
    if (factory_types[i] == ordered_task->get_factory_type()) {
      wTaskTypes->SetCursorIndex(i); 
    }
  }

  RefreshView();

  if (!wf) return false;
  assert(wf!=NULL);

  if (wf->ShowModal()== mrOK) {
    bRetVal = true;
  }
  task_type_returned = get_cursor_type();

  delete wf;
  wf = NULL;

  return bRetVal;
}
