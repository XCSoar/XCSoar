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

#ifndef XCSOAR_TASK_ACTIONS_PANEL_HPP
#define XCSOAR_TASK_ACTIONS_PANEL_HPP

#include "Form/XMLWidget.hpp"

class TaskMiscPanel;
class TabBarControl;
class WndOwnerDrawFrame;
class TabbedControl;
class OrderedTask;
class TaskListPanel;

class TaskActionsPanel : public XMLWidget {
  TaskMiscPanel &parent;

  TabBarControl &tab_bar;

  OrderedTask **active_task;
  bool *task_modified;

  WndOwnerDrawFrame* wTaskView;

  TaskListPanel *list_panel;

public:
  TaskActionsPanel(TaskMiscPanel &_parent,
                   TabBarControl &_tab_bar,
                   OrderedTask **_active_task, bool *_task_modified)
    :parent(_parent), tab_bar(_tab_bar),
     active_task(_active_task), task_modified(_task_modified),
     wTaskView(NULL), list_panel(NULL) {}

  void SetTaskView(WndOwnerDrawFrame *_task_view) {
    assert(wTaskView == NULL);
    assert(_task_view != NULL);

    wTaskView = _task_view;
  }

  void SetListPanel(TaskListPanel *_list_panel) {
    assert(list_panel == NULL);
    assert(_list_panel != NULL);

    list_panel = _list_panel;
  }

  void SaveTask();

  void OnBrowseClicked();
  void OnNewTaskClicked();
  void OnDeclareClicked();

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) gcc_override;
  virtual void ReClick() gcc_override;
  virtual void Show(const PixelRect &rc) gcc_override;
  virtual void Hide() gcc_override;
};

#endif
