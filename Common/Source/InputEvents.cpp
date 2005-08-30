/*

InputEvent

	This class handles all input events. Basic features include:

	* Defaults
	* Read configuration file (parse and check data)
	* Process input types (eg: keystrokes)
	* Execute events (eg: Turn on AutoZoom)


Configuration File Format and Data

	* Text File
	* CrLf combinations (DOS standard) for new lines
	* Each record is terminated by a blank line

	Eg:

		type=hardware
		data=APP1
		function=activatemode
		misc=menu1
		mode=all

		type=hardware
		data=APP7
		function=fullscreen
		misc=toggle
		mode=menu1
		label=FullSc
		location=1,4,3

		type=hardware
		data=APP8
		function=fullscreen
		misc=toggle
		mode=menu1
		label=FullSc
		location=1,4,3

		type=hardware
		data=APP9
		function=fullscreen
		misc=toggle
		mode=menu1
		label=FullSc
		location=1,4,3

		type=keyboard
		data=m
		function=marker
		misc=Some optional text to drop with marker
		mode=all

		type=hardware
		data=APP1
		function=zoom
		misc=0.25
		mode=thermal


Modes

	XCSoar now has the concept of Modes. These are an arbitrary
	string that associates with where and what XCS is doing.

	XXX guiMode !

	? Do they need to consider flight mode?

	Modes:

		all		A pseudo mode matching all (or any) mode

		mainwindow / map ???
		(
			thermal		Thermallingâ circling, in lift

			cruise		Standard cruise mode - lookout for gliders
					and lift

			finalglide	Final glide - here comes the field
		)

		menubutton	Strange name maybe, but this allows you
				to define what buttons do when the menu button
				is active. This will be particularly useful
				for devices that have no screen (eg: Smartphone)

		menu		Like menubutton, only now we have the menu
				window being displayed.

				mode = map
				key=b1
				function= activate_mode
				misc= mm1

		warning		Airspace warning currently being displayed

		input		Input a value - eg: Bugs/Ballast/Wind.
				Not sure about this one yet - may require more
				thinking, or separation into more than one.
				Nice thing here is that there can be a standard
				set of input buttons, jump buttons, up, down etc
				that are set in this mode.

		infobox gui mode

				- up value
				- down values
				- enter (eg: on/off auto mccready)
				- cycle up what is displayed at that location
				- cycle down what is displayed at that location
				- navigate left between info boxes
				- navigate right between info boxes

					mode=infobox
					key=left
					function=infobox_navigate
					misc=next | previous | first | last | number


		"Dialogs"

			dialog_bugs
			dialog_standard


	Mode precedence has been tricky, so instead of solving the problem
	it is being worked around. XCS will choose to set a global variable
	to specify what mode it thinks it is in. This can then be used by the
	input code to decide what to do. This mode could get out of sink
	with the real world, and careful checking will be required, but at
	this stage it seems like the only sensible option.

	The code will review first if an entry exists in the current mode, and
	then in the all mode. This allows you to do one of the following
	example: Define a default action for button "A" to be "Zoom In" but
	make that button increase Bugs value in Input mode only. You can do
	this by making an ALL and a INPUT entry. You can also put an entry
	in for Button "A" for every mode and have complete control.

Special Modes - eg: the level of a menu (Think File vs Edit, vs Tools vs Help)

	Theoretically we may even be able to have special modes, such as
	the level of the menu you are at. You press one button, then another
	set become available (like pressing menu and seeing Settings etc). This
	will be very useful in non-touch screen models. The menu configuration
	can then be read from this same file and configured, allowing any
	number of levels and any number of combinations.

	The only hard part is what mode to go back to. We need a
	"Calculate Live Mode" function - which can be called to calculate the
	real live mode (eg: finalglide vs curse) rather than the temporary
	mode such as Menuâ Special Menu Level, Warning etc.

	The label and location values are examples of what can be done here
	to allow input button labels to be displayed. What needs to be
	considered is a simple way of mapping the locations and the size.
	In some models it may be that buttons are 4 across the top of the
	screen, where as others it is 3 or 2 or even 6. So both size and
	location needs to be considered.

	The label itself will go through gettext to allow language
	translations.

Input Types

	Types:

		hardware	These are the standard hardware buttons
				on normal organisers. Usually these are
				APP1..6.

		keyboard	Normal characters on the keyboard (a-z etc)

		nmea		A sentence received via NMEA stream (either)

		virtual		Virtual buttons are a new ideaâ allowing
				multiple buttons to be created on screen.
				These buttons can then be optionally mapped
				to physical buttons or to a spot on the
				screen (probably transparent buttons over
				the map).

Modifiers

	It is a long term goal of this project to allow modifiers for keys.
	This could include one of the following possibilities:

		* Combination presses (although not supported on many devices)

		* Double Click

		* Long Click

	Modifiers such as the above will not be supported in the first release.

Functions/Events - what it does

	AutoZoom		on, off, toggle
	FullScreen		on, off, toggle
	SnailTrail 		on, off, long, toggle
	VarioSound 		on, off
	Marker 			optional text to add
	MenuButton 		on, off, toggle
	Menu			open, close, toggle
	MenuEntry		task, b+b, abortresume, abore, resume, pressure
				logger, settings, status, analysis, exit, cancel
				NOTE: Some of the above may be separate functions
	Settings		(each setting, bring up to that point)
	Bugs			add, subtract, 0-100% (set value)
	Ballast			add, subtract, 0-100% (set value)
	Zoom			add, subtract, 0-nn (set value)
	Wind			up, down, 0-nn (set value, left, right, "n","ne","e","se","s","sw","w","nw"...
	McCready		add, subtract, 0-nn (set value)
	WaypointNext		"String" to specific waypoint
				eg: WayPointNext "home"
	WayPoint???		"reverse" - reverse, from last passed back to start (ie: from here to home)
				"drop next" - drop the next
				"restore" - restore all - from start of flight but
				XXX This needs more thought
	flight 			"startstop", "start", "stop", "release"
				Start/Stop of flight - Can be automatic, but pressing will override
				automatic part.
        release 		markse the point of release from tow

*/


