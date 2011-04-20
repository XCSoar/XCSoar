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

#ifndef XCSOAR_FLARM_STATE_HPP
#define XCSOAR_FLARM_STATE_HPP

#include "FLARM/Traffic.hpp"
#include "NMEA/Validity.hpp"
#include "Util/StaticArray.hpp"

/**
 * Received FLARM data, cached
 */
struct FLARM_STATE
{
  enum {
    FLARM_MAX_TRAFFIC = 15,
  };

  /** Number of received FLARM devices */
  unsigned short rx;
  /** Transmit status */
  unsigned short tx;
  /** GPS status */
  unsigned short gps;
  /** Alarm level of FLARM (0-3) */
  unsigned short alarm_level;
  /** Is FLARM information available? */
  Validity available;
  /** Flarm traffic information */
  StaticArray<FLARM_TRAFFIC, FLARM_MAX_TRAFFIC> traffic;

  /**
   * Is there new FLARM traffic present?
   * @see traffic
   */
  bool NewTraffic;

public:
  void clear();

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   */
  void complement(const FLARM_STATE &add) {
    if (!available && add.available)
      *this = add;
  }

  unsigned GetActiveTrafficCount() const {
    return traffic.size();
  }

  /**
   * Looks up an item in the traffic list.
   *
   * @param id FLARM id
   * @return the FLARM_TRAFFIC pointer, NULL if not found
   */
  FLARM_TRAFFIC *FindTraffic(FlarmId id) {
    for (unsigned i = 0; i < traffic.size(); i++)
      if (traffic[i].ID == id)
        return &traffic[i];

    return NULL;
  }

  /**
   * Looks up an item in the traffic list.
   *
   * @param id FLARM id
   * @return the FLARM_TRAFFIC pointer, NULL if not found
   */
  const FLARM_TRAFFIC *FindTraffic(FlarmId id) const {
    for (unsigned i = 0; i < traffic.size(); i++)
      if (traffic[i].ID == id)
        return &traffic[i];

    return NULL;
  }

  /**
   * Looks up an item in the traffic list.
   *
   * @param name the name or call sign
   * @return the FLARM_TRAFFIC pointer, NULL if not found
   */
  FLARM_TRAFFIC *FindTraffic(const TCHAR *name) {
    for (unsigned i = 0; i < traffic.size(); i++)
      if (traffic[i].Name.equals(name))
        return &traffic[i];

    return NULL;
  }

  /**
   * Looks up an item in the traffic list.
   *
   * @param name the name or call sign
   * @return the FLARM_TRAFFIC pointer, NULL if not found
   */
  const FLARM_TRAFFIC *FindTraffic(const TCHAR *name) const {
    for (unsigned i = 0; i < traffic.size(); i++)
      if (traffic[i].Name.equals(name))
        return &traffic[i];

    return NULL;
  }

  /**
   * Allocates a new FLARM_TRAFFIC object from the array.
   *
   * @return the FLARM_TRAFFIC pointer, NULL if the array is full
   */
  FLARM_TRAFFIC *AllocateTraffic() {
    return traffic.full()
      ? NULL
      : &traffic.append();
  }

  /**
   * Search for the previous traffic in the ordered list.
   */
  const FLARM_TRAFFIC *PreviousTraffic(const FLARM_TRAFFIC *t) const {
    return t > traffic.begin()
      ? t - 1
      : NULL;
  }

  /**
   * Search for the next traffic in the ordered list.
   */
  const FLARM_TRAFFIC *NextTraffic(const FLARM_TRAFFIC *t) const {
    return t + 1 < traffic.end()
      ? t + 1
      : NULL;
  }

  /**
   * Search for the first traffic in the ordered list.
   */
  const FLARM_TRAFFIC *FirstTraffic() const {
    return traffic.empty() ? NULL : traffic.begin();
  }

  /**
   * Search for the last traffic in the ordered list.
   */
  const FLARM_TRAFFIC *LastTraffic() const {
    return traffic.empty() ? NULL : traffic.end() - 1;
  }

  /**
   * Finds the most critical alert.  Returns NULL if there is no
   * alert.
   */
  const FLARM_TRAFFIC *FindMaximumAlert() const;

  unsigned TrafficIndex(const FLARM_TRAFFIC *t) const {
    return t - traffic.begin();
  }

  void Refresh(fixed Time) {
    available.Expire(Time, fixed(10));
    if (!available)
      traffic.clear();

    for (unsigned i = traffic.size(); i-- > 0;)
      if (!traffic[i].Refresh(Time))
        traffic.quick_remove(i);

    NewTraffic = false;
  }
};

#endif
