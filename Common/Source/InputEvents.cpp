

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

int TrailActive = TRUE;

// XXX header file ?
// What to do when a key is pressed... 
typedef struct {
  void (*pt2Func)(TCHAR *);	// Which function to call (can be any, but should be here)
  TCHAR *misc;			// What data to pass (eg: on, off, toggle)
} InputKeySTRUCT;


InputKeySTRUCT InputKeyData[99][255];	// 100 modes, 256 keys
			
// XXX Size constant etc
TCHAR mode_current[26] = TEXT("default");		// Current mode
TCHAR mode_map[99][26];					// Map mode to location
int mode_map_count = 0;

// -----------------------------------------------------------------------
// Initialisation and Defaults
// -----------------------------------------------------------------------

// Read the data files
void InputEvents::readFile() {
  // XXX Example only

  /*
    MODE !
    
    At read/init time, generate all buttons ?
    What about unusual modes - such as special menus
    Do we take a default set and inherit/copy them across on execute
    Do we do that dynamically, falling back ?
    Can each mode have a sub-set of keys, copying the previous
    advantage - only define what you change
    disadvantage - undefined keys, what ever they last were set to - weird
    Alternative - one default set, mode on top of default only
    If we do that, then check current mode then default entry version only
    Because special modes are TEXT, how do we define that... Do we use a HASH, or lookup?
    or combination - array lookup of entry for mode...
    
    XXX Example of looking up mode, finding key, fall back to default etc.
    
  */

  // JMW all of these define the default behaviour, which is how XCSoar currently works.
  // 
  // default/map mode

  InputKeyData[mode2int(TEXT("default"), true)][VK_APP1].pt2Func = &eventMainMenu;	
  InputKeyData[mode2int(TEXT("default"), true)][VK_APP1].misc = TEXT("");	

  InputKeyData[mode2int(TEXT("default"), true)][VK_APP2].pt2Func = &eventMarkLocation;
  InputKeyData[mode2int(TEXT("default"), true)][VK_APP2].misc = TEXT("");
  
  InputKeyData[mode2int(TEXT("default"), true)][VK_APP3].pt2Func = &eventSelectInfoBox;
  InputKeyData[mode2int(TEXT("default"), true)][VK_APP3].misc = TEXT("next");
  
  InputKeyData[mode2int(TEXT("default"), true)][VK_APP4].pt2Func = &setMode;
  InputKeyData[mode2int(TEXT("default"), true)][VK_APP4].misc = TEXT("display1");

  InputKeyData[mode2int(TEXT("default"), true)][VK_DOWN].pt2Func = &eventScaleZoom;
  InputKeyData[mode2int(TEXT("default"), true)][VK_DOWN].misc = TEXT("-");

  InputKeyData[mode2int(TEXT("default"), true)][VK_UP].pt2Func = &eventScaleZoom;
  InputKeyData[mode2int(TEXT("default"), true)][VK_UP].misc = TEXT("+");

  /*
  InputKeyData[mode2int(TEXT("default"), true)][VK_LEFT].pt2Func = &eventAutoZoom;
  InputKeyData[mode2int(TEXT("default"), true)][VK_LEFT].misc = TEXT("toggle");

  InputKeyData[mode2int(TEXT("default"), true)][VK_RIGHT].pt2Func = &eventPan;
  InputKeyData[mode2int(TEXT("default"), true)][VK_RIGHT].misc = TEXT("toggle");
  */

  // infobox mode

  InputKeyData[mode2int(TEXT("infobox"), true)][VK_APP1].pt2Func = &eventSelectInfoBox;
  InputKeyData[mode2int(TEXT("infobox"), true)][VK_APP1].misc = TEXT("previous");

  InputKeyData[mode2int(TEXT("infobox"), true)][VK_APP4].pt2Func = &eventSelectInfoBox;
  InputKeyData[mode2int(TEXT("infobox"), true)][VK_APP4].misc = TEXT("next");

  InputKeyData[mode2int(TEXT("infobox"), true)][VK_APP2].pt2Func = &eventChangeInfoBoxType;
  InputKeyData[mode2int(TEXT("infobox"), true)][VK_APP2].misc = TEXT("previous");

  InputKeyData[mode2int(TEXT("infobox"), true)][VK_APP3].pt2Func = &eventChangeInfoBoxType;
  InputKeyData[mode2int(TEXT("infobox"), true)][VK_APP3].misc = TEXT("next");

  InputKeyData[mode2int(TEXT("infobox"), true)][VK_UP].pt2Func = &eventDoInfoKey;
  InputKeyData[mode2int(TEXT("infobox"), true)][VK_UP].misc = TEXT("up");
  
  InputKeyData[mode2int(TEXT("infobox"), true)][VK_DOWN].pt2Func = &eventDoInfoKey;
  InputKeyData[mode2int(TEXT("infobox"), true)][VK_DOWN].misc = TEXT("down");

  InputKeyData[mode2int(TEXT("infobox"), true)][VK_LEFT].pt2Func = &eventDoInfoKey;
  InputKeyData[mode2int(TEXT("infobox"), true)][VK_LEFT].misc = TEXT("left");

  InputKeyData[mode2int(TEXT("infobox"), true)][VK_RIGHT].pt2Func = &eventDoInfoKey;
  InputKeyData[mode2int(TEXT("infobox"), true)][VK_RIGHT].misc = TEXT("right");

  InputKeyData[mode2int(TEXT("infobox"), true)][VK_RETURN].pt2Func = &eventDoInfoKey;
  InputKeyData[mode2int(TEXT("infobox"), true)][VK_RETURN].misc = TEXT("return");

  ////////

  InputKeyData[mode2int(TEXT("display1"), true)][VK_APP1].pt2Func = &eventClearWarningsAndTerrain;	
  InputKeyData[mode2int(TEXT("display1"), true)][VK_APP1].misc = TEXT("");	

  InputKeyData[mode2int(TEXT("display1"), true)][VK_APP2].pt2Func = &eventScreenModes;
  InputKeyData[mode2int(TEXT("display1"), true)][VK_APP2].misc = TEXT("toggle");
  
  InputKeyData[mode2int(TEXT("display1"), true)][VK_APP3].pt2Func = &eventPan;
  InputKeyData[mode2int(TEXT("display1"), true)][VK_APP3].misc = TEXT("toggle");

  InputKeyData[mode2int(TEXT("display1"), true)][VK_APP4].pt2Func = &setMode;
  InputKeyData[mode2int(TEXT("display1"), true)][VK_APP4].misc = TEXT("display2");

  InputKeyData[mode2int(TEXT("display1"), true)][VK_UP].pt2Func = &eventPanCursor;
  InputKeyData[mode2int(TEXT("display1"), true)][VK_UP].misc = TEXT("up");
  
  InputKeyData[mode2int(TEXT("display1"), true)][VK_DOWN].pt2Func = &eventPanCursor;
  InputKeyData[mode2int(TEXT("display1"), true)][VK_DOWN].misc = TEXT("down");

  InputKeyData[mode2int(TEXT("display1"), true)][VK_LEFT].pt2Func = &eventPanCursor;
  InputKeyData[mode2int(TEXT("display1"), true)][VK_LEFT].misc = TEXT("left");

  InputKeyData[mode2int(TEXT("display1"), true)][VK_RIGHT].pt2Func = &eventPanCursor;
  InputKeyData[mode2int(TEXT("display1"), true)][VK_RIGHT].misc = TEXT("right");

  /////////

  InputKeyData[mode2int(TEXT("display2"), true)][VK_APP1].pt2Func = &eventAutoZoom;
  InputKeyData[mode2int(TEXT("display2"), true)][VK_APP1].misc = TEXT("toggle");	

  InputKeyData[mode2int(TEXT("display2"), true)][VK_APP2].pt2Func = &eventSnailTrail;
  InputKeyData[mode2int(TEXT("display2"), true)][VK_APP2].misc = TEXT("toggle");

  InputKeyData[mode2int(TEXT("display2"), true)][VK_APP3].pt2Func = &eventSounds;
  InputKeyData[mode2int(TEXT("display2"), true)][VK_APP3].misc = TEXT("toggle");
    
  InputKeyData[mode2int(TEXT("display2"), true)][VK_APP4].pt2Func = &setMode;
  InputKeyData[mode2int(TEXT("display2"), true)][VK_APP4].misc = TEXT("default");

  InputKeyData[mode2int(TEXT("display2"), true)][VK_UP].pt2Func = &eventPanCursor;
  InputKeyData[mode2int(TEXT("display2"), true)][VK_UP].misc = TEXT("up");
  
  InputKeyData[mode2int(TEXT("display2"), true)][VK_DOWN].pt2Func = &eventPanCursor;
  InputKeyData[mode2int(TEXT("display2"), true)][VK_DOWN].misc = TEXT("down");

  InputKeyData[mode2int(TEXT("display2"), true)][VK_LEFT].pt2Func = &eventPanCursor;
  InputKeyData[mode2int(TEXT("display2"), true)][VK_LEFT].misc = TEXT("left");

  InputKeyData[mode2int(TEXT("display2"), true)][VK_RIGHT].pt2Func = &eventPanCursor;
  InputKeyData[mode2int(TEXT("display2"), true)][VK_RIGHT].misc = TEXT("right");



  /* 
     if (VK_APP6 < 256) {
     InputKeyData[VK_APP4].pt2Func = &InputEvents::eventShowMenu;
     InputKeyData[VK_APP4].misc = TEXT("");
     }
  */

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

  wcsncpy(mode_current, mode, 25);

  thismode = mode2int(mode,true);
  if (thismode == lastmode) return;
  lastmode = thismode;

  // XXX TODO: set button labels for each mode (we don't have a way of specifying
  // these mappings yet, so I'm hardcoding them...

  // for debugging at least, set mode indicator on screen
  if (thismode==0) {
    ButtonLabel::SetLabelText(0,NULL);
    ButtonLabel::SetLabelText(1,NULL);
    ButtonLabel::SetLabelText(2,NULL);
    ButtonLabel::SetLabelText(3,NULL);
    ButtonLabel::SetLabelText(4,NULL);
  } else {
    ButtonLabel::SetLabelText(0,mode);
  }
  if (thismode==1) { // XXX infobox mode, naughty I am hardcoding it..
    ButtonLabel::SetLabelText(1,TEXT("<<"));
    ButtonLabel::SetLabelText(4,TEXT(">>"));
    ButtonLabel::SetLabelText(2,TEXT("<type"));
    ButtonLabel::SetLabelText(3,TEXT("type>"));   
  }
  if (thismode==2) {
    ButtonLabel::SetLabelText(1,TEXT("Terrain"));
    ButtonLabel::SetLabelText(2,TEXT("Layout"));
    ButtonLabel::SetLabelText(3,TEXT("Pan"));   
    ButtonLabel::SetLabelText(4,TEXT(".."));
  }
  if (thismode==3) {
    ButtonLabel::SetLabelText(1,TEXT("AZoom"));
    ButtonLabel::SetLabelText(2,TEXT("Snail"));
    ButtonLabel::SetLabelText(3,TEXT("Audio"));   
    ButtonLabel::SetLabelText(4,TEXT(".."));
  }
}