#include "stdafx.h"
#include "XCSoar.h"
#include "InputEvents.h"
#include "Utils.h"
#include "VarioSound.h"
#include "Terrain.h"
#include "compatibility.h"
#include <commctrl.h>
#include <aygshell.h>
#include "InfoBoxLayout.h"
#include "Airspace.h"

// XXX What is this for?
int TrailActive = TRUE;

#define MAX_MODE 100
#define MAX_MODE_STRING 25
#define MAX_KEY 255
#define MAX_EVENTS 1024
#define MAX_LABEL NUMBUTTONLABELS

// Current modes - map mode to integer (primitive hash)
TCHAR mode_current[MAX_MODE_STRING] = TEXT("default");		// Current mode
TCHAR mode_map[MAX_MODE][MAX_MODE_STRING];					// Map mode to location
int mode_map_count = 0;

// Key map to Event - Keys (per mode) mapped to events
int Key2Event[MAX_MODE][MAX_KEY];		// Points to Events location

// Events - What do you want to DO
typedef struct {
	void (*event)(TCHAR *);		// Which function to call (can be any, but should be here)
	TCHAR *misc;				// Parameters
} EventSTRUCT;
EventSTRUCT Events[MAX_EVENTS];
int Events_count;				// How many have we defined

// Labels - defined per mode
typedef struct {
	TCHAR *label;
	int location;
	int event;
} ModeLabelSTRUCT;
ModeLabelSTRUCT ModeLabel[MAX_MODE][MAX_LABEL];
int ModeLabel_count[MAX_MODE];				// Where are we up to in this mode...

// -----------------------------------------------------------------------
// Initialisation and Defaults
// -----------------------------------------------------------------------

// Read the data files
void InputEvents::readFile() {
	// Get defaults
	#include "InputEvents_keys.h"

	// TODO - Read in user defined configuration file
}

