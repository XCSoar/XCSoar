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

#ifndef DLGTASKMANAGER_HPP
#define DLGTASKMANAGER_HPP

#include "DataField/Base.hpp"
#include "Form/TabBar.hpp"

struct CallBackTableEntry;
class OrderedTask;
class SingleWindow;
class WndForm;
class WndButton;
class WndOwnerDrawFrame;

class dlgTaskManager
{
public:
  dlgTaskManager();

  /**
   * Validates task and prompts if change or error
   * Commits task if no error
   * @return True if task manager should close
   *          False if window should remain open
   */
  static bool CommitTaskChanges();

  /**
   * @returns True if validated, False if window shall remain open
   */
  static bool OnClose();
  static void dlgTaskManagerShowModal(SingleWindow &parent);
  static void RevertTask();
  static const CallBackTableEntry CallBackTable[];

  /**
   * restores task view rect
   */
  static void TaskViewRestore(WndOwnerDrawFrame *wTaskView);

public:
  /**
   * paints the task int the frame
   * @param Sender the frame in which to paint the task
   * @param canvas the canvas in which to paint the task
   */
  static void OnTaskPaint(WndOwnerDrawFrame *Sender, Canvas &canvas);

  /**
   * Continues the black bar of the TabBar across the width of portrait screen
   * @param Sender
   * @param canvas
   */
  static void OnBlackBarPaint(WndOwnerDrawFrame *Sender, Canvas &canvas);

  /**
   * toggles maximize or restore state of the TaskView frame
   * @param Sender
   * @param x
   * @param y
   * @return
   */
  static bool OnTaskViewClick(WndOwnerDrawFrame *Sender,
                              PixelScalar x, PixelScalar y);

  /**
   * different for landscape and for portrait mode
   * Used by TaskList after loading
   * @return Tab index that shows turnpoints
   */
  static unsigned GetTurnpointTab();

  /**
   * different for landscape and for portrait mode
   * Used by TaskList after new task
   * @return Tab index that shows task properties
   */
  static unsigned GetPropertiesTab();

  static void SetTitle();
};


class pnlTaskEdit
{
public:
  /**
   * creates the control from its XML file and does any init work
   * @param parent
   * @param task_modified Sets to True if changes it.
   * @param wf
   * @return Window* that points to the control created
   */
  static Window* Load(SingleWindow &parent, TabBarControl* wTabBar,
                      WndForm* wf, OrderedTask** task, bool* _task_modified);

  /**
   * clears task points
   * leaves task properies unchanged
   * prompts if task has points already
   * @param Sender
   */
  static void OnClearAllClicked(WndButton &Sender);
  static void OnEditTurnpointClicked(WndButton &Sender);

  static void OnMoveUpClicked(WndButton &Sender);

  /* only visible if on last tp
   * converts non-finish to finish point
   * using finish defaults
   */
  static void OnMakeFinish(WndButton &Sender);


  /**
   * moves the task point up (down) in the task point list
   * @param Sender
   */
  static void OnMoveDownClicked(WndButton &Sender);
  static bool OnKeyDown(WndForm &Sender, unsigned key_code);
  static void OnTaskCursorCallback(unsigned i);

  /**
   * Paints a single task list item in the listbox
   * @param canvas
   * @param rc
   * @param DrawListIndex
   */
  static void OnTaskPaintListItem(Canvas &canvas, const PixelRect rc,
                                  unsigned DrawListIndex);

  /**
   * shows dlgTaskPoint to edit point's properties
   * or adds new waypoint if (Add Waypoint) is clicked
   * @param ItemIndex index of item
   */
  static void OnTaskListEnter(unsigned ItemIndex);

  /**
   * updates TaskView, TaskPoints and TaskSummary
   * update
   */
  static void RefreshView();

  /**
   * callback to when tab is viewed
   * loads task, refreshes view
   * @param EventType 0 = Mouse Click, 1 = up/dn/left/right key
   * @return True
   */
  static bool OnTabPreShow(TabBarControl::EventType EventType);

  /**
   * unzooms the task view if it is maximized
   */
  static void OnTabReClick();
};

class pnlTaskProperties
{
public:
  /**
   * creates the control from its XML file and does any init work
   * @param parent
   * @param task_modified Sets to True if changes it.
   * @param wf
   * @return Window* that points to the control created
   */
  static Window* Load(SingleWindow &parent, TabBarControl* wTabBar,
                      WndForm* wf, OrderedTask** task, bool* _task_modified);

  /**
   * copies values from form to ordered_task
   * @return true
   */
  static bool OnTabPreHide();

