#ifndef INPUTEVENTS_H
#define INPUTEVENTS_H

#include "XCSoar.h"
#include "externs.h"

typedef void (*pt2Event)(TCHAR *);

HINSTANCE _loadDLL(TCHAR *name);

class InputEvents {
 public:
  static void readFile();
  static int mode2int(TCHAR *mode, bool create);
  static void setMode(TCHAR *mode);
  static TCHAR* getMode();
  static int getModeID();
  static int InputEvents::findKey(TCHAR *data);
  static int InputEvents::findGCE(TCHAR *data);
  static pt2Event InputEvents::findEvent(TCHAR *);
  static bool processKey(int key);
  static bool processNmea(TCHAR *data);
  static bool processButton(int bindex);
  static bool processGlideComputer(int);
  static void processGo(int event_id);
  static int  makeEvent(void (*event)(TCHAR *), TCHAR *misc, int next = 0);
  static void makeLabel(int mode_id, TCHAR *label, int location, int event_id);

  // -------

  static void eventScreenModes(TCHAR *misc);
  static void eventSnailTrail(TCHAR *misc);
  static void eventSounds(TCHAR *misc);
  static void eventMarkLocation(TCHAR *misc);
  static void eventZoom(TCHAR *misc);
  static void eventPan(TCHAR *misc);
  static void eventClearWarningsAndTerrain(TCHAR *misc);
  static void eventSelectInfoBox(TCHAR *misc);
  static void eventChangeInfoBoxType(TCHAR *misc);
  static void eventDoInfoKey(TCHAR *misc);
  static void eventMainMenu(TCHAR *misc);
  static void eventMode(TCHAR *misc);
  static void eventStatus(TCHAR *misc);
  static void eventAnalysis(TCHAR *misc);
  static void eventWaypointDetails(TCHAR *misc);
  static void eventStatusMessage(TCHAR *misc);
  static void eventPlaySound(TCHAR *misc);
  static void eventMcCready(TCHAR *misc);
  static void eventWind(TCHAR *misc);
  static void eventAdjustVarioFilter(TCHAR *misc);
  static void eventAdjustWaypoint(TCHAR *misc);
  static void eventAbortTask(TCHAR *misc);
  static void eventBugs(TCHAR *misc);
  static void eventBallast(TCHAR *misc);
  static void eventLogger(TCHAR *misc);
  static void eventClearAirspaceWarnings(TCHAR *misc);
  static void eventClearStatusMessages(TCHAR *misc);
  static void eventNearestWaypointDetails(TCHAR *misc);
  static void eventNearestAirspaceDetails(TCHAR *misc);
  static void eventRepeatStatusMessage(TCHAR *misc);
  static void eventAudioDeadband(TCHAR *misc);
  static void eventDLLExecute(TCHAR *misc);

  // -------

#ifdef _SIM_
  static void InputEvents::showErrors();
#endif

};


// GCE = Glide Computer Event
enum {
		GCE_FLIGHTMODE_FINALGLIDE,
		GCE_FLIGHTMODE_CRUISE,
		GCE_FLIGHTMODE_CLIMB,
		GCE_HEIGHT_MAX,
		GCE_GPS_CONNECTION_WAIT,
		GCE_COMMPORT_RESTART,
		GCE_GPS_FIX_WAIT,
		GCE_STARTUP_SIMULATOR,
		GCE_STARTUP_REAL,
		GCE_TAKEOFF,
		GCE_LANDING,
		GCE_AIRSPACE_ENTER,
		GCE_AIRSPACE_LEAVE,
		GCE_TASK_START,
		GCE_TASK_NEXTWAYPOINT,
		GCE_FLIGHTMODE_FINALGLIDE_ABOVE,
		GCE_FLIGHTMODE_FINALGLIDE_BELOW,

		GCE_COUNT			// How many we have for arrays etc
};



#endif
