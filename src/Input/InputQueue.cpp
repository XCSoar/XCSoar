// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputQueue.hpp"
#include "InputEvents.hpp"
#include "thread/Mutex.hxx"
#include "thread/Debug.hpp"
#include "util/Macros.hpp"

#include <algorithm>

#include <cassert>

static Mutex mutexEventQueue;

static constexpr std::size_t MAX_GCE_QUEUE = 10;
static int GCE_Queue[MAX_GCE_QUEUE];
static constexpr std::size_t MAX_NMEA_QUEUE = 10;
static int NMEA_Queue[MAX_NMEA_QUEUE];

void
InputEvents::ClearQueues()
{
  const std::lock_guard lock{mutexEventQueue};

  std::fill_n(GCE_Queue, ARRAY_SIZE(GCE_Queue), -1);
  std::fill_n(NMEA_Queue, ARRAY_SIZE(NMEA_Queue), -1);
}

bool
InputEvents::processNmea(unsigned ne_id)
{
  assert(InMainThread());

  // add an event to the bottom of the queue
  for (unsigned i = 0; i < MAX_NMEA_QUEUE; i++) {
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
  unsigned i;

  // copy the queue first, blocking
  {
    const std::lock_guard lock{mutexEventQueue};
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
  const std::lock_guard lock{mutexEventQueue};

  for (unsigned i = 0; i < MAX_GCE_QUEUE; i++) {
    if (GCE_Queue[i] == -1) {
      GCE_Queue[i] = gce_id;
      break;
    }
  }

  return true;
}
