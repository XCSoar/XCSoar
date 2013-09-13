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

#include "TaskDialogs.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Form/List.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Task/TypeStrings.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Language/Language.hpp"
#include "Util/StaticString.hpp"
#include "Util/TrivialArray.hpp"

#include <assert.h>

static OrderedTask* ordered_task = nullptr;
static OrderedTaskPoint* point = nullptr;
static unsigned active_index = 0;

static TrivialArray<TaskPointFactoryType, LegalPointSet::N> point_types;

static TaskPointFactoryType
get_point_type() 
{
  return ordered_task->GetFactory().GetType(*point);
}

static const TCHAR *
TPTypeItemHelp(unsigned i)
{
  return OrderedTaskPointDescription(point_types[i]);
}

static void
OnPointPaintListItem(Canvas &canvas, const PixelRect rc,
                     unsigned DrawListIndex)
{
  assert(DrawListIndex < point_types.size());

  StaticString<120> buffer;

  const TCHAR* text = OrderedTaskPointName(point_types[DrawListIndex]);

  if (point_types[DrawListIndex] == get_point_type())
    buffer.Format(_T("*%s"), text);
  else
    buffer.Format(_T(" %s"), text);

  canvas.DrawText(rc.left + Layout::GetTextPadding(),
                  rc.top + Layout::GetTextPadding(),
                  buffer);
}

/**
 * @return true if the task was modified
 */
static bool
SetPointType(TaskPointFactoryType type)
{
  if (type == get_point_type())
    // no change
    return false;

  AbstractTaskFactory &factory = ordered_task->GetFactory();
  bool task_modified = false;

  point = factory.CreateMutatedPoint(*point, type);
  if (point == nullptr)
    return false;

  if (factory.Replace(*point, active_index, true))
    task_modified = true;
  delete point;

  return task_modified;
}

bool
dlgTaskPointType(OrderedTask** task, const unsigned index)
{
  ordered_task = *task;
  active_index = index;

  point = &ordered_task->GetPoint(active_index);

  point_types.clear();
  ordered_task->GetFactory().GetValidTypes(index)
    .CopyTo(std::back_inserter(point_types));

  if (point_types.empty()) {
    assert(1);
    return false;
  }

  if (point_types.size() == 1)
    return SetPointType(point_types[0]);

  unsigned initial_index = 0;
  const auto b = point_types.begin(), e = point_types.end();
  auto i = std::find(b, e, get_point_type());
  if (i != e)
    initial_index = std::distance(b, i);

  FunctionListItemRenderer item_renderer(OnPointPaintListItem);

  int result = ListPicker(_("Task Point Type"),
                          point_types.size(), initial_index,
                          Layout::Scale(18),
                          item_renderer, false,
                          nullptr, TPTypeItemHelp);
  return result >= 0 && SetPointType(point_types[result]);
}
