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

/**
 * Received FLARM data, cached
 */
struct FLARM_STATE
{
  enum {
    FLARM_MAX_TRAFFIC = 15,
  };

  /** Number of received FLARM devices */
  unsigned short FLARM_RX;
  /** Transmit status */
  unsigned short FLARM_TX;
  /** GPS status */
  unsigned short FLARM_GPS;
  /** Alarm level of FLARM (0-3) */
  unsigned short FLARM_AlarmLevel;
  /** Is FLARM information available? */
  bool FLARM_Available;
  /** Flarm traffic information */
  FLARM_TRAFFIC FLARM_Traffic[FLARM_MAX_TRAFFIC];
  /**
   * Is there FLARM traffic present?
   * @see FLARM_Traffic
   */
  bool FLARMTraffic;
  /**
   * Is there new FLARM traffic present?
   * @see FLARM_Traffic
   */
  bool NewTraffic;

protected:
  const FLARM_TRAFFIC *FirstTrafficSlot() const {
    return &FLARM_Traffic[0];
  }

  const FLARM_TRAFFIC *LastTrafficSlot() const {
    return &FLARM_Traffic[FLARM_MAX_TRAFFIC - 1];
  }

public:

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   */
  void complement(const FLARM_STATE &add) {
    if (!FLARM_Available && add.FLARM_Available)
      *this = add;
  }

  unsigned GetActiveTrafficCount() const {
    unsigned count = 0;

    for (unsigned i = 0; i < FLARM_MAX_TRAFFIC; i++)
      if (FLARM_Traffic[i].defined())
        ++count;

    return count;
  }

  /**
   * Looks up an item in the traffic list.
   *
   * @param id FLARM id
   * @return the FLARM_TRAFFIC pointer, NULL if not found
   */
  FLARM_TRAFFIC *FindTraffic(FlarmId id) {
    for (unsigned i = 0; i < FLARM_MAX_TRAFFIC; i++)
      if (FLARM_Traffic[i].ID == id)
        return &FLARM_Traffic[i];

    return NULL;
  }

  /**
   * Looks up an item in the traffic list.
   *
   * @param id FLARM id
   * @return the FLARM_TRAFFIC pointer, NULL if not found
   */
  const FLARM_TRAFFIC *FindTraffic(FlarmId id) const {
    for (unsigned i = 0; i < FLARM_MAX_TRAFFIC; i++)
      if (FLARM_Traffic[i].ID == id)
        return &FLARM_Traffic[i];

    return NULL;
  }

  /**
   * Looks up an item in the traffic list.
   *
   * @param name the name or call sign
   * @return the FLARM_TRAFFIC pointer, NULL if not found
   */
  FLARM_TRAFFIC *FindTraffic(const TCHAR *name) {
    for (unsigned i = 0; i < FLARM_MAX_TRAFFIC; i++)
      if (FLARM_Traffic[i].defined() &&
          _tcscmp(FLARM_Traffic[i].Name, name) == 0)
        return &FLARM_Traffic[i];

    return NULL;
  }

  /**
   * Looks up an item in the traffic list.
   *
   * @param name the name or call sign
   * @return the FLARM_TRAFFIC pointer, NULL if not found
   */
  const FLARM_TRAFFIC *FindTraffic(const TCHAR *name) const {
    for (unsigned i = 0; i < FLARM_MAX_TRAFFIC; i++)
      if (FLARM_Traffic[i].defined() &&
          _tcscmp(FLARM_Traffic[i].Name, name) == 0)
        return &FLARM_Traffic[i];

    return NULL;
  }

  /**
   * Allocates a new FLARM_TRAFFIC object from the array.
   *
   * @return the FLARM_TRAFFIC pointer, NULL if the array is full
   */
  FLARM_TRAFFIC *AllocateTraffic() {
    for (unsigned i = 0; i < FLARM_MAX_TRAFFIC; i++)
      if (!FLARM_Traffic[i].defined())
        return &FLARM_Traffic[i];

    return NULL;
  }

  /**
   * Search for the previous traffic in the ordered list.
   */
  const FLARM_TRAFFIC *PreviousTraffic(const FLARM_TRAFFIC *t) const {
    while (--t >= FirstTrafficSlot()) {
      if (t->defined())
        return t;
    }

    return NULL;
  }

  /**
   * Search for the next traffic in the ordered list.
   */
  const FLARM_TRAFFIC *NextTraffic(const FLARM_TRAFFIC *t) const {
    while (++t <= LastTrafficSlot()) {
      if (t->defined())
        return t;
    }

    return NULL;
  }

  /**
   * Search for the first traffic in the ordered list.
   */
  const FLARM_TRAFFIC *FirstTraffic() const {
    return NextTraffic(FirstTrafficSlot() - 1);
  }

  /**
   * Search for the last traffic in the ordered list.
   */
  const FLARM_TRAFFIC *LastTraffic() const {
    return PreviousTraffic(LastTrafficSlot() + 1);
  }

  /**
   * Finds the most critical alert.  Returns NULL if there is no
   * alert.
   */
  const FLARM_TRAFFIC *FindMaximumAlert() const;

  unsigned TrafficIndex(const FLARM_TRAFFIC *traffic) const {
    return traffic - FirstTrafficSlot();
  }

  void Refresh(fixed Time) {
    bool present = false;

    if (FLARM_Available) {
      for (unsigned i = 0; i < FLARM_MAX_TRAFFIC; i++) {
        if (FLARM_Traffic[i].Refresh(Time))
          present = true;
      }
    }

    FLARMTraffic = present;
    NewTraffic = false;
  }
};

#endif
