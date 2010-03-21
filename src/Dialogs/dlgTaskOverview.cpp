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
#include "Screen/Layout.hpp"
#include "Protection.hpp"
#include "Blackboard.hpp"
#include "SettingsTask.hpp"
#include "Logger.hpp"
#include "Math/FastMath.h"
#include "MainWindow.hpp"
#include "LocalPath.hpp"
#include "DataField/FileReader.hpp"
#include "Components.hpp"
#include "StringUtil.hpp"
#include "TaskClientUI.hpp"
#include "Task/TaskPoints/StartPoint.cpp"
#include "Task/TaskPoints/FinishPoint.cpp"
#include "Task/Visitors/TaskVisitor.hpp"

#include <assert.h>

static SingleWindow *parent_window;
static WndForm *wf=NULL;
static WndFrame *wfAdvanced=NULL;
static WndListFrame *wTaskList=NULL;
static bool showAdvanced= false;

static int UpLimit=0;
static int LowLimit=0;

static double lengthtotal = 0.0;
static bool fai_ok = false;

static const OrderedTask *ordered_task;

static void UpdateFilePointer(void) {
  WndProperty *wp = (WndProperty*)wf->FindByName(_T("prpFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
#ifdef OLD_TASK
    if (!string_is_empty(task.getTaskFilename())) {
      dfe->Lookup(task.getTaskFilename());
    } else {
      dfe->Set(0);
    }
#endif
    wp->RefreshDisplay();
  }
}


static void UpdateCaption (void) {
#ifdef OLD_TASK
  TCHAR title[MAX_PATH];
  TCHAR name[MAX_PATH];
  int len = _tcslen(task.getTaskFilename());
  if (len>0) {
    int index = 0;
    const TCHAR *src = task.getTaskFilename();
    while ((*src != _T('\0')) && (*src != _T('.'))) {
      if ((*src == _T('\\')) || (*src == _T('/'))) {
        index = 0;
      } else {
        name[index] = *src;
        index++;
      }
      src++;
    }
    name[index]= _T('\0');
  }

  if (!string_is_empty(name)) {
    _stprintf(title, _T("%s: %s"),
              gettext(_T("Task Overview")),
              name);
  } else {
    _stprintf(title, _T("%s"),
              gettext(_T("Task Overview")));
  }

  if (task.isTaskModified()) {
    _tcscat(title, _T(" *"));
  }

  wf->SetCaption(title);
#endif
}


static void
OnTaskPaintListItem(Canvas &canvas, const RECT rc, unsigned DrawListIndex)
{
  unsigned n = UpLimit - LowLimit;
  TCHAR sTmp[120];

  int w0 = rc.right - rc.left - Layout::FastScale(4);
  int w1 = canvas.text_width(_T(" 000km"));
  int w2 = canvas.text_width(_T("  000")_T(DEG));

  int p1 = w0-w1-w2; // Layout::FastScale(125)
  //int p2 = w0-w2; // Layout::FastScale(175)

  if (DrawListIndex < n){
    unsigned i = DrawListIndex;

    const OrderedTaskPoint *tp = ordered_task->getTaskPoint(i);
    if (tp == NULL)
      return;

#ifdef OLD_TASK
    if (Layout::landscape &&
        task.getSettings().AATEnabled && task.ValidTaskPoint(i+1) && (i>0)) {
      if (tp.AATType==0) {
        _stprintf(sTmp, _T("%s %.1f"),
                  way_points.get(tp.Index).Name,
                  Units::ToUserUnit(tp.AATCircleRadius, Units::DistanceUnit));
      } else {
        _stprintf(sTmp, _T("%s %.1f"),
                  way_points.get(tp.Index).Name,
                  Units::ToUserUnit(tp.AATSectorRadius, Units::DistanceUnit));
      }
    } else {
#endif
      const TCHAR *suffix;

      if (dynamic_cast<const StartPoint*>(tp) != NULL)
        suffix = _T(" (start)");
      else if (dynamic_cast<const FinishPoint*>(tp) != NULL)
        suffix = _T(" (finish)");
      else
        suffix = _T("");
      _stprintf(sTmp, _T("%s%s"),
                tp->get_waypoint().Name.c_str(), suffix);
#ifdef OLD_TASK
    }
#endif

    canvas.text_clipped(rc.left + Layout::FastScale(2),
                        rc.top + Layout::FastScale(2),
                        p1 - Layout::FastScale(4), sTmp);

    Units::FormatUserDistance(tp->scan_distance_nominal(), sTmp,
                              sizeof(sTmp) / sizeof(sTmp[0]));
    canvas.text(rc.left + p1 + w1 - canvas.text_width(sTmp),
                rc.top + Layout::FastScale(2), sTmp);

#ifdef OLD_TASK
    _stprintf(sTmp, _T("%d")_T(DEG),  iround(tp.InBound));
    canvas.text(rc.left + p2 + w2 - canvas.text_width(sTmp),
                rc.top + Layout::FastScale(2), sTmp);
#endif
  } else if (DrawListIndex==n) {
    _stprintf(sTmp, _T("  (%s)"), gettext(_T("add waypoint")));
    canvas.text(rc.left + Layout::FastScale(2), rc.top + Layout::FastScale(2),
                sTmp);
  } else if ((DrawListIndex==n+1) && ordered_task != NULL &&
             ordered_task->task_size() > 0) {

#ifdef OLD_TASK
    if (!task.getSettings().AATEnabled) {
#endif
      _stprintf(sTmp, gettext(_T("Total:")));
      canvas.text(rc.left + Layout::FastScale(2), rc.top + Layout::FastScale(2),
                  sTmp);

      if (fai_ok) {
        _stprintf(sTmp, _T("%.0f %s FAI"),
                  Units::ToUserUnit(lengthtotal, Units::DistanceUnit),
                  Units::GetDistanceName());
      } else {
        _stprintf(sTmp, _T("%.0f %s"),
                  Units::ToUserUnit(lengthtotal, Units::DistanceUnit),
                  Units::GetDistanceName());
      }

      canvas.text(rc.left + p1 + w1 - canvas.text_width(sTmp),
                  rc.top + Layout::FastScale(2), sTmp);
#ifdef OLD_TASK
    } else {

      double d1 = (XCSoarInterface::Calculated().TaskDistanceToGo
                   +XCSoarInterface::Calculated().TaskDistanceCovered);
      if (d1==0.0) {
        d1 = XCSoarInterface::Calculated().AATTargetDistance;
      }

      _stprintf(sTmp, _T("%s %.0f min %.0f (%.0f) %s"),
                gettext(_T("Total:")),
                task.getSettings().AATTaskLength*1.0,
                Units::ToUserUnit(lengthtotal, Units::DistanceUnit),
                Units::ToUserUnit(d1, Units::DistanceUnit),
                Units::GetDistanceName());
      canvas.text(rc.left + Layout::FastScale(2),
                  rc.top + Layout::FastScale(2), sTmp);
    }
#endif
  }
}


static void OverviewRefreshTask(void) {
#ifdef OLD_TASK
  task.RefreshTask(XCSoarInterface::SettingsComputer(),
                   XCSoarInterface::Basic());

  int i;
  // Only need to refresh info where the removal happened
  // as the order of other taskpoints hasn't changed
  UpLimit = 0;
  lengthtotal = 0;

  for (i=0; task.ValidTaskPoint(i); i++) {
    lengthtotal += task.getTaskPoint(i).LegDistance;
    UpLimit = i+1;
  }
  // Simple FAI 2004 triangle rules
  fai_ok = true;
  if (lengthtotal>0) {
    for (i=0; task.ValidTaskPoint(i); i++) {
      double lrat = task.getTaskPoint(i).LengthPercent;
      if ((lrat>0.45)||(lrat<0.10)) {
        fai_ok = false;
      }
    }
  } else {
    fai_ok = false;
  }

  RefreshTaskStatistics();

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpAATEst"));
  if (wp) {
    double dd = XCSoarInterface::Calculated().TaskTimeToGo;
    if ((XCSoarInterface::Calculated().TaskStartTime>0.0)&&(XCSoarInterface::Calculated().Flying)) {
      dd += XCSoarInterface::Basic().Time-XCSoarInterface::Calculated().TaskStartTime;
    }
    wp->GetDataField()->SetAsFloat(dd/60.0);
    wp->RefreshDisplay();
  }

  LowLimit =0;
#else
  LowLimit = 0;
  UpLimit = ordered_task != NULL
    ? ordered_task->task_size()
    : 0;
#endif

  wTaskList->SetLength(UpLimit - LowLimit + 1);
  wTaskList->invalidate();

  UpdateCaption();
}



static void UpdateAdvanced(void) {
  if (wfAdvanced) {
    wfAdvanced->set_visible(showAdvanced);
  }
}

static void
OnTaskListEnter(unsigned ItemIndex)
{
  if (UpLimit > 0 && ItemIndex < (unsigned)UpLimit) {
    const OrderedTaskPoint *tp = ordered_task->getTaskPoint(ItemIndex);
    if (tp == NULL)
      return;

    AbstractTaskFactory &factory = ordered_task->get_factory();
    dlgTaskWaypointShowModal(*parent_window, factory, ItemIndex, *tp, false);
  } else {
    bool is_finish = false;

    if ((int)ItemIndex >= UpLimit)
      ItemIndex= UpLimit;

    // add new waypoint
    if (logger.CheckDeclaration()) {

      if (ItemIndex>0) {
        if (MessageBoxX(gettext(_T("Will this be the finish?")),
                        gettext(_T("Add Waypoint")),
                        MB_YESNO|MB_ICONQUESTION) == IDYES) {
          is_finish = true;
        } else {
          is_finish = false;
        }
      }

      const Waypoint *wp = dlgWayPointSelect(*parent_window,
                                             XCSoarInterface::Basic().Location);
      if (wp == NULL)
        return;

      AbstractTaskFactory &factory = ordered_task->get_factory();
      OrderedTaskPoint *tp;
      if (ItemIndex==0) {
        tp = factory.createStart(AbstractTaskFactory::START_LINE, *wp);
      } else if (is_finish) {
        tp = factory.createFinish(AbstractTaskFactory::FINISH_LINE, *wp);
      } else {
        tp = factory.createIntermediate(AbstractTaskFactory::AST_CYLINDER, *wp);
      }

      if (tp == NULL)
        return;

      if (!factory.append(tp, true)) {
        //fprintf(stderr, "Failed to append turn point\n");
        return;
      }

      if (ItemIndex==0) {
        dlgTaskWaypointShowModal(*parent_window, factory, 0, *tp, true);
      } else if (is_finish) {
        dlgTaskWaypointShowModal(*parent_window, factory, 2, *tp, true);
      } else {
        dlgTaskWaypointShowModal(*parent_window, factory, 1, *tp, true);
      }
    }
  }

  OverviewRefreshTask();
}

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrCancel);
}

