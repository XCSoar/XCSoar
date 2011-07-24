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
#if !defined(XCSOAR_RENDER_TASK_HPP)
#define XCSOAR_RENDER_TASK_HPP

#include "Task/Visitors/TaskVisitor.hpp"
#include "Geo/GeoBounds.hpp"

class RenderTaskPoint;
class AbstractTask;
class AbortTask;
class OrderedTask;
class GotoTask;

class RenderTask: 
  public TaskVisitor
{
  RenderTaskPoint &tpv;
  GeoBounds screen_bounds;

public:
  RenderTask(RenderTaskPoint& _tpv, GeoBounds _screen_bounds);
  void Visit(const AbortTask& task);
  void Visit(const OrderedTask& task);
  void Visit(const GotoTask& task);
protected:
  void draw_layers(const AbstractTask& task);
};

#endif