  /**
   * Shows/Hides normal finish height depending on whether FAI
   * Finish Height is on or off
   * @param Sender
   * @param Mode
   */
  static void OnFAIFinishHeightData(DataField *Sender, DataField::DataAccessKind_t Mode);

  /**
   *
   * Changes the task type and mutates the task points as necessary
   * @param Sender datafield for prpTaskType
   * @param Mode
   */
  static void OnTaskTypeData(DataField *Sender, DataField::DataAccessKind_t Mode);

  /**
   * copies values from ordered_task to form
   * @param EventType 0 = Mouse Click, 1 = up/dn/left/right key
   * @return true
   */
  static bool OnTabPreShow(TabBarControl::EventType EventType);

  /**
   * unzooms the task view if it is maximized
   */
  static void OnTabReClick();
};

class pnlTaskList
{
public:
  /**
   * creates the control from its XML file and does any init work
   * @param parent
   * @param task_modified Sets to True if changes it.
   * @param wf
   * @return Window* that points to the control created
   */
  static Window* Load(SingleWindow &parent, TabBarControl* wTabBar,
                      WndForm* wf, OrderedTask** task, bool* _task_modified);

  static void OnTaskPaint(WndOwnerDrawFrame *Sender, Canvas &canvas);
  static void OnTaskPaintListItem(Canvas &canvas, const PixelRect rc,
                                  unsigned DrawListIndex);
  static void OnManageClicked(WndButton &Sender);
  static void OnBrowseClicked(WndButton &Sender);
  static void OnNewTaskClicked(WndButton &Sender);
  static void OnSaveClicked(WndButton &Sender);
  static void OnLoadClicked(WndButton &Sender);
  static bool OnDeclareClicked(WndButton &Sender);
  static bool OnTaskViewClick(WndOwnerDrawFrame *Sender,
                              PixelScalar x, PixelScalar y);
  static void OnDeleteClicked(WndButton &Sender);
  static void OnRenameClicked(WndButton &Sender);
  static void OnTaskListEnter(unsigned ItemIndex);
  static void OnTaskCursorCallback(unsigned i);
  /**
   * callback for Tab being clicked
   * refreshes / reloads list
   * @param EventType 0 = Mouse Click, 1 = up/dn/left/right key
   * @return True
   */
  static bool OnTabPreShow(TabBarControl::EventType EventType);

  /**
   * unzooms the task view if it is maximized
   */
  static void OnTabReClick();

  /**
   *  called when parent dialog closes
   */
  static void DestroyTab();
};

class pnlTaskManagerClose
{
public:
  /**
   * creates the control from its XML file and does any init work
   * @param parent
   * @param task_modified Sets to True if changes it.
   * @param wf
   * @return Window* that points to the control created
   */
  static Window* Load(SingleWindow &parent, TabBarControl* wTabBar,
                      WndForm* wf, OrderedTask** task, bool* _task_modified);

  static void OnCloseClicked(WndButton &Sender);
  static void OnRevertClicked(WndButton &Sender);

  /**
   * callback
   * sets status and buttons per task edit status
   * Closes immediately if flying and task not changed and mouse click
   * @param EventType 0 = Mouse Click, 1 = up/dn/left/right key
   * @return True
   */
  static bool OnTabPreShow(TabBarControl::EventType EventType);

  /**
   * Called when the Close tab is displayed and clicked a 2nd time
   * Acts as if the user clicked the Fly button
   */
  static void OnTabReClick();

};

class pnlTaskCalculator
{
public:
  /**
   * creates the control from its XML file and does any init work
   * @param parent
   * @param wf
   * @param _task_modified tells calc whether to display warning
   * @return Window* that points to the control created
   */
  static Window* Load(SingleWindow &parent, TabBarControl* wTabBar,
                      WndForm* wf, bool* _task_modified);

  /**
   * copies values from form to ordered_task
   * @return true
   */
  static bool OnTabPreHide();

  /**
   * copies values from ordered_task to form
   * @param EventType 0 = Mouse Click, 1 = up/dn/left/right key
   * @return true
   */
  static bool OnTabPreShow(TabBarControl::EventType EventType);

  /**
   * draws colored warning message if task has been modified
   */
  static void OnWarningPaint(WndOwnerDrawFrame *Sender, Canvas &canvas);
  static void OnTargetClicked(WndButton &Sender);

  static void OnTimerNotify(WndForm &Sender);

  static void OnMacCreadyData(DataField *Sender,
      DataField::DataAccessKind_t Mode);

  static void OnCruiseEfficiencyData(DataField *Sender,
      DataField::DataAccessKind_t Mode);
};

#endif /* DLGTASKMANAGER_HPP */
