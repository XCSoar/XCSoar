/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

/*

InputEvents

This class is used to control all user and extrnal InputEvents.
This includes some Nmea strings, virtual events (Glide Computer
Evnets) and Keyboard.

What it does not cover is Glide Computer normal processing - this
includes GPS and Vario processing.

What it does include is what to do when an automatic event (switch
to Climb mode) and user events are entered.

It also covers the configuration side of on screen labels.

For further information on config file formats see

source/Common/Data/Input/ALL
doc/html/advanced/input/ALL		http://xcsoar.sourceforge.net/advanced/input/

*/

#include "InputEvents.hpp"
#include "InputConfig.hpp"
#include "InputParser.hpp"
#include "UIActions.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Protection.hpp"
#include "LogFile.hpp"
#include "Menu/ButtonLabel.hpp"
#include "Profile/Profile.hpp"
#include "LocalPath.hpp"
#include "Asset.hpp"
#include "Menu/MenuData.hpp"
#include "IO/ConfiguredFile.hpp"
#include "MapSettings.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Language/Language.hpp"
#include "Pan.hpp"

#ifdef KOBO
#include "Screen/Key.h"
#endif

#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory>

namespace InputEvents
{
  static const TCHAR *flavour;

  static Mode current_mode = InputEvents::MODE_DEFAULT;

  /**
   * A mode that overrides the #current_mode.  Only if a value does
   * not exist in this mode, it will be taken from the #current_mode.
   * The special value #MODE_DEFAULT means there is no overlay mode.
   */
  static Mode overlay_mode = MODE_DEFAULT;

  static unsigned MenuTimeOut = 0;

  gcc_pure
  static Mode getModeID();

  static void UpdateOverlayMode();

  gcc_pure
  static unsigned gesture_to_event(const TCHAR *data);

  /**
   * @param full if false, update only the dynamic labels
   */
  static void drawButtons(Mode mode, bool full=false);

  static void ProcessMenuTimer();

  static void processGo(unsigned event_id);
};

static InputConfig input_config;

// Read the data files
void
InputEvents::readFile()
{
  LogFormat("Loading input events file");

  // clear the GCE and NMEA queues
  ClearQueues();

  LoadDefaults(input_config);

  // Read in user defined configuration file
  std::unique_ptr<TLineReader> reader(OpenConfiguredTextFile(ProfileKeys::InputFile));
  if (reader)
    ::ParseInputFile(input_config, *reader);
}

void
InputEvents::setMode(Mode mode)
{
  assert((unsigned)mode < input_config.modes.size());

  if (mode == current_mode)
    return;

  current_mode = mode;
  UpdateOverlayMode();

  drawButtons(getModeID(), true);
}

void
InputEvents::setMode(const TCHAR *mode)
{
  int m = input_config.LookupMode(mode);
  if (m >= 0)
    setMode((InputEvents::Mode)m);
}

void
InputEvents::UpdatePan()
{
  drawButtons(getModeID(), true);
}

void
InputEvents::SetFlavour(const TCHAR *_flavour)
{
  if (flavour == NULL && _flavour == NULL)
    /* optimised default case */
    return;

  flavour = _flavour;

  const Mode old_overlay_mode = overlay_mode;
  UpdateOverlayMode();

  if (overlay_mode != old_overlay_mode)
    /* the overlay_mode has changed, update the displayed menu */
    drawButtons(getModeID(), true);
}

bool
InputEvents::IsFlavour(const TCHAR *_flavour)
{
  if (flavour == NULL)
    return _flavour == NULL;

  if (_flavour == NULL)
    return false;

  return StringIsEqual(flavour, _flavour);
}

bool
InputEvents::IsDefault()
{
  return current_mode == MODE_DEFAULT;
}


void
InputEvents::drawButtons(Mode mode, bool full)
{
  if (!global_running)
    return;

  const Menu &menu = input_config.menus[mode];
  const Menu *const overlay_menu = overlay_mode != MODE_DEFAULT
    ? &input_config.menus[overlay_mode]
    : NULL;

  ButtonLabel::Set(menu, overlay_menu, full);
}

InputEvents::Mode
InputEvents::getModeID()
{
  if (current_mode == MODE_DEFAULT && IsPanning())
    return MODE_PAN;

  return current_mode;
}

void
InputEvents::UpdateOverlayMode()
{
  if (flavour != NULL) {
    /* build the "flavoured" mode name from the current "major" mode
       and the flavour name */
    StaticString<InputConfig::MAX_MODE_STRING + 32> name;
    name.Format(_T("%s.%s"), input_config.modes[current_mode].c_str(),
                flavour);

    /* see if it exists */
    int new_mode = input_config.LookupMode(name);
    if (new_mode >= 0)
      /* yep, it does */
      overlay_mode = (Mode)new_mode;
    else
      /* not defined, disable the overlay with the magic value
         "MODE_DEFAULT" */
      overlay_mode = MODE_DEFAULT;
  } else
    overlay_mode = MODE_DEFAULT;
}

// -----------------------------------------------------------------------
// Processing functions - which one to do
// -----------------------------------------------------------------------

gcc_pure
static int
FindMenuItemByEvent(InputEvents::Mode mode, InputEvents::Mode overlay_mode,
                    unsigned event_id)
{
  const Menu *const overlay_menu = overlay_mode != InputEvents::MODE_DEFAULT
    ? &input_config.menus[overlay_mode]
    : NULL;
  if (overlay_menu != NULL) {
    int i = overlay_menu->FindByEvent(event_id);
    if (i >= 0)
      return i;
  }

  const Menu &menu = input_config.menus[mode];
  int i = menu.FindByEvent(event_id);
  if (i >= 0 && overlay_menu != NULL && (*overlay_menu)[i].IsDefined())
    /* this location is in use by the overlay */
    i = -1;

  return i;
}