// Create EVENT Entry
// NOTE: String must already be copied (allows us to use literals
// without taking up more data - but when loading from file must copy string
int InputEvents::makeEvent(void (*event)(TCHAR *), TCHAR *misc) {
	if (Events_count >= MAX_EVENTS)
		return 0;
	Events_count++;	// NOTE - Starts at 1 - 0 is a noop
	Events[Events_count].event = event;
	Events[Events_count].misc = misc;
	return Events_count;
}


// Make a new label (add to the end each time)
// NOTE: String must already be copied (allows us to use literals
// without taking up more data - but when loading from file must copy string
void InputEvents::makeLabel(int mode_id, TCHAR* label, int location, int event_id) {
	if ((mode_id >= 0) && (mode_id < MAX_MODE) && (ModeLabel_count[mode_id] < MAX_LABEL)) {
		ModeLabel[mode_id][ModeLabel_count[mode_id]].label = label;
		ModeLabel[mode_id][ModeLabel_count[mode_id]].location = location;
		ModeLabel[mode_id][ModeLabel_count[mode_id]].event = event_id;
		ModeLabel_count[mode_id]++;
	}
}

// Return 0 for anything else - should probably return -1 !
int InputEvents::mode2int(TCHAR *mode, bool create) {
  int i = 0;

  // Better checks !
  if ((mode == NULL))
    return 0;

  for (i = 0; i < mode_map_count; i++) {
    if (wcscmp(mode, mode_map[i]) == 0)
      return i;
  }

  if (create) {
    // Keep a copy
    wcsncpy(mode_map[mode_map_count], mode, 25);
    mode_map_count++;
    return mode_map_count - 1;
  }

  return 0;
}


void InputEvents::setMode(TCHAR *mode) {
  static int lastmode = -1;
  int thismode;

  wcsncpy(mode_current, mode, MAX_MODE_STRING);

  thismode = mode2int(mode,true);
  if (thismode == lastmode) return;
  lastmode = thismode;

  // for debugging at least, set mode indicator on screen
  if (thismode==0) {
    ButtonLabel::SetLabelText(0,NULL);
  } else {
    ButtonLabel::SetLabelText(0,mode);
  }

  // Set button labels
  int i;
  for (i = 0; i < ModeLabel_count[thismode]; i++) {
    if ((ModeLabel[thismode][i].label != NULL) && (ModeLabel[thismode][i].location > 0))
      ButtonLabel::SetLabelText(
				ModeLabel[thismode][i].location,
				ModeLabel[thismode][i].label
				);
  }

}


TCHAR* InputEvents::getMode() {
  return mode_current;
}

int InputEvents::getModeID() {
	return InputEvents::mode2int(InputEvents::getMode(), false);
}

// -----------------------------------------------------------------------
// Processing functions - which one to do
// -----------------------------------------------------------------------

bool InputEvents::processButton(int bindex) {
    // XXX  DoStatusMessage(TEXT("Pressed a button"));
	int thismode = getModeID();
	int i;
	for (i = 0; i < ModeLabel_count[thismode]; i++) {
		if ((ModeLabel[thismode][i].location == bindex) && (ModeLabel[thismode][i].label != NULL)) {
			eventGo(ModeLabel[thismode][i].event);
			return true;
		}
  }

	return false;
}

/*
  InputEvent::processKey(KeyID);
	Process keys normally brought in by hardware or keyboard presses
	Future will also allow for long and double click presses...
	Return = We had a valid key (even if nothing happens because of Bounce)
*/
bool InputEvents::processKey(int dWord) {
	int event_id;

	// Valid input ?
	if ((dWord < 0) || (dWord > MAX_KEY))
		return false;

	// get current mode
	int mode = InputEvents::getModeID();

	// Which key - can be defined locally or at default (fall back to default)
	event_id = Key2Event[mode][dWord];
	if (event_id == 0) {
		// go with default key..
		event_id = Key2Event[0][dWord];
	}

	if (event_id > 0) {
		if (!Debounce()) return true;
		InputEvents::eventGo(event_id);
		return true;
	}

	return false;
}

