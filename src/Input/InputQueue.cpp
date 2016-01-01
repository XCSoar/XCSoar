/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "InputQueue.hpp"
#include "InputEvents.hpp"
#include "Thread/Mutex.hpp"
#include "Thread/Debug.hpp"
#include "Util/Macros.hpp"

#include <algorithm>

static Mutex mutexEventQueue;

#define MAX_GCE_QUEUE 10
static int GCE_Queue[MAX_GCE_QUEUE];
#define MAX_NMEA_QUEUE 10
static int NMEA_Queue[MAX_NMEA_QUEUE];

void
InputEvents::ClearQueues()
{
  ScopeLock protect(mutexEventQueue);

  std::fill_n(GCE_Queue, ARRAY_SIZE(GCE_Queue), -1);
  std::fill_n(NMEA_Queue, ARRAY_SIZE(NMEA_Queue), -1);
}

bool
InputEvents::processNmea(unsigned ne_id)
{
  assert(InMainThread());

  // add an event to the bottom of the queue
  for (int i = 0; i < MAX_NMEA_QUEUE; i++) {
    if (NMEA_Queue[i] == -1) {
      NMEA_Queue[i] = ne_id;
      break;
    }
  }

  return true; // ok.
}


void
InputEvents::DoQueuedEvents()
{
  assert(InMainThread());

  int GCE_Queue_copy[MAX_GCE_QUEUE];
  int i;

  // copy the queue first, blocking
  {
    const ScopeLock lock(mutexEventQueue);
    std::copy_n(GCE_Queue, MAX_GCE_QUEUE, GCE_Queue_copy);
    std::fill_n(GCE_Queue, MAX_GCE_QUEUE, -1);
  }

  // process each item in the queue
  for (i = 0; i < MAX_GCE_QUEUE; i++) {
    if (GCE_Queue_copy[i] != -1) {
      processGlideComputer_real(GCE_Queue_copy[i]);
    }
  }
  for (i = 0; i < MAX_NMEA_QUEUE; i++) {
    if (NMEA_Queue[i] != -1)
      processNmea_real(NMEA_Queue[i]);
  }
}

bool
InputEvents::processGlideComputer(unsigned gce_id)
{
  // add an event to the bottom of the queue
  ScopeLock protect(mutexEventQueue);

  for (int i = 0; i < MAX_GCE_QUEUE; i++) {
    if (GCE_Queue[i] == -1) {
      GCE_Queue[i] = gce_id;
      break;
    }
  }

  return true;
}
