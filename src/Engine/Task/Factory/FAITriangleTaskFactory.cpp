// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FAITriangleTaskFactory.hpp"
#include "Constraints.hpp"
#include "Task/Shapes/FAITriangleTask.hpp"

static constexpr TaskFactoryConstraints fai_triangle_constraints = {
  true,
  true,
  false,
  false,
  false,
  4, 4,
};

FAITriangleTaskFactory::FAITriangleTaskFactory(OrderedTask& _task,
                               const TaskBehaviour &tb):
  FAITaskFactory(fai_triangle_constraints, _task, tb)
{
}

TaskValidationErrorSet
FAITriangleTaskFactory::Validate() const noexcept
{
  auto errors = FAITaskFactory::Validate();

  if (!FAITriangleValidator::Validate(task))
    errors |= TaskValidationErrorType::WRONG_SHAPE;

  return errors;
}
