#include "AATIsoline.hpp"

AATIsoline::AATIsoline(const AATPoint& ap):
  ell(ap.get_previous()->get_reference_remaining(),
      ap.get_next()->get_reference_remaining(),
      ap.getTargetLocation(),
      ap.get_task_projection())
{
}

