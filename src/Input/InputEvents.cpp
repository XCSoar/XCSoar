// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
doc/html/advanced/input/ALL
https://xcsoar.readthedocs.io/en/latest/input_events.html

*/

#include "InputEvents.hpp"
#include "InputConfig.hpp"
#include "InputParser.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Protection.hpp"
#include "Menu/ButtonLabel.hpp"
#include "Profile/Keys.hpp"
#include "Menu/MenuData.hpp"
#include "io/ConfiguredFile.hpp"
#include "io/FileReader.hxx"
#include "io/BufferedReader.hxx"
#include "Pan.hpp"
#include "Dialogs/LockScreen.hpp"
#include "Menu/MenuBar.hpp"
#include "MapWindow/GlueMapWindow.hpp"

#ifdef KOBO
#include "ui/event/KeyCode.hpp"
#endif

#include "lua/InputEvent.hpp"

#include <cassert>
#include <tchar.h>
#include <stdio.h>
#include <memory>

namespace InputEvents {

static const TCHAR *flavour;

static Mode current_mode = InputEvents::MODE_DEFAULT;

/**
 * A mode that overrides the #current_mode.  Only if a value does
 * not exist in this mode, it will be taken from the #current_mode.
 * The special value #MODE_DEFAULT means there is no overlay mode.
 */
static Mode overlay_mode = MODE_DEFAULT;

static std::chrono::duration<unsigned> MenuTimeOut{};

/**
 * True if a full menu update was postponed by drawButtons().
 */
static bool menu_dirty = false;

[[gnu::pure]]
static Mode
getModeID() noexcept;

static void
UpdateOverlayMode() noexcept;

[[gnu::pure]]
static unsigned
gesture_to_event(const TCHAR *data) noexcept;

/**
 * @param full if false, update only the dynamic labels
 */
static void
drawButtons(Mode mode, bool full=false) noexcept;

static void
ProcessMenuTimer() noexcept;

static void
processGo(unsigned event_id) noexcept;

} // namespace InputEvents

static InputConfig input_config;

// Read the data files
void
InputEvents::readFile()
{
  // clear the GCE and NMEA queues
  ClearQueues();

  LoadDefaults(input_config);

  // Read in user defined configuration file
  auto reader = OpenConfiguredFile(ProfileKeys::InputFile);
  if (reader) {
    BufferedReader buffered_reader{*reader};
    ::ParseInputFile(input_config, buffered_reader);
  }
}

void
InputEvents::setMode(Mode mode) noexcept
{
  assert((unsigned)mode < input_config.modes.size());

  if (mode == current_mode)
    return;

  current_mode = mode;
  UpdateOverlayMode();

  drawButtons(getModeID(), true);
}

void
InputEvents::setMode(const TCHAR *mode) noexcept
{
  int m = input_config.LookupMode(mode);
  if (m >= 0)
    setMode((InputEvents::Mode)m);
}

void
InputEvents::UpdatePan() noexcept
{
  drawButtons(getModeID(), true);
}

void
InputEvents::SetFlavour(const TCHAR *_flavour) noexcept
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
InputEvents::IsFlavour(const TCHAR *_flavour) noexcept
{
  if (flavour == NULL)
    return _flavour == NULL;

  if (_flavour == NULL)
    return false;

  return StringIsEqual(flavour, _flavour);
}

bool
InputEvents::IsDefault() noexcept
{
  return current_mode == MODE_DEFAULT;
}

void
InputEvents::drawButtons(Mode mode, bool full) noexcept
{
  if (!global_running)
    return;

  if (CommonInterface::main_window->HasDialog()) {
    /* don't activate the menu if a modal dialog is visible; the menu
       buttons would be put above the dialog, but would not be
       accessible; instead, postpone */
    if (full)
      menu_dirty = true;
    return;
  }

  full |= std::exchange(menu_dirty, false);

  const Menu &menu = input_config.menus[mode];
  const Menu *const overlay_menu = overlay_mode != MODE_DEFAULT
    ? &input_config.menus[overlay_mode]
    : NULL;

  CommonInterface::main_window->ShowMenu(menu, overlay_menu, full);

  GlueMapWindow *map = CommonInterface::main_window->GetMapIfActive();
  if (map != nullptr){
      if (mode != MODE_DEFAULT){
        /* Adjust the margin to ensure that GlueMapWindow elements,
         * such as the scale, are not overdraw by the buttons
         * when in Pan mode. */
        map->SetBottomMarginFactor(menubar_height_scale_factor);
      } else {
          map->SetBottomMarginFactor(0);
      }
  }
}

InputEvents::Mode
InputEvents::getModeID() noexcept
{
  if (current_mode == MODE_DEFAULT && IsPanning())
    return MODE_PAN;

  return current_mode;
}

