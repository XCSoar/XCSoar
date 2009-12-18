/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "InputEvents.h"
#include "Protection.hpp"
#include "LogFile.hpp"
#include "Compatibility/vk.h"
#include "InfoBoxLayout.h"
#include "ButtonLabel.hpp"
#include "Registry.hpp"
#include "LocalPath.hpp"
#include "MacCready.h"
#include "UtilsText.hpp"
#include "StringUtil.hpp"
#include "Asset.hpp"

#include <assert.h>
#include <ctype.h>
#include <tchar.h>

// Sensible maximums
#define MAX_MODE 100
#define MAX_MODE_STRING 25
#define MAX_KEY 255
#define MAX_EVENTS 2048
#define MAX_LABEL ButtonLabel::NUMBUTTONLABELS

/*
  TODO code - All of this input_Errors code needs to be removed and
  replaced with standard logger.  The logger can then display messages
  through Message if ncessary and log to files etc This code, and
  baddly written #ifdef should be moved to Macros in the Log class.
*/

#ifdef _INPUTDEBUG_
// Log first NN input event errors for display in simulator mode
#define MAX_INPUT_ERRORS 5
TCHAR input_errors[MAX_INPUT_ERRORS][3000];
int input_errors_count = 0;
// JMW this is just far too annoying right now,
// since "title" "note" and commencts are not parsed, they
// come up as errors.
#endif

// Current modes - map mode to integer (primitive hash)
InputEvents::mode InputEvents::current_mode = InputEvents::MODE_DEFAULT;

/** Map mode to location */
static TCHAR mode_map[MAX_MODE][MAX_MODE_STRING] = {
  _T("default"),
  _T("pan"),
  _T("infobox"),
  _T("Menu"),
};

static int mode_map_count = 4;

// Key map to Event - Keys (per mode) mapped to events
static int Key2Event[MAX_MODE][MAX_KEY];		// Points to Events location

// Glide Computer Events
static int GC2Event[MAX_MODE][GCE_COUNT];

// NMEA Triggered Events
static int N2Event[MAX_MODE][NE_COUNT];

// Events - What do you want to DO
typedef struct {
  // Which function to call (can be any, but should be here)
  pt2Event event;
  // Parameters
  const TCHAR *misc;
  // Next in event list - eg: Macros
  int next;
} EventSTRUCT;

static EventSTRUCT Events[MAX_EVENTS];
// How many have we defined
static int Events_count;

// Labels - defined per mode
typedef struct {
  const TCHAR *label;
  int location;
  int event;
} ModeLabelSTRUCT;

static ModeLabelSTRUCT ModeLabel[MAX_MODE][MAX_LABEL];
static int ModeLabel_count[MAX_MODE];	       // Where are we up to in this mode...

#define MAX_GCE_QUEUE 10
static int GCE_Queue[MAX_GCE_QUEUE];
#define MAX_NMEA_QUEUE 10
static int NMEA_Queue[MAX_NMEA_QUEUE];

// -----------------------------------------------------------------------
// Initialisation and Defaults
// -----------------------------------------------------------------------

bool InitONCE = false;

// Mapping text names of events to the real thing
typedef struct {
  const TCHAR *text;
  pt2Event event;
} Text2EventSTRUCT;

Text2EventSTRUCT Text2Event[256];  // why 256?
int Text2Event_count;

// Mapping text names of events to the real thing
const TCHAR *Text2GCE[GCE_COUNT+1];

// Mapping text names of events to the real thing
const TCHAR *Text2NE[NE_COUNT+1];

Mutex InputEvents::mutexEventQueue;