/*
  InputEvent::processNmea(TCHAR* data)
  Process a string match for NMEA data and call function
 Return = TRUE if we have a valid key match
*/
bool InputEvents::processNmea(TCHAR* data) {
  return true;
}


// EXECUTE an Event - lookup event handler and call back - no return
void InputEvents::eventGo(int eventid) {
	// evnentid 0 is special for "noop" - otherwise check event
	// exists (pointer to function)
	if (eventid && Events[eventid].event)
		Events[eventid].event(Events[eventid].misc);
}

// -----------------------------------------------------------------------
// Execution - list of things you can do
// -----------------------------------------------------------------------

// TODO Keep marker text for log file etc.
void InputEvents::eventMarkLocation(TCHAR *misc) {
  // ARH Let the user know what's happened
  DoStatusMessage(TEXT("Dropped marker"));

  LockFlightData();
  MarkLocation(GPS_INFO.Longditude, GPS_INFO.Lattitude);
  UnlockFlightData();
}


// Vario sounds only XXX change eventVarioSounds
void InputEvents::eventSounds(TCHAR *misc) {
	int OldEnableSoundVario = EnableSoundVario;

	// XXX Consider single byte chars not wide
	if (wcscmp(misc, TEXT("toggle")) == 0)
		EnableSoundVario = !EnableSoundVario;
	else if (wcscmp(misc, TEXT("on")) == 0)
		EnableSoundVario = 1;
	if (wcscmp(misc, TEXT("off")) == 0)
		EnableSoundVario = 0;

  	// ARH Let the user know what's happened
	if (EnableSoundVario != OldEnableSoundVario) {
		VarioSound_EnableSound((BOOL)EnableSoundVario);
		if (EnableSoundVario)
			DoStatusMessage(TEXT("Vario Sounds ON"));
		else
			DoStatusMessage(TEXT("Vario Sounds OFF"));
	}
}

void InputEvents::eventSnailTrail(TCHAR *misc) {
  int OldTrailActive;
  OldTrailActive = TrailActive;

  if (wcscmp(misc, TEXT("toggle")) == 0) {
    TrailActive ++;
    if (TrailActive>2) {
      TrailActive=0;
    }
  }
  else if (wcscmp(misc, TEXT("off")) == 0)
    TrailActive = 0;
  else if (wcscmp(misc, TEXT("long")) == 0)
    TrailActive = 1;
  else if (wcscmp(misc, TEXT("short")) == 0)
    TrailActive = 2;

  if (OldTrailActive != TrailActive) {
    if (TrailActive==0)
      DoStatusMessage(TEXT("SnailTrail OFF"));
    if (TrailActive==1)
      DoStatusMessage(TEXT("SnailTrail ON Long"));
    if (TrailActive==2)
      DoStatusMessage(TEXT("SnailTrail ON Short"));
  }
}

// XXX This should be just ScreenModes - toggle etc with string
void InputEvents::eventScreenModes(TCHAR *misc) {
  // toggle switches like this:
  //  -- normal infobox
  //  -- auxiliary infobox
  //  -- full screen
  //  -- normal infobox

  if (EnableAuxiliaryInfo) {
    MapWindow::RequestToggleFullScreen();
    EnableAuxiliaryInfo = false;
  } else {
    if (MapWindow::IsMapFullScreen()) {
      MapWindow::RequestToggleFullScreen();
    } else {
      EnableAuxiliaryInfo = true;
    }
  }
}




// eventAutoZoom - Turn on|off|toggle AutoZoom
// misc:
//	on - Turn on if not already
//	off - Turn off if not already
//	toggle - Toggle current full screen status
void InputEvents::eventAutoZoom(TCHAR* misc) {
  // JMW pass through to handler in MapWindow
  // here:
  // -1 means toggle
  // 0 means off
  // 1 means on

  if (wcscmp(misc, TEXT("toggle")) == 0) {
    MapWindow::Event_AutoZoom(-1);
    return;
  }
  else if (wcscmp(misc, TEXT("on")) == 0) {
    MapWindow::Event_AutoZoom(1);
    return;
  }
  else if (wcscmp(misc, TEXT("off")) == 0) {
    MapWindow::Event_AutoZoom(0);
    return;
  }
}


