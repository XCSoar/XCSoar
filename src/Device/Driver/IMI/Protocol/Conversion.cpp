/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights
*/

/**
 * IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */

#include "Conversion.hpp"
#include "Math/Angle.hpp"

IMI::AngleConverter::AngleConverter(Angle angle)
{
  sign = (angle.sign() == -1) ? 1 : 0;
  double mag = angle.magnitude_degrees();
  degrees = static_cast<IMIDWORD> (mag);
  milliminutes = static_cast<IMIDWORD> ((mag - degrees) * 60 * 1000);
}
