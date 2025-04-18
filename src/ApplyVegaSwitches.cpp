// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ApplyVegaSwitches.hpp"
#include "Interface.hpp"
#include "Input/InputQueue.hpp"
#include "thread/Debug.hpp"

static VegaSwitchState last_vega_switches;

void
ApplyVegaSwitches()
{
  assert(InMainThread());

  const VegaSwitchState &switches = CommonInterface::Basic().switch_state.vega;

  // detect changes to ON: on now (x) and not on before (!lastx)
  // detect changes to OFF: off now (!x) and on before (lastx)

  const unsigned up_inputs = switches.inputs & ~last_vega_switches.inputs;
  const unsigned down_inputs = ~switches.inputs & last_vega_switches.inputs;
  const unsigned up_outputs = switches.outputs & ~last_vega_switches.outputs;
  const unsigned down_outputs = ~switches.outputs & last_vega_switches.outputs;
  last_vega_switches = switches;

  for (unsigned i = 0; i < 32; ++i) {
    const unsigned thebit = 1 << i;

    if (down_inputs & thebit)
      InputEvents::processNmea(i);
    else if (up_inputs & thebit)
      InputEvents::processNmea(i + 64);

    if (down_outputs & thebit)
      InputEvents::processNmea(i + 32);
    else if (up_outputs & thebit)
      InputEvents::processNmea(i + 96);
  }

}
