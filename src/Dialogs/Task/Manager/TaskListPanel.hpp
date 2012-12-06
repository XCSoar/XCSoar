/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_TASK_LIST_PANEL_HPP
#define XCSOAR_TASK_LIST_PANEL_HPP

#include "Widget/XMLWidget.hpp"
#include "Form/List.hpp"

class TaskManagerDialog;
class WndButton;
class WndOwnerDrawFrame;
class TabbedControl;
class Canvas;
class OrderedTask;
class TaskStore;

class TaskListPanel : public XMLWidget, private ListControl::Handler {
  TaskManagerDialog &dialog;

  OrderedTask **active_task;
  bool *task_modified;

  TaskStore *task_store;

  bool lazy_loaded; // if store has been loaded first time tab displayed

  /**
   * Showing all task files?  (including *.igc, *.cup)
   */
  bool more;

  ListControl *wTasks;
  WndButton *more_button;

public:
  TaskListPanel(TaskManagerDialog &_dialog,
                OrderedTask **_active_task, bool *_task_modified)
    :dialog(_dialog),
     active_task(_active_task), task_modified(_task_modified),
     more(false) {}

  void RefreshView();
  void DirtyList();

  void SaveTask();
  void LoadTask();
  void DeleteTask();
  void RenameTask();

  void OnMoreClicked();

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual void Unprepare();
  virtual void Show(const PixelRect &rc);
  virtual void Hide();

protected:
  OrderedTask *get_cursor_task();

  gcc_pure
  const TCHAR *get_cursor_name();

private:
  /* virtual methods from class ListControl::Handler */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) gcc_override;

  virtual void OnCursorMoved(unsigned index) gcc_override {
    RefreshView();
  }

  virtual bool CanActivateItem(unsigned index) const gcc_override {
      return true;
  }

  virtual void OnActivateItem(unsigned index) gcc_override {
    LoadTask();
  }
};

#endif
