/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Task/TypeStrings.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Language/Language.hpp"
#include "Util/TrivialArray.hxx"

#include <assert.h>

static TrivialArray<TaskPointFactoryType, LegalPointSet::N> point_types;

static const TCHAR *
TPTypeItemHelp(unsigned i)
{
  return OrderedTaskPointDescription(point_types[i]);
}

class MutateTaskPointRenderer final : public ListItemRenderer {
  const TaskPointFactoryType current_type;

  TextRowRenderer row_renderer;

public:
  explicit MutateTaskPointRenderer(TaskPointFactoryType _current_type)
    :current_type(_current_type) {}

  unsigned CalculateLayout(const DialogLook &look) {
    return row_renderer.CalculateLayout(*look.list.font);
  }

  void OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned i) override;
};

void
MutateTaskPointRenderer::OnPaintItem(Canvas &canvas, PixelRect rc,
                                     unsigned DrawListIndex)
{
  assert(DrawListIndex < point_types.size());

  if (point_types[DrawListIndex] == current_type)
    rc.left = row_renderer.DrawColumn(canvas, rc, _T("*"));

  row_renderer.DrawTextRow(canvas, rc,
                           OrderedTaskPointName(point_types[DrawListIndex]));
}

/**
 * @return true if the task was modified
 */
static bool
SetPointType(OrderedTask &task, unsigned index,
             TaskPointFactoryType type)
{
  AbstractTaskFactory &factory = task.GetFactory();
  const auto &old_point = task.GetPoint(index);
  const auto current_type = factory.GetType(old_point);
  if (type == current_type)
    // no change
    return false;

  bool task_modified = false;

  auto point = factory.CreateMutatedPoint(old_point, type);
  if (point == nullptr)
    return false;

  if (factory.Replace(*point, index, true))
    task_modified = true;
  delete point;

  return task_modified;
}

bool
dlgTaskPointType(OrderedTask &task, const unsigned index)
{
  point_types.clear();
  task.GetFactory().GetValidTypes(index)
    .CopyTo(std::back_inserter(point_types));

  if (point_types.empty()) {
    assert(1);
    return false;
  }

  if (point_types.size() == 1)
    return SetPointType(task, index, point_types[0]);

  const auto &point = task.GetPoint(index);
  const auto current_type = task.GetFactory().GetType(point);

  unsigned initial_index = 0;
  const auto b = point_types.begin(), e = point_types.end();
  auto i = std::find(b, e, current_type);
  if (i != e)
    initial_index = std::distance(b, i);

  MutateTaskPointRenderer item_renderer(current_type);

  int result = ListPicker(_("Task Point Type"),
                          point_types.size(), initial_index,
                          item_renderer.CalculateLayout(UIGlobals::GetDialogLook()),
                          item_renderer, false,
                          nullptr, TPTypeItemHelp);
  return result >= 0 && SetPointType(task, index, point_types[result]);
}