TCHAR* InputEvents::getMode() {
  return mode_current;
}


// -----------------------------------------------------------------------
// Processing functions - which one to do
// -----------------------------------------------------------------------

/*
  InputEvent::processKey(KeyID);
	Process keys normally brought in by hardware or keyboard presses
	Future will also allow for long and double click presses...
	Return = We had a valid key (even if nothing happens because of Bounce)
*/
bool InputEvents::processKey(int dWord) {

  // JMW changed this to pointer for efficiency and because C++ doesn't do deep
  // copy e.g. of text strings!
  InputKeySTRUCT *current;

  // get current mode
  int mode = mode2int(InputEvents::getMode(), false);
  
  if ((dWord < 0) || (dWord > 255))
    return false;
  
  current = &(InputKeyData[mode][dWord]);
  if (current->pt2Func == NULL) {
    // go with default key..
    current = &(InputKeyData[0][dWord]);
    return false; // JMW if no mapping defined, should be blank and return false
  }
  
  if (current->pt2Func != NULL) {
    if (!Debounce()) return true;
    current->pt2Func(current->misc);
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



// -----------------------------------------------------------------------
// Execution - list of things you can do
// -----------------------------------------------------------------------

// XXX Keep marker text for log file etc.
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

// XXX This should be just SnailTrail - Turn on/off with string
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