// Read the data files
void
InputEvents::readFile()
{
  StartupStore(TEXT("Loading input events file\n"));

  // clear the GCE and NMEA queues
  mutexEventQueue.Lock();
  int i;
  for (i = 0; i < MAX_GCE_QUEUE; i++) {
    GCE_Queue[i] = -1;
  }
  for (i = 0; i < MAX_NMEA_QUEUE; i++) {
    NMEA_Queue[i] = -1;
  }
  mutexEventQueue.Unlock();

  // Get defaults
  if (!InitONCE) {
    if (is_fivv()) {
      #include "InputEvents_fivv.cpp"   // VENTA3
    } else if (!is_embedded()) {
      #include "InputEvents_pc.cpp"
    } else if (is_altair()) {
      #include "InputEvents_altair.cpp"
    } else {
      #include "InputEvents_defaults.cpp"
    }

    #include "InputEvents_Text2Event.cpp"
    InitONCE = true;
  }

  // Read in user defined configuration file
  TCHAR szFile1[MAX_PATH];
  FILE *fp = NULL;

  // Open file from registry
  GetRegistryString(szRegistryInputFile, szFile1, MAX_PATH);
  ExpandLocalPath(szFile1);
  SetRegistryString(szRegistryInputFile, TEXT("\0"));

  if (!string_is_empty(szFile1))
    fp = _tfopen(szFile1, TEXT("rt"));

  if (fp == NULL)
    return;

  // TODO code - Safer sizes, strings etc - use C++ (can scanf restrict length?)

  // Buffer for all
  TCHAR buffer[2049];
  // key from scanf
  TCHAR key[2049];
  // value from scanf
  TCHAR value[2049];
  TCHAR *new_label = NULL;
  int found;

  // Init first entry

  // Did we find some in the last loop...
  bool some_data = false;
  // Multiple modes (so large string)
  TCHAR d_mode[1024] = TEXT("");
  TCHAR d_type[256] = TEXT("");
  TCHAR d_data[256] = TEXT("");
  int event_id = 0;
  TCHAR d_label[256] = TEXT("");
  int d_location = 0;
  TCHAR d_event[256] = TEXT("");
  TCHAR d_misc[256] = TEXT("");

  int line = 0;

  // Read from the file
  // TODO code: What about \r - as in \r\n ?
  // TODO code: Note that ^# does not allow # in key - might be required (probably not)
  //   Better way is to separate the check for # and the scanf
  while (_fgetts(buffer, 2048, fp)
         && ((found = _stscanf(buffer, TEXT("%[^#=]=%[^\r\n][\r\n]"), key, value)) != EOF)) {

    line++;

    // experimental: if the first line is "#CLEAR" then the whole default config is cleared
    //               and can be overwritten by file
    if ((line == 1) && (_tcsstr(buffer, TEXT("#CLEAR")))) {
      memset(&Key2Event, 0, sizeof(Key2Event));
      memset(&GC2Event, 0, sizeof(GC2Event));
      memset(&Events, 0, sizeof(Events));
      memset(&ModeLabel, 0, sizeof(ModeLabel));
      memset(&ModeLabel_count, 0, sizeof(ModeLabel_count));
      Events_count = 0;
    }

    // Check valid line? If not valid, assume next record (primative, but works ok!)
    if ((buffer[0] == '\r') || (buffer[0] == '\n') || (buffer[0] == '\0')) {
      // General checks before continue...
      if (some_data && (d_mode != NULL) && (_tcscmp(d_mode, TEXT("")) != 0)) {

        TCHAR *token;

        // For each mode
        token = _tcstok(d_mode, TEXT(" "));

        // General errors - these should be true
        assert(d_location >= 0);
        assert(d_location < 1024); // Scott arbitrary limit
        assert(event_id >= 0);
        assert(d_mode != NULL);
        assert(d_type != NULL);
        assert(d_label != NULL);

        // These could indicate bad data - thus not an ASSERT (debug only)
        // assert(_tcslen(d_mode) < 1024);
        // assert(_tcslen(d_type) < 1024);
        // assert(_tcslen(d_label) < 1024);

        while (token != NULL) {

          // All modes are valid at this point
          mode mode_id = mode2int(token, true);
          assert(mode_id != MODE_INVALID);

          // Make label event
          // TODO code: Consider Reuse existing entries...
          if (d_location > 0) {
            // Only copy this once per object - save string space
            if (!new_label) {
              new_label = StringMallocParse(d_label);
            }
            InputEvents::makeLabel(mode_id, new_label, d_location, event_id);
          }

          // Make key (Keyboard input)
          // key - Hardware key or keyboard
          if (_tcscmp(d_type, TEXT("key")) == 0) {
            // Get the int key (eg: APP1 vs 'a')
            int key = findKey(d_data);
            if (key > 0)
              Key2Event[mode_id][key] = event_id;

            #ifdef _INPUTDEBUG_
            else if (input_errors_count < MAX_INPUT_ERRORS)
            _stprintf(input_errors[input_errors_count++], TEXT("Invalid key data: %s at %i"), d_data, line);
            #endif

          // Make gce (Glide Computer Event)
          // GCE - Glide Computer Event
          } else if (_tcscmp(d_type, TEXT("gce")) == 0) {
            // Get the int key (eg: APP1 vs 'a')
            int key = findGCE(d_data);
            if (key >= 0)
              GC2Event[mode_id][key] = event_id;

            #ifdef _INPUTDEBUG_
            else if (input_errors_count < MAX_INPUT_ERRORS)
            _stprintf(input_errors[input_errors_count++], TEXT("Invalid GCE data: %s at %i"), d_data, line);
            #endif

          // Make ne (NMEA Event)
          // NE - NMEA Event
          } else if (_tcscmp(d_type, TEXT("ne")) == 0) {
            // Get the int key (eg: APP1 vs 'a')
            int key = findNE(d_data);
            if (key >= 0)
              N2Event[mode_id][key] = event_id;

            #ifdef _INPUTDEBUG_
            else if (input_errors_count < MAX_INPUT_ERRORS)
            _stprintf(input_errors[input_errors_count++], TEXT("Invalid GCE data: %s at %i"), d_data, line);
            #endif

          // label only - no key associated (label can still be touch screen)
          } else if (_tcscmp(d_type, TEXT("label")) == 0) {
            // Nothing to do here...

          #ifdef _INPUTDEBUG_
          } else if (input_errors_count < MAX_INPUT_ERRORS) {
            _stprintf(input_errors[input_errors_count++], TEXT("Invalid type: %s at %i"), d_type, line);
          #endif

          }

          token = _tcstok(NULL, TEXT(" "));
        }
      }

      // Clear all data.
      some_data = false;
      _tcscpy(d_mode, TEXT(""));
      _tcscpy(d_type, TEXT(""));
      _tcscpy(d_data, TEXT(""));
      event_id = 0;
      _tcscpy(d_label, TEXT(""));
      d_location = 0;
      new_label = NULL;

    } else if ((found != 2) || key[0] == 0 || value[0] == 0) {
      // Do nothing - we probably just have a comment line
      // JG removed "void;" - causes warning (void is declaration and needs variable)
      // NOTE: Do NOT display buffer to user as it may contain an invalid stirng !

    } else {
      if (_tcscmp(key, TEXT("mode")) == 0) {
        if (_tcslen(value) < 1024) {
          some_data = true; // Success, we have a real entry
          _tcscpy(d_mode, value);
        }
      } else if (_tcscmp(key, TEXT("type")) == 0) {
        if (_tcslen(value) < 256)
          _tcscpy(d_type, value);
      } else if (_tcscmp(key, TEXT("data")) == 0) {
        if (_tcslen(value) < 256)
          _tcscpy(d_data, value);
      } else if (_tcscmp(key, TEXT("event")) == 0) {
        if (_tcslen(value) < 256) {
          _tcscpy(d_event, TEXT(""));
          _tcscpy(d_misc, TEXT(""));
          int ef;

          #if defined(__BORLANDC__)
          memset(d_event, 0, sizeof(d_event));
          memset(d_misc, 0, sizeof(d_event));
          if (_tcschr(value, ' ') == NULL) {
            _tcscpy(d_event, value);
          } else {
          #endif

          ef = _stscanf(value, TEXT("%[^ ] %[A-Za-z0-9 \\/().,]"), d_event,
              d_misc);

          #if defined(__BORLANDC__)
          }
          #endif

          // TODO code: Can't use token here - breaks
          // other token - damn C - how about
          // C++ String class ?

          // TCHAR *eventtoken;
          // eventtoken = _tcstok(value, TEXT(" "));
          // d_event = token;
          // eventtoken = _tcstok(value, TEXT(" "));

          if ((ef == 1) || (ef == 2)) {

            // TODO code: Consider reusing existing identical events

            pt2Event event = findEvent(d_event);
            if (event) {
              event_id = makeEvent(event, StringMallocParse(d_misc), event_id);

            #ifdef _INPUTDEBUG_
            } else if (input_errors_count < MAX_INPUT_ERRORS) {
              _stprintf(input_errors[input_errors_count++],
                  TEXT("Invalid event type: %s at %i"), d_event, line);
            #endif

            }

          #ifdef _INPUTDEBUG_
          } else if (input_errors_count < MAX_INPUT_ERRORS) {
            _stprintf(input_errors[input_errors_count++],
                TEXT("Invalid event type at %i"), line);
          #endif

          }
        }
      } else if (_tcscmp(key, TEXT("label")) == 0) {
        _tcscpy(d_label, value);
      } else if (_tcscmp(key, TEXT("location")) == 0) {
        _stscanf(value, TEXT("%d"), &d_location);

      #ifdef _INPUTDEBUG_
      } else if (input_errors_count < MAX_INPUT_ERRORS) {
        _stprintf(input_errors[input_errors_count++], TEXT("Invalid key/value pair %s=%s at %i"), key, value, line);
      #endif

      }
    }

  } // end while

  // file was ok, so save it to registry
  ContractLocalPath(szFile1);
  SetRegistryString(szRegistryInputFile, szFile1);

  fclose(fp);
}

