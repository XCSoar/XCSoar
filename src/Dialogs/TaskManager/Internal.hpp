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

#ifndef XCSOAR_TASK_MANAGER_INTERNAL_HPP
#define XCSOAR_TASK_MANAGER_INTERNAL_HPP

#include "Screen/Point.hpp"

struct CallBackTableEntry;
class OrderedTask;
class SingleWindow;
class WndOwnerDrawFrame;
class Canvas;

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

#endif /* DLGTASKMANAGER_HPP */
