/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_FLARM_NET_HPP
#define XCSOAR_FLARM_NET_HPP

#include "FLARM/Traffic.hpp"

#include <map>
#include <tchar.h>

class NLineReader;

/**
 * Handles the FlarmNet.org file
 */
namespace FlarmNet
{

  /**
   * FlarmNet.org file entry
   */
  struct Record
  {
    TCHAR id[7];           /**< FLARM id 6 bytes */
    TCHAR pilot[22];       /**< Name 15 bytes */
    TCHAR airfield[22];    /**< Airfield 4 bytes */
    TCHAR plane_type[22];  /**< Aircraft type 1 byte */
    TCHAR registration[8]; /**< Registration 7 bytes */
    TCHAR callsign[4];     /**< Callsign 3 bytes */
    TCHAR frequency[8];    /**< Radio frequency 6 bytes */

    FlarmId GetId() const;
  };

  void Destroy();

  unsigned LoadFile(NLineReader &reader);

  /**
   * Reads the FlarmNet.org file and fills the map
   *
   * @param path the path of the file
   * @return the number of records read from the file
   */
  unsigned LoadFile(const TCHAR *path);

  /**
   * Finds a FLARMNetRecord object based on the given FLARM id
   * @param id FLARM id
   * @return FLARMNetRecord object
   */
  const Record *FindRecordById(FlarmId id);

  /**
   * Finds a FLARMNetRecord object based on the given Callsign
   * @param cn Callsign
   * @return FLARMNetRecord object
   */
  const Record *FindFirstRecordByCallSign(const TCHAR *cn);
  unsigned FindRecordsByCallSign(const TCHAR *cn, const Record *array[],
                                 unsigned size);
};

#endif
