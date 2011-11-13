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

#ifndef XCSOAR_TASK_EDIT_PANEL_HPP
#define XCSOAR_TASK_EDIT_PANEL_HPP

#include "Screen/Point.hpp"

class Window;
class SingleWindow;
class WndForm;
class TabBarControl;
class WndButton;
class Canvas;
class OrderedTask;

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
  static bool OnTabPreShow();

  /**
   * unzooms the task view if it is maximized
   */
  static void OnTabReClick();
};

#endif