/*
	XXX  Entries todo (see above documentation)

		SnailTrail (on, off, long, toggle)
		VarioSound (on, off)
		Marker (optional text to add)
		MenuButton (on, off, toggle)
		Menu(open, close, toggle)

*/

/*
MapWindow::Event_SetZoom(double value);
MapWindow::Event_ScaleZoom(int vswitch);
MapWindow::Event_Pan(int vswitch);
MapWindow::Event_Terrain(int vswitch);
MapWindow::Event_AutoZoom(int vswitch);
*/

void InputEvents::eventScaleZoom(TCHAR *misc) {
  if (wcscmp(misc, TEXT("-")) == 0) {
    MapWindow::Event_ScaleZoom(-1);
    return;
  }
  if (wcscmp(misc, TEXT("+")) == 0) {
    MapWindow::Event_ScaleZoom(1);
    return;
  }
  if (wcscmp(misc, TEXT("--")) == 0) {
    MapWindow::Event_ScaleZoom(-2);
    return;
  }
  if (wcscmp(misc, TEXT("++")) == 0) {
    MapWindow::Event_ScaleZoom(2);
    return;
  }
}

void InputEvents::eventPan(TCHAR *misc) {
  if (wcscmp(misc, TEXT("toggle")) == 0) {
    MapWindow::Event_Pan(-1);
    return;
  }
  else if (wcscmp(misc, TEXT("on")) == 0) {
    MapWindow::Event_Pan(1);
    return;
  }
  else if (wcscmp(misc, TEXT("off")) == 0) {
    MapWindow::Event_Pan(0);
    return;
  }
}


void InputEvents::eventClearWarningsAndTerrain(TCHAR *misc) {
  // dumb at the moment, just to get legacy functionality
  if (ClearAirspaceWarnings(true)) {
    // airspace was active, enter was used to acknowledge
    return;
  }
  MapWindow::Event_Terrain(-1);
}


void InputEvents::eventSelectInfoBox(TCHAR *misc) {
  if (wcscmp(misc, TEXT("next")) == 0) {
    Event_SelectInfoBox(1);
  }
  if (wcscmp(misc, TEXT("previous")) == 0) {
    Event_SelectInfoBox(-1);
  }
}


void InputEvents::eventChangeInfoBoxType(TCHAR *misc) {
  if (wcscmp(misc, TEXT("next")) == 0) {
    Event_ChangeInfoBoxType(1);
  }
  if (wcscmp(misc, TEXT("previous")) == 0) {
    Event_ChangeInfoBoxType(-1);
  }
}


void InputEvents::eventDoInfoKey(TCHAR *misc) {
  if (wcscmp(misc, TEXT("up")) == 0) {
    DoInfoKey(1);
  }
  if (wcscmp(misc, TEXT("down")) == 0) {
    DoInfoKey(-1);
  }
  if (wcscmp(misc, TEXT("left")) == 0) {
    DoInfoKey(-2);
  }
  if (wcscmp(misc, TEXT("right")) == 0) {
    DoInfoKey(2);
  }
  if (wcscmp(misc, TEXT("return")) == 0) {
    DoInfoKey(0);
  }

}


void InputEvents::eventPanCursor(TCHAR *misc) {
  if (wcscmp(misc, TEXT("up")) == 0) {
    MapWindow::Event_PanCursor(0,1);
  }
  if (wcscmp(misc, TEXT("down")) == 0) {
    MapWindow::Event_PanCursor(0,-1);
  }
  if (wcscmp(misc, TEXT("left")) == 0) {
    MapWindow::Event_PanCursor(1,0);
  }
  if (wcscmp(misc, TEXT("right")) == 0) {
    MapWindow::Event_PanCursor(-1,0);
  }
}


void InputEvents::eventMainMenu(TCHAR *misc) {
  // todo: popup main menu
}

void InputEvents::eventMode(TCHAR *misc) {
	InputEvents::setMode(misc);
}