#ifdef _INPUTDEBUG_
void
InputEvents::showErrors()
{
  TCHAR buffer[2048];
  int i;
  for (i = 0; i < input_errors_count; i++) {
    _stprintf(buffer, TEXT("%i of %i\r\n%s"), i + 1, input_errors_count, input_errors[i]);
    DoStatusMessage(TEXT("XCI Error"), buffer);
  }
  input_errors_count = 0;
}
#endif

int
InputEvents::findKey(const TCHAR *data)
{
  if (_tcscmp(data, TEXT("APP1")) == 0)
    return VK_APP1;
  else if (_tcscmp(data, TEXT("APP2")) == 0)
    return VK_APP2;
  else if (_tcscmp(data, TEXT("APP3")) == 0)
    return VK_APP3;
  else if (_tcscmp(data, TEXT("APP4")) == 0)
    return VK_APP4;
  else if (_tcscmp(data, TEXT("APP5")) == 0)
    return VK_APP5;
  else if (_tcscmp(data, TEXT("APP6")) == 0)
    return VK_APP6;

  else if (_tcscmp(data, TEXT("F1")) == 0)
    return VK_F1;
  else if (_tcscmp(data, TEXT("F2")) == 0)
    return VK_F2;
  else if (_tcscmp(data, TEXT("F3")) == 0)
    return VK_F3;
  else if (_tcscmp(data, TEXT("F4")) == 0)
    return VK_F4;
  else if (_tcscmp(data, TEXT("F5")) == 0)
    return VK_F5;
  else if (_tcscmp(data, TEXT("F6")) == 0)
    return VK_F6;
  else if (_tcscmp(data, TEXT("F7")) == 0)
    return VK_F7;
  else if (_tcscmp(data, TEXT("F8")) == 0)
    return VK_F8;
  else if (_tcscmp(data, TEXT("F9")) == 0)
    return VK_F9;
  else if (_tcscmp(data, TEXT("F10")) == 0)
    return VK_F10;
// VENTA-TEST HANDLING EXTRA HW KEYS ON HX4700 and HP31X
// else if (_tcscmp(data, TEXT("F11")) == 0)
//  return VK_F11;
// else if (_tcscmp(data, TEXT("F12")) == 0)
//    return VK_F12;

  else if (_tcscmp(data, TEXT("LEFT")) == 0)
    return VK_LEFT;
  else if (_tcscmp(data, TEXT("RIGHT")) == 0)
    return VK_RIGHT;
  else if (_tcscmp(data, TEXT("UP")) == 0)
    return VK_UP;
  else if (_tcscmp(data, TEXT("DOWN")) == 0)
    return VK_DOWN;

  else if (_tcscmp(data, TEXT("RETURN")) == 0)
    return VK_RETURN;
  else if (_tcscmp(data, TEXT("ESCAPE")) == 0)
    return VK_ESCAPE;

  else if (_tcslen(data) == 1)
    return _totupper(data[0]);

  else
    return 0;

}

