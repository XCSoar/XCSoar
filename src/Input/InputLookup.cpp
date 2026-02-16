// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputLookup.hpp"
#include "InputEvents.hpp"
#include "InputQueue.hpp"
#include "util/StringAPI.hxx"

// Mapping text names of events to the real thing
struct Text2EventSTRUCT {
  const char *text;
  pt2Event event;
};

static constexpr Text2EventSTRUCT Text2Event[] = {
#include "InputEvents_Text2Event.cpp"
  { nullptr, nullptr }
};

// Mapping text names of events to the real thing
static const char *const Text2GCE[] = {
#include "InputEvents_Text2GCE.cpp"
  nullptr
};

// Mapping text names of events to the real thing
static const char *const Text2NE[] = {
#include "InputEvents_Text2NE.cpp"
  nullptr
};

pt2Event
InputEvents::findEvent(tstring_view name) noexcept
{
  for (unsigned i = 0; Text2Event[i].text != nullptr; ++i)
    if (name == Text2Event[i].text)
      return Text2Event[i].event;

  return nullptr;
}

int
InputEvents::findGCE(const char *data)
{
  int i;
  for (i = 0; i < GCE_COUNT; i++) {
    if (StringIsEqual(data, Text2GCE[i]))
      return i;
  }

  return -1;
}

int
InputEvents::findNE(const char *data)
{
  int i;
  for (i = 0; i < NE_COUNT; i++) {
    if (StringIsEqual(data, Text2NE[i]))
      return i;
  }

  return -1;
}