static void
OnClearClicked(WindowControl *Sender)
{
  (void)Sender;

  if (MessageBoxX(gettext(_T("Clear the task?")),
                  gettext(_T("Clear task")),
                  MB_YESNO|MB_ICONQUESTION) == IDYES) {
#ifdef OLD_TASK
    if (logger.CheckDeclaration()) {
      task.ClearTask();
      UpdateFilePointer();
      OverviewRefreshTask();
      UpdateCaption();
    }
#endif
  }
}

static void
OnCalcClicked(WindowControl *Sender)
{
  (void)Sender;

  wf->hide();
  dlgTaskCalculatorShowModal(*parent_window);
  OverviewRefreshTask();
  wf->show();
}

static void
OnAnalysisClicked(WindowControl *Sender)
{
  (void)Sender;

  wf->hide();
  dlgAnalysisShowModal();
  wf->show();
}

static void
OnDeclareClicked(WindowControl *Sender)
{
	(void)Sender;

#ifdef OLD_TASK
  task.RefreshTask(XCSoarInterface::SettingsComputer(),
                   XCSoarInterface::Basic());
#endif

  logger.LoggerDeviceDeclare();

  // do something here.
}

static void
OnSaveClicked(WindowControl * Sender)
{
#ifdef OLD_TASK
  (void)Sender;

  int file_index;
  TCHAR task_name[MAX_PATH];
  TCHAR file_name[MAX_PATH];
  WndProperty* wp;
  DataFieldFileReader *dfe;

  wp = (WndProperty*)wf->FindByName(_T("prpFile"));
  if (!wp) return;
  dfe = (DataFieldFileReader*)wp->GetDataField();

  file_index = dfe->GetAsInteger();

  if (file_index==0) {

    // TODO enhancement: suggest a good new name not already in the list
    _tcscpy(task_name,_T("0"));
    if (!dlgTextEntryShowModal(task_name, 10)){ // max length
      // TODO add messagebox
      return;
    }

    if (!string_is_empty(task_name)) {

      _tcscat(task_name, _T(".tsk"));

      LocalPath(file_name, task_name);

      dfe->Lookup(file_name);
      file_index = dfe->GetAsInteger();

      if (file_index==0) {
        // good, this file is unique..
        dfe->addFile(task_name, file_name);
        dfe->Lookup(file_name);
        wp->RefreshDisplay();
      }

    } else {
      // TODO code: report error, task not saved since no name was given
      return;
    }
  }

  if (file_index>0) {
    // file already exists! ask if want to overwrite

    _stprintf(file_name, _T("%s: '%s'"),
              gettext(_T("Task file already exists")),
              dfe->GetAsString());
    if(MessageBoxX(file_name,
                   gettext(_T("Overwrite?")),
                   MB_YESNO|MB_ICONQUESTION) != IDYES) {
      return;
    }
  }

  task.SaveTask(dfe->GetPathFile());
  UpdateCaption();
#endif
}


