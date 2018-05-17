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

#include "ApplyVegaSwitches.hpp"
#include "Interface.hpp"
#include "Input/InputQueue.hpp"
#include "Thread/Debug.hpp"

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