pt2Event
InputEvents::findEvent(const TCHAR *data)
{
  int i;
  for (i = 0; i < Text2Event_count; i++) {
    if (_tcscmp(data, Text2Event[i].text) == 0)
      return Text2Event[i].event;
  }

  return NULL;
}

int
InputEvents::findGCE(const TCHAR *data)
{
  int i;
  for (i = 0; i < GCE_COUNT; i++) {
    if (_tcscmp(data, Text2GCE[i]) == 0)
      return i;
  }

  return -1;
}

int
InputEvents::findNE(const TCHAR *data)
{
  int i;
  for (i = 0; i < NE_COUNT; i++) {
    if (_tcscmp(data, Text2NE[i]) == 0)
      return i;
  }

  return -1;
}

// Create EVENT Entry
// NOTE: String must already be copied (allows us to use literals
// without taking up more data - but when loading from file must copy string
int
InputEvents::makeEvent(void (*event)(const TCHAR *), const TCHAR *misc, int next)
{
  if (Events_count >= MAX_EVENTS) {
    assert(0);
    return 0;
  }

  Events_count++;	// NOTE - Starts at 1 - 0 is a noop
  Events[Events_count].event = event;
  Events[Events_count].misc = misc;
  Events[Events_count].next = next;

  return Events_count;
}


