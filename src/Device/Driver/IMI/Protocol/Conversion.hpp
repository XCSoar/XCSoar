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

#include <tchar.h>

class Angle;
struct BrokenDateTime;
struct Declaration;
class Waypoint;

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

  void ConvertToChar(const TCHAR* unicode, char* ascii, int outSize);
  BrokenDateTime ConvertToDateTime(IMIDATETIMESEC in);

  /**
   * @brief Sets data in IMI Waypoint structure
   *
   * @param decl LK task declaration
   * @param imiIdx The index of IMI waypoint to set
   * @param imiWp IMI waypoint structure to set
   */
  void ConvertOZ(const Declaration &decl, unsigned imiIdx, TWaypoint &imiWp);
  void ConvertWaypoint(const Waypoint &wp, TWaypoint &imiWp);

}

#endif
