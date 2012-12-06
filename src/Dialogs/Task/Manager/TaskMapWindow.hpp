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

#ifndef XCSOAR_TASK_MAP_WINDOW_HPP
#define XCSOAR_TASK_MAP_WINDOW_HPP

#include "Screen/PaintWindow.hpp"

struct MapLook;
class ActionListener;
class OrderedTask;

/**
 * A window that shows the task.
 */
class TaskMapWindow : public PaintWindow {
  const MapLook &look;

  ActionListener &listener;
  const int id;

  const OrderedTask *task;

public:
  TaskMapWindow(const MapLook &_look,
                ActionListener &_listener, const int _id)
    :look(_look), listener(_listener), id(_id), task(nullptr) {}

  void SetTask(const OrderedTask *_task) {
    task = _task;
    Invalidate();
  }

  /* virtual methods from class Window */
  virtual void OnPaint(Canvas &canvas) gcc_override;
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y) gcc_override;
};

#endif /* DLGTASKMANAGER_HPP */