// Make a new label (add to the end each time)
// NOTE: String must already be copied (allows us to use literals
// without taking up more data - but when loading from file must copy string
void
InputEvents::makeLabel(int mode_id, const TCHAR* label, int location, int event_id)
{

  // int i;

  /*
  // experimental, dont work because after loaded default strings are static, after loading
  //               from file some strings are static some not
  // add code for overwrite existing mode,location label
  for (i=0; i<ModeLabel_count[mode_id]; i++){
    if (ModeLabel[mode_id][i].location == location && ModeLabel[mode_id][i].event == event_id){
      if (ModeLabel[mode_id][i].label != NULL && ModeLabel[mode_id][i].label != label){
        TCHAR *pC;
        pC = ModeLabel[mode_id][i].label;
        free(ModeLabel[mode_id][i].label);
      }
      ModeLabel[mode_id][i].label = label;
      return;
    }
  }
  */

  if ((mode_id >= 0) && (mode_id < MAX_MODE) && (ModeLabel_count[mode_id] < MAX_LABEL)) {
    ModeLabel[mode_id][ModeLabel_count[mode_id]].label = label;
    ModeLabel[mode_id][ModeLabel_count[mode_id]].location = location;
    ModeLabel[mode_id][ModeLabel_count[mode_id]].event = event_id;
    ModeLabel_count[mode_id]++;
  } else {
    assert(0);
  }
}