void
InputEvents::ProcessEvent(unsigned event_id)
{
  assert(event_id != 0);

  InputEvents::Mode lastMode = getModeID();

  int bindex = FindMenuItemByEvent(lastMode, overlay_mode, event_id);
  if (bindex < 0 || ButtonLabel::IsEnabled(bindex))
    InputEvents::processGo(event_id);

  // experimental: update button text, macro may change the value
  if (lastMode == getModeID() && bindex > 0)
    drawButtons(lastMode);
}

/**
 * Looks up the specified key code, and returns the associated event
 * id.  Returns 0 if the key was not found.
 */
gcc_pure
static unsigned
key_to_event(InputEvents::Mode mode, unsigned key_code)
{
  return input_config.GetKeyEvent(mode, key_code);
}

gcc_pure
static unsigned
key_to_event(InputEvents::Mode mode, InputEvents::Mode overlay_mode,
             unsigned key_code)
{
  if (overlay_mode != InputEvents::MODE_DEFAULT) {
    unsigned event_id = key_to_event(overlay_mode, key_code);
    if (event_id > 0)
      return event_id;
  }

  return key_to_event(mode, key_code);
}

bool
InputEvents::ProcessKey(Mode mode, unsigned key_code)
{
  if (IsAltair() && key_code == 0xF5) {
    UIActions::SignalShutdown(false);
    return true;
  }

  if (!global_running)
    return false;

#ifdef KOBO
#ifdef ENABLE_SDL
  if (key_code == SDLK_POWER)
    /* the Kobo power button opens the main menu */
    key_code = KEY_MENU;
#else
  // TODO: check the console key code
#endif
#endif

  // Which key - can be defined locally or at default (fall back to default)
  unsigned event_id = key_to_event(mode, overlay_mode, key_code);
  if (event_id == 0)
    return false;

  ProcessEvent(event_id);
  return true;
}

/*
  InputEvent::processKey(KeyID);
  Process keys normally brought in by hardware or keyboard presses
  Future will also allow for long and double click presses...
  Return = We had a valid key (even if nothing happens because of Bounce)
*/
bool
InputEvents::processKey(unsigned key_code)
{
  return ProcessKey(getModeID(), key_code);
}

unsigned
InputEvents::gesture_to_event(const TCHAR *data)
{
  return input_config.Gesture2Event.Get(data, 0);
}

bool
InputEvents::IsGesture(const TCHAR *data)
{
  return gesture_to_event(data) != 0;
}

bool
InputEvents::processGesture(const TCHAR *data)
{
  // get current mode
  unsigned event_id = gesture_to_event(data);
  if (event_id)
  {
    InputEvents::processGo(event_id);
    return true;
  }
  return false;
}

/*
  InputEvent::processNmea(TCHAR* data)
  Take hard coded inputs from NMEA processor.
  Return = TRUE if we have a valid key match
*/
bool
InputEvents::processNmea_real(unsigned ne_id)
{
  if (!global_running)
    return false;

  int event_id = 0;

  // Valid input ?
  if (ne_id >= NE_COUNT)
    return false;

  event_id = input_config.N2Event[ne_id];
  if (event_id > 0) {
    InputEvents::processGo(event_id);
    return true;
  }

  return false;
}

/*
  InputEvents::processGlideComputer
  Take virtual inputs from a Glide Computer to do special events
*/
bool
InputEvents::processGlideComputer_real(unsigned gce_id)
{
  if (!global_running)
    return false;
  int event_id = 0;

  // TODO feature: Log glide computer events to IGC file

  // Valid input ?
  if (gce_id >= GCE_COUNT)
    return false;

  event_id = input_config.GC2Event[gce_id];
  if (event_id > 0) {
    InputEvents::processGo(event_id);
    return true;
  }

  return false;
}

// EXECUTE an Event - lookup event handler and call back - no return
void
InputEvents::processGo(unsigned eventid)
{
  /* eventid 0 is special for "noop" */

  while (global_running && eventid > 0) {
    const InputConfig::Event &event = input_config.events[eventid];
    if (event.event != NULL) {
      event.event(event.misc);
      MenuTimeOut = 0;
    }

    eventid = event.next;
  }
}

void
InputEvents::HideMenu()
{
  setMode(MODE_DEFAULT);
}

void
InputEvents::ShowMenu()
{
  setMode(MODE_MENU);
  MenuTimeOut = 0;
  ProcessMenuTimer();
}

Menu *
InputEvents::GetMenu(const TCHAR *mode)
{
 int m = input_config.LookupMode(mode);
 if (m >= 0)
   return &input_config.menus[m];
 else
   return NULL;
}

void
InputEvents::ProcessMenuTimer()
{
  if (CommonInterface::main_window->HasDialog())
    /* no menu updates while a dialog is visible */
    return;

  if (MenuTimeOut == CommonInterface::GetUISettings().menu_timeout)
    HideMenu();

  // refresh visible buttons if still visible
  drawButtons(getModeID());

  MenuTimeOut++;
}

void
InputEvents::ProcessTimer()
{
  DoQueuedEvents();
  ProcessMenuTimer();
}
