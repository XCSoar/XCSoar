#include "AATIsoline.hpp"

AATIsoline::AATIsoline(const AATPoint& ap):
  ell(ap.get_previous()->get_location_remaining(),
      ap.get_next()->get_location_remaining(),
      ap.get_location_target(),
      ap.get_task_projection())
{
}