// Return 0 for anything else - should probably return -1 !
InputEvents::mode
InputEvents::mode2int(const TCHAR *mode, bool create)
{
  int i = 0;

  // Better checks !
  if ((mode == NULL))
    return MODE_INVALID;

  for (i = 0; i < mode_map_count; i++) {
    if (_tcscmp(mode, mode_map[i]) == 0)
      return (InputEvents::mode)i;
  }

  if (create) {
    // Keep a copy
    _tcsncpy(mode_map[mode_map_count], mode, 25);
    return (InputEvents::mode)mode_map_count++;
  }

  // Should never reach this point
  assert(false);
  return MODE_INVALID;
}

void
InputEvents::setMode(mode mode)
{
  assert(mode < mode_map_count);

  if (mode == current_mode)
    return;

  current_mode = mode;

  // TODO code: Enable this in debug modes
  // for debugging at least, set mode indicator on screen
  /*
  if (thismode == 0) {
    ButtonLabel::SetLabelText(0, NULL);
  } else {
    ButtonLabel::SetLabelText(0, mode);
  }
  */
  ButtonLabel::SetLabelText(0, NULL);

  drawButtons(current_mode);
  /*
  // Set button labels
  int i;
  for (i = 0; i < ModeLabel_count[thismode]; i++) {
    // JMW removed requirement that label has to be non-null
    if (// (ModeLabel[thismode][i].label != NULL) &&
	(ModeLabel[thismode][i].location > 0)) {

      ButtonLabel::SetLabelText(
				ModeLabel[thismode][i].location,
				ModeLabel[thismode][i].label
				);
    }
  }
  */
}

void
InputEvents::setMode(const TCHAR *mode)
{
  InputEvents::mode thismode;

  assert(mode != NULL);

  // Mode must already exist to use it here...
  thismode = mode2int(mode, false);
  // Technically an error in config (eg event=Mode DoesNotExist)
  if (thismode == MODE_INVALID)
    // TODO enhancement: Add debugging here
    return;

  setMode(thismode);
}

void
InputEvents::drawButtons(mode Mode)
{
  int i;

  if (!globalRunningEvent.test())
    return;

  for (i = 0; i < ModeLabel_count[Mode]; i++) {
    if ((ModeLabel[Mode][i].location > 0)) {
      ButtonLabel::SetLabelText(ModeLabel[Mode][i].location,
          ModeLabel[Mode][i].label);
    }
  }
}

InputEvents::mode
InputEvents::getModeID()
{
  return current_mode;
}

// -----------------------------------------------------------------------
// Processing functions - which one to do
// -----------------------------------------------------------------------

// Input is a via the user touching the label on a touch screen / mouse
bool
InputEvents::processButton(int bindex)
{
  if (!globalRunningEvent.test())
    return false;

  mode thismode = getModeID();

  int i;
  // Note - reverse order - last one wins
  for (i = ModeLabel_count[thismode]; i >= 0; i--) {
    if ((ModeLabel[thismode][i].location == bindex)) {
      // && (ModeLabel[thismode][i].label != NULL)
      // JMW removed requirement of having a label!

      mode lastMode = thismode;

      /* JMW illegal, should be done by gui handler loop
      // JMW need a debounce method here..
      if (!Debounce()) return true;
      */

      if (ButtonLabel::hWndButtonWindow[bindex].is_enabled()) {
        ButtonLabel::AnimateButton(bindex);
        processGo(ModeLabel[thismode][i].event);
      }

      // experimental: update button text, macro may change the label
      if ((lastMode == getModeID()) && (ModeLabel[thismode][i].label != NULL)
          && (ButtonLabel::ButtonVisible[bindex])) {
        drawButtons(thismode);
      }

      return true;
    }
  }

  return false;
}

unsigned
InputEvents::key_to_event(mode mode, unsigned key_code)
{
  if (key_code >= MAX_KEY)
    return 0;

  unsigned event_id = Key2Event[mode][key_code];
  if (event_id == 0)
    /* not found in this mode - try the default binding */
    event_id = Key2Event[0][key_code];

  return event_id;
}

