/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights
*/

/**
 * IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */

#ifndef XCSOAR_IMI_CONVERSION_HPP
#define XCSOAR_IMI_CONVERSION_HPP

#include "Types.hpp"

class Angle;

namespace IMI
{
  struct AngleConverter
  {
    union
    {
      struct
      {
        IMIDWORD milliminutes :16;
        IMIDWORD degrees :8;
        IMIDWORD sign :1;
      };
      IMIDWORD value;
    };

    AngleConverter(Angle angle);
  };
}

#endif
