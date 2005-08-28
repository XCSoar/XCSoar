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
		function=fullscreen
		misc=toggle
		mode=all
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

	Modes:

		all		A pseudo mode matching all (or any) mode

		thermal		Thermallingâ circling, in lift

		cruise		Standard cruise mode - lookout for gliders
				and lift

		finalglide	Final glide - here comes the field

		menubutton	Strange name maybe, but this allows you
				to define what buttons do when the menu button
				is active. This will be particularly useful
				for devices that have no screen (eg: Smartphone)

		menu		Like menubutton, only now we have the menu
				window being displayed.

		warning		Airspace warning currently being displayed

		input		Input a value - eg: Bugs/Ballast/Wind.
				Not sure about this one yet - may require more
				thinking, or separation into more than one.
				Nice thing here is that there can be a standard
				set of input buttons, jump buttons, up, down etc
				that are set in this mode.



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

#include "InputEvent.h"
#include "externs.h"
#include "Utils.h"

// What to do when a key is pressed...
typedef struct {
	void (*pt2Func)(TCHAR *);	// Which function to call (can be any, but should be here)
	TCHAR *misc;			// What data to pass (eg: on, off, toggle)
} InputKeySTRUCT;

typedef char TCHAR;

class InputEvent {
public:
	InputKeySTRUCT InputKeyData[255];

	// -----------------------------------------------------------------------
	// Initialisation and Defaults
	// -----------------------------------------------------------------------

	// Read the data files
	void readFile() {
		// XXX Example only
		// VK_APP1 - hardware key
		InputEvent[VK_APP1].event = &eventFullScreen;	// Which function
		InputEvent[VK_APP1].misc = TEXT("toggle");	// Data to send
	}

	// -----------------------------------------------------------------------
	// Processing functions - which one to do
	// -----------------------------------------------------------------------

	// InputEvent::processKey(KeyID);
	//	Process keys normally brought in by hardware or keyboard presses
	//	Futureâ will also allow for long and double click presses...
	// Return = We had a valid key (even if nothing happens because of Bounce)
	bool processKey(int dWord) {
		if (InputKeyData[dWord] && InputKeyData[dWord].event) {
			if (!Debounce()) return true;
			InputKeyData[dWord].pt2Func(InputKeyData[dWord].misc);
			return true;
		}

		return false;
	}

	// InputEvent::processNmea(TCHAR* data)
	//	Process a string match for NMEA data and call function
	// Return = TRUE if we have a valid key match
	bool processNmea(TCHAR* data) {
	}

	// -----------------------------------------------------------------------
	// Execution - list of things you can do
	// -----------------------------------------------------------------------
	void eventAutoZoom(TCHAR* misc) {
	}


	// eventFullScreen - Turn on|off|toggle full screen
	// misc:
	//	on - Turn on if not already
	//	off - Turn off if not already
	//	toggle - Toggle current full screen status
	void eventFullScreen(TCHAR* misc) {
		if (wcscmp(misc, TEXT("toggle")) == 0)
			MapWindow::RequestToggleFullScreen();
		else if (wcscmp(misc, TEXT("on")) == 0)
			MapWindow::RequestOnFullScreen();
		else if (wcscmp(misc, TEXT("off")) == 0)
			MapWindow::RequestOffFullScreen();
	}

	// eventAutoZoom - Turn on|off|toggle AutoZoom
	// misc:
	//	on - Turn on if not already
	//	off - Turn off if not already
	//	toggle - Toggle current full screen status
	void eventAutoZoom(TCHAR* misc) {
		MapWindow::RequestToggleFullScreen();
		if (wcscmp(misc, TEXT("toggle")) == 0)
			// XXX Could pass in -1, 0, 1 to AutoZoom -
			// or separate functions or something else ...
			MapWindow::Event_AutoZoom();
		else if (wcscmp(misc, TEXT("on")) == 0)
			MapWindow::Event_AutoZoom();
		else if (wcscmp(misc, TEXT("off")) == 0)
			MapWindow::Event_AutoZoom();
	}

	/*
		XXX  Entries todo (see above documentation)

			SnailTrail (on, off, long, toggle)
			VarioSound (on, off)
			Marker (optional text to add)
			MenuButton (on, off, toggle)
			Menu(open, close, toggle)

	*/

};

/*

Extra code...



*/