/*
  InputEvent::processKey(KeyID);
  Process keys normally brought in by hardware or keyboard presses
  Future will also allow for long and double click presses...
  Return = We had a valid key (even if nothing happens because of Bounce)
*/
bool
InputEvents::processKey(int dWord)
{
  if (!globalRunningEvent.test())
    return false;

  /* JMW illegal, should be done by gui handler loop
  InterfaceTimeoutReset();
  */

  // get current mode
  InputEvents::mode mode = InputEvents::getModeID();

  // Which key - can be defined locally or at default (fall back to default)
  int event_id = key_to_event(mode, dWord);

  if (event_id <= 0)
    return false;

  int bindex = -1;
  InputEvents::mode lastMode = mode;
  const TCHAR *pLabelText = NULL;

  // JMW should be done by gui handler
  // if (!Debounce()) return true;

  int i;
  for (i = ModeLabel_count[mode]; i >= 0; i--) {
    if ((ModeLabel[mode][i].event == event_id)) {
      bindex = ModeLabel[mode][i].location;
      pLabelText = ModeLabel[mode][i].label;
      if (bindex > 0) {
        ButtonLabel::AnimateButton(bindex);
      }
    }
  }

  if (bindex < 0 || ButtonLabel::hWndButtonWindow[bindex].is_enabled())
    InputEvents::processGo(event_id);

  // experimental: update button text, macro may change the value
  if ((lastMode == getModeID()) && (bindex > 0) && (pLabelText != NULL)
      && ButtonLabel::ButtonVisible[bindex]) {
    drawButtons(lastMode);
  }

  return true;
}

bool
InputEvents::processNmea(int ne_id)
{
  // add an event to the bottom of the queue
  mutexEventQueue.Lock();
  for (int i = 0; i < MAX_NMEA_QUEUE; i++) {
    if (NMEA_Queue[i] == -1) {
      NMEA_Queue[i] = ne_id;
      break;
    }
  }
  mutexEventQueue.Unlock();
  return true; // ok.
}

/*
  InputEvent::processNmea(TCHAR* data)
  Take hard coded inputs from NMEA processor.
  Return = TRUE if we have a valid key match
*/
bool
InputEvents::processNmea_real(int ne_id)
{
  if (!globalRunningEvent.test())
    return false;

  int event_id = 0;

  // JMW not required
  //  InterfaceTimeoutReset();

  // Valid input ?
  if ((ne_id < 0) || (ne_id >= NE_COUNT))
    return false;

  // get current mode
  InputEvents::mode mode = InputEvents::getModeID();

  // Which key - can be defined locally or at default (fall back to default)
  event_id = N2Event[mode][ne_id];
  if (event_id == 0) {
    // go with default key..
    event_id = N2Event[0][ne_id];
  }

  if (event_id > 0) {
    InputEvents::processGo(event_id);
    return true;
  }

  return false;
}


// This should be called ONLY by the GUI thread.
void
InputEvents::DoQueuedEvents(void)
{
  int GCE_Queue_copy[MAX_GCE_QUEUE];
  int NMEA_Queue_copy[MAX_NMEA_QUEUE];
  int i;

  // copy the queue first, blocking
  mutexEventQueue.Lock();
  for (i = 0; i < MAX_GCE_QUEUE; i++) {
    GCE_Queue_copy[i] = GCE_Queue[i];
  }
  for (i = 0; i < MAX_NMEA_QUEUE; i++) {
    NMEA_Queue_copy[i] = NMEA_Queue[i];
  }
  mutexEventQueue.Unlock();

  // process each item in the queue
  for (i = 0; i < MAX_GCE_QUEUE; i++) {
    if (GCE_Queue_copy[i] != -1) {
      processGlideComputer_real(GCE_Queue_copy[i]);
    }
  }
  for (i = 0; i < MAX_NMEA_QUEUE; i++) {
    if (NMEA_Queue_copy[i] != -1) {
      processNmea_real(NMEA_Queue_copy[i]);
    }
  }

  // now flush the queue, again blocking
  mutexEventQueue.Lock();
  for (i = 0; i < MAX_GCE_QUEUE; i++) {
    GCE_Queue[i] = -1;
  }
  for (i = 0; i < MAX_NMEA_QUEUE; i++) {
    NMEA_Queue[i] = -1;
  }
  mutexEventQueue.Unlock();
}

