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

#ifndef XCSOAR_TASK_CLOSE_PANEL_HPP
#define XCSOAR_TASK_CLOSE_PANEL_HPP

#include "Widget/XMLWidget.hpp"

class TaskManagerDialog;
class OrderedTask;
class WndFrame;
class WndButton;

class TaskClosePanel : public XMLWidget {
public:
  TaskManagerDialog &dialog;

private:
  bool *task_modified;

  WndFrame *wStatus;
  WndButton *cmdRevert, *cmdClose;

public:
  TaskClosePanel(TaskManagerDialog &_dialog, bool *_task_modified)
    :dialog(_dialog), task_modified(_task_modified) {}

  void RefreshStatus();

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Click();
  virtual void ReClick();
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
};

#endif