void
InputEvents::UpdateOverlayMode() noexcept
{
  if (flavour != NULL) {
    /* build the "flavoured" mode name from the current "major" mode
       and the flavour name */
    StaticString<InputConfig::MAX_MODE_STRING + 32> name;
    name.Format(_T("%s.%s"), input_config.modes[current_mode].c_str(),
                flavour);

    /* see if it exists */
    int new_mode = input_config.LookupMode(name.c_str());
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

[[gnu::pure]]
static int
FindMenuItemByEvent(InputEvents::Mode mode, InputEvents::Mode overlay_mode,
                    unsigned event_id) noexcept
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
InputEvents::ProcessEvent(unsigned event_id) noexcept
{
  assert(event_id != 0);

  InputEvents::Mode lastMode = getModeID();

  int bindex = FindMenuItemByEvent(lastMode, overlay_mode, event_id);
  if (bindex < 0 ||
      CommonInterface::main_window->IsMenuButtonEnabled(bindex))
    InputEvents::processGo(event_id);

  // experimental: update button text, macro may change the value
  if (lastMode == getModeID() && bindex > 0)
    drawButtons(lastMode);
}

/**
 * Looks up the specified key code, and returns the associated event
 * id.  Returns 0 if the key was not found.
 */
[[gnu::pure]]
static unsigned
key_to_event(InputEvents::Mode mode, unsigned key_code) noexcept
{
  return input_config.GetKeyEvent(mode, key_code);
}

[[gnu::pure]]
static unsigned
key_to_event(InputEvents::Mode mode, InputEvents::Mode overlay_mode,
             unsigned key_code) noexcept
{
  if (overlay_mode != InputEvents::MODE_DEFAULT) {
    unsigned event_id = key_to_event(overlay_mode, key_code);
    if (event_id > 0)
      return event_id;
  }

  return key_to_event(mode, key_code);
}

bool
InputEvents::ProcessKey(Mode mode, unsigned key_code) noexcept
{
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

  if (Lua::FireKey(key_code)) {
    //    return true;
  }

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
InputEvents::processKey(unsigned key_code) noexcept
{
  return ProcessKey(getModeID(), key_code);
}

unsigned
InputEvents::gesture_to_event(const TCHAR *data) noexcept
{
  return input_config.Gesture2Event.Get(data, 0);
}

bool
InputEvents::IsGesture(const TCHAR *data) noexcept
{
  return (Lua::IsGesture(data)) || (gesture_to_event(data) != 0);
}

bool
InputEvents::processGesture(const TCHAR *data) noexcept
{
  // start with lua event if available!
  if (Lua::FireGesture(data))
    return true;

  // get current mode
  unsigned event_id = gesture_to_event(data);
  if (event_id) {
    InputEvents::processGo(event_id);
    return true;
  }

  return false;
}

/*
  InputEvent::processNmea(TCHAR *data)
  Take hard coded inputs from NMEA processor.
  Return = TRUE if we have a valid key match
*/
bool
InputEvents::processNmea_real(unsigned ne_id) noexcept
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
  return Lua::FireNMEAEvent(ne_id);
}

/*
  InputEvents::processGlideComputer
  Take virtual inputs from a Glide Computer to do special events
*/
bool
InputEvents::processGlideComputer_real(unsigned gce_id) noexcept
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

  return Lua::FireGlideComputerEvent(gce_id);
}

// EXECUTE an Event - lookup event handler and call back - no return
void
InputEvents::processGo(unsigned eventid) noexcept
{
  /* eventid 0 is special for "noop" */

  while (global_running && eventid > 0) {
    const InputConfig::Event &event = input_config.events[eventid];
    if (event.event != NULL) {
      event.event(event.misc);
      MenuTimeOut = {};
    }

    eventid = event.next;
  }
}

void
InputEvents::HideMenu() noexcept
{
  setMode(MODE_DEFAULT);
}

void
InputEvents::ShowMenu() noexcept
{
  setMode(MODE_MENU);
  MenuTimeOut = {};
  ProcessMenuTimer();
}

Menu *
InputEvents::GetMenu(const TCHAR *mode) noexcept
{
 int m = input_config.LookupMode(mode);
 if (m >= 0)
   return &input_config.menus[m];
 else
   return NULL;
}

void
InputEvents::ProcessMenuTimer() noexcept
{
  if (CommonInterface::main_window->HasDialog())
    /* no menu updates while a dialog is visible */
    return;

  if (MenuTimeOut == CommonInterface::GetUISettings().menu_timeout)
    HideMenu();

  // refresh visible buttons if still visible
  drawButtons(getModeID());

  MenuTimeOut += std::chrono::seconds{1};
}

void
InputEvents::ProcessTimer() noexcept
{
  DoQueuedEvents();
  ProcessMenuTimer();
}

void
InputEvents::eventLockScreen([[maybe_unused]] const TCHAR *mode)
{
  ShowLockBox();
}
