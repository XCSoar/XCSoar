/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

static void OnCloseClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrCancel);
}

static void
OnSelect()
{
  if (ordered_task->get_factory_type() == get_cursor_type()) {
    // no change
    wf->SetModalResult(mrOK);
    return;
  }
  bool apply = false;
  if (!ordered_task->task_size()) {
    apply = true;
    // empty task, don't ask confirmation
  } else if (MessageBoxX(_("Change task type?"),
                  _("Task Type"),
                  MB_YESNO|MB_ICONQUESTION) == IDYES) {
    apply = true;
  }
  if (apply) {
    task_modified |= true;
    ordered_task->set_factory(get_cursor_type());
    wf->SetModalResult(mrOK);
  }
}

static void
OnTaskListEnter(unsigned ItemIndex)
{
  OnSelect();
}

static void OnSelectClicked(WindowControl * Sender)
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

bool
dlgTaskTypeShowModal(SingleWindow &parent, OrderedTask** task)
{
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
    if (*task != ordered_task) {
      *task = ordered_task;
    }
  }

  delete wf;
  wf = NULL;

  return task_modified;
}
