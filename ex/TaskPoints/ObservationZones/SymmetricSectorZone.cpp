#include "SymmetricSectorZone.hpp"
#include <stdio.h>

void SymmetricSectorZone::set_legs(const TaskPoint *previous,
                                   const TaskPoint *current,
                                   const TaskPoint *next) 
{
  double biSector;
  if (!next && previous) { 
    // final
    biSector = previous->bearing(*current);
  } else if (next && previous) {
    // intermediate
    biSector = BiSector(previous->bearing(*current),
                        current->bearing(*next));
  } else if (next && !previous) {
    // start
    biSector = next->bearing(*current);
  } else {
    // single point
    biSector = 0;
  }

  setStartRadial(AngleLimit360(biSector-SectorAngle/2));
  setEndRadial(AngleLimit360(biSector+SectorAngle/2));

};