static void
OnLoadClicked(WindowControl *Sender)
{
  (void)Sender;

#ifdef OLD_TASK
  WndProperty* wp;
  DataFieldFileReader *dfe;

  wp = (WndProperty*)wf->FindByName(_T("prpFile"));
  if (!wp) return;
  dfe = (DataFieldFileReader*) wp->GetDataField();
  int file_index = dfe->GetAsInteger();

  if (file_index>0) {
    task.LoadNewTask(dfe->GetPathFile(), XCSoarInterface::SettingsComputer(),
                     XCSoarInterface::Basic());
    OverviewRefreshTask();
    UpdateFilePointer();
    UpdateCaption();
  }
#endif
}

static void
OnAdvancedClicked(WindowControl *Sender)
{
  (void)Sender;

  showAdvanced = !showAdvanced;
  UpdateAdvanced();
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnDeclareClicked),
  DeclareCallBackEntry(OnCalcClicked),
  DeclareCallBackEntry(OnClearClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnAdvancedClicked),
  DeclareCallBackEntry(OnSaveClicked),
  DeclareCallBackEntry(OnLoadClicked),
  DeclareCallBackEntry(OnAnalysisClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgTaskOverviewShowModal(SingleWindow &parent)
{
  parent_window = &parent;

  UpLimit = 0;
  LowLimit = 0;

  showAdvanced = false;

  wf = NULL;

  if (!Layout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgTaskOverview_L.xml"),
                        parent,
                        _T("IDR_XML_TASKOVERVIEW_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgTaskOverview.xml"),
                        parent,
                        _T("IDR_XML_TASKOVERVIEW"));
  }

  if (!wf) return;

  assert(wf!=NULL);

  ordered_task = task_ui.task_clone();

  UpdateCaption();

  wfAdvanced = ((WndFrame *)wf->FindByName(_T("frmAdvanced")));
  assert(wfAdvanced!=NULL);

  wTaskList = (WndListFrame*)wf->FindByName(_T("frmTaskList"));
  assert(wTaskList!=NULL);
  wTaskList->SetActivateCallback(OnTaskListEnter);
  wTaskList->SetPaintItemCallback(OnTaskPaintListItem);

  WndProperty* wp;

  //

  wp = (WndProperty*)wf->FindByName(_T("prpFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.tsk"));
    wp->RefreshDisplay();
  }
  UpdateFilePointer();

  // XCSoarInterface::Calculated().AATTimeToGo
  //

  // initialise and turn on the display
  OverviewRefreshTask();

  UpdateAdvanced();

  wf->ShowModal();

  task_ui.task_commit(*ordered_task);

  delete wf;

  wf = NULL;

}