bool
InputEvents::processGlideComputer(int gce_id)
{
  // add an event to the bottom of the queue
  mutexEventQueue.Lock();
  for (int i = 0; i < MAX_GCE_QUEUE; i++) {
    if (GCE_Queue[i] == -1) {
      GCE_Queue[i] = gce_id;
      break;
    }
  }
  mutexEventQueue.Unlock();
  return true;
}

/*
  InputEvents::processGlideComputer
  Take virtual inputs from a Glide Computer to do special events
*/
bool
InputEvents::processGlideComputer_real(int gce_id)
{
  if (!globalRunningEvent.test())
    return false;
  int event_id = 0;

  // TODO feature: Log glide computer events to IGC file

  // Valid input ?
  if ((gce_id < 0) || (gce_id >= GCE_COUNT))
    return false;

  // get current mode
  InputEvents::mode mode = InputEvents::getModeID();

  // Which key - can be defined locally or at default (fall back to default)
  event_id = GC2Event[mode][gce_id];
  if (event_id == 0) {
    // go with default key..
    event_id = GC2Event[0][gce_id];
  }

  if (event_id > 0) {
    InputEvents::processGo(event_id);
    return true;
  }

  return false;
}

// EXECUTE an Event - lookup event handler and call back - no return
void
InputEvents::processGo(int eventid)
{
  if (!globalRunningEvent.test())
    return;

  // TODO feature: event/macro recorder
  /*
  if (LoggerActive) {
    LoggerNoteEvent(Events[eventid].);
  }
  */

  // evnentid 0 is special for "noop" - otherwise check event
  // exists (pointer to function)
  if (eventid) {
    if (Events[eventid].event) {
      Events[eventid].event(Events[eventid].misc);
      MenuTimeOut = 0;
    }
    if (Events[eventid].next > 0)
      InputEvents::processGo(Events[eventid].next);
  }
}

#include "SettingsUser.hpp"
#include "Screen/Blank.hpp"

int InputEvents::MenuTimeOut = 0;

void
InputEvents::HideMenu()
{
  MenuTimeOut = MenuTimeoutMax;
  ProcessMenuTimer();
  ResetDisplayTimeOut();
}

void
InputEvents::ResetMenuTimeOut()
{
  ResetDisplayTimeOut();
  MenuTimeOut = 0;
}

void
InputEvents::ShowMenu()
{
  #if !defined(GNAV) && !defined(PCGNAV)
  // Popup exit button if in .xci
  // setMode(TEXT("Exit"));
  setMode(MODE_MENU); // VENTA3
  #endif

  ResetDisplayTimeOut();
  MenuTimeOut = 0;
  ProcessMenuTimer();
}

#include "MapWindowProjection.hpp"
#include "InfoBoxManager.h"

void
InputEvents::ProcessMenuTimer()
{
  if (!InfoBoxManager::IsFocus()) {
    if (MenuTimeOut == MenuTimeoutMax) {
      if (SettingsMap().EnablePan && !SettingsMap().TargetPan) {
        setMode(MODE_PAN);
      } else {
        setMode(MODE_DEFAULT);
      }
    }

    MenuTimeOut++;
  }
}

void
InputEvents::ProcessTimer()
{
  if (globalRunningEvent.test()) {
    DoQueuedEvents();
  }
  ProcessMenuTimer();
}
