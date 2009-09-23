#include "SymmetricSectorZone.hpp"
#include <stdio.h>

void SymmetricSectorZone::set_legs(const TaskPoint *previous,
                                   const TaskPoint *current,
                                   const TaskPoint *next) 
{
  double biSector;
  if (!next && previous) { 
    // final
    biSector = current->bearing(*previous);
  } else if (next && previous) {
    // intermediate
    biSector = BiSector(current->bearing(*previous),
                        next->bearing(*current));
  } else if (next && !previous) {
    // start
    biSector = current->bearing(*next);
  } else {
    // single point
    biSector = 0;
  }

  setStartRadial(AngleLimit360(biSector-SectorAngle/2));
  setEndRadial(AngleLimit360(biSector+SectorAngle/2));

};

