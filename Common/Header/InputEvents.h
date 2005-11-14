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
  static int InputEvents::findNE(TCHAR *data);
  static pt2Event InputEvents::findEvent(TCHAR *);
  static bool processKey(int key);
  static bool processNmea(int key);
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
  static void eventTerrainTopology(TCHAR *misc);
  static void eventClearWarningsOrTerrainTopology(TCHAR *misc);
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
  static void eventMacCready(TCHAR *misc);
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
  static void eventTaskLoad(TCHAR *misc);
  static void eventTaskSave(TCHAR *misc);
  static void eventSendNMEA(TCHAR *misc);
  static void eventNull(TCHAR *misc);
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

// NE = NMEA Events (hard coded triggered events from the NMEA processor)
enum {
  NE_DOWN_IN_FLAP_POS=                  0,
  NE_DOWN_IN_FLAP_ZERO=                 1,
  NE_DOWN_IN_FLAP_NEG=                  2,
  NE_DOWN_IN_SC=                        3,
  NE_DOWN_IN_GEAR_RETRACTED=            4,
  NE_DOWN_IN_GEAR_EXTENDED=             5,
  NE_DOWN_IN_AIRBRAKENOTLOCKED=         6,
  NE_DOWN_IN_AUX=                       7,
  NE_DOWN_IN_ACK=                       8,
  NE_DOWN_IN_REP=                       9,
  NE_DOWN_IN_CIRCLING_PDA=              10,
  NE_DOWN_IN_CIRCLING_FLARM=            11,
  NE_DOWN_IN_FLYING=                    12,
  NE_DOWN_IN_NOTFLYING=                 13,
  NE_DOWN_IN_PDA_CONNECTED=             14,
  NE_DOWN_IN_VELOCITY_MANOEUVERING=     15,
  NE_DOWN_IN_VELOCITY_FLAP=             16,
  NE_DOWN_IN_VELOCITY_AIRBRAKE=         17,
  NE_DOWN_IN_VELOCITY_TERRAIN=          18,
  NE_DOWN_IN_VELOCITY_NEVEREXCEED=      19,
  NE_DOWN_IN_STALL=                     20,
  NE_DOWN_IN_AIRBRAKELOCKED=            21,
  NE_DOWN_IN_TAKINGOFF=                 22,
  NE_DOWN_IN_USERSWUP=                  23,
  NE_DOWN_IN_USERSWMIDDLE=              24,
  NE_DOWN_IN_USERSWDOWN=                25,

  NE_UNUSED_0 =                      26,
  NE_UNUSED_1 =                      27,
  NE_UNUSED_2 =                      28,
  NE_UNUSED_3 =                      29,
  NE_UNUSED_4 =                      30,
  NE_UNUSED_5 =                      31,

  NE_DOWN_OUT_CIRCLING=                 32,
  NE_DOWN_OUT_VELOCITY_MANOEUVERING=    33,
  NE_DOWN_OUT_VELOCITY_FLAP=            34,
  NE_DOWN_OUT_VELOCITY_AIRBRAKE=        35,
  NE_DOWN_OUT_VELOCITY_TERRAIN=         36,
  NE_DOWN_OUT_VELOCITY_NEVEREXCEED=     37,
  NE_DOWN_OUT_GEAR_LANDING=             38,
  NE_DOWN_OUT_FLAP_LANDING=             39,
  NE_DOWN_OUT_AIRBRAKE_TAKEOFF=         40,
  NE_DOWN_OUT_STALL=                    41,

  NE_UNUSED_6 =                      42,
  NE_UNUSED_7 =                      43,
  NE_UNUSED_8 =                      44,
  NE_UNUSED_9 =                      45,
  NE_UNUSED_10 =                      46,
  NE_UNUSED_11 =                      47,
  NE_UNUSED_12 =                      48,
  NE_UNUSED_13 =                      49,
  NE_UNUSED_14 =                      50,
  NE_UNUSED_15 =                      51,
  NE_UNUSED_16 =                      52,
  NE_UNUSED_17 =                      53,
  NE_UNUSED_18 =                      54,
  NE_UNUSED_19 =                      55,
  NE_UNUSED_20 =                      56,
  NE_UNUSED_21 =                      57,
  NE_UNUSED_22 =                      58,
  NE_UNUSED_23 =                      59,
  NE_UNUSED_24 =                      60,
  NE_UNUSED_25 =                      61,
  NE_UNUSED_26 =                      62,
  NE_UNUSED_27 =                      63,

  NE_UP_IN_FLAP_POS=                  64,
  NE_UP_IN_FLAP_ZERO=                 65,
  NE_UP_IN_FLAP_NEG=                  66,
  NE_UP_IN_SC=                        67,
  NE_UP_IN_GEAR_RETRACTED=            68,
  NE_UP_IN_GEAR_EXTENDED=             69,
  NE_UP_IN_AIRBRAKENOTLOCKED=         70,
  NE_UP_IN_AUX=                       71,
  NE_UP_IN_ACK=                       72,
  NE_UP_IN_REP=                       73,
  NE_UP_IN_CIRCLING_PDA=              74,
  NE_UP_IN_CIRCLING_FLARM=            75,
  NE_UP_IN_FLYING=                    76,
  NE_UP_IN_NOTFLYING=                 77,
  NE_UP_IN_PDA_CONNECTED=             78,
  NE_UP_IN_VELOCITY_MANOEUVERING=     79,
  NE_UP_IN_VELOCITY_FLAP=             80,
  NE_UP_IN_VELOCITY_AIRBRAKE=         81,
  NE_UP_IN_VELOCITY_TERRAIN=          82,
  NE_UP_IN_VELOCITY_NEVEREXCEED=      83,
  NE_UP_IN_STALL=                     84,
  NE_UP_IN_AIRBRAKELOCKED=            85,
  NE_UP_IN_TAKINGOFF=                 86,
  NE_UP_IN_USERSWUP=                  87,
  NE_UP_IN_USERSWMIDDLE=              88,
  NE_UP_IN_USERSWDOWN=                89,

  NE_UNUSED_28 =                      90,
  NE_UNUSED_29 =                      91,
  NE_UNUSED_30 =                      92,
  NE_UNUSED_31 =                      93,
  NE_UNUSED_32 =                      94,
  NE_UNUSED_33 =                      95,

  NE_UP_OUT_CIRCLING=                 96,
  NE_UP_OUT_VELOCITY_MANOEUVERING=    97,
  NE_UP_OUT_VELOCITY_FLAP=            98,
  NE_UP_OUT_VELOCITY_AIRBRAKE=        99,
  NE_UP_OUT_VELOCITY_TERRAIN=         100,
  NE_UP_OUT_VELOCITY_NEVEREXCEED=     101,
  NE_UP_OUT_GEAR_LANDING=             102,
  NE_UP_OUT_FLAP_LANDING=             103,
  NE_UP_OUT_AIRBRAKE_TAKEOFF=         104,
  NE_UP_OUT_STALL=                    105,

  NE_UNUSED_34 =                      106,
  NE_UNUSED_35 =                      107,
  NE_UNUSED_36 =                      108,
  NE_UNUSED_37 =                      108,
  NE_UNUSED_38 =                      109,
  NE_UNUSED_39 =                      110,
  NE_UNUSED_40 =                      111,
  NE_UNUSED_41 =                      112,
  NE_UNUSED_42 =                      113,
  NE_UNUSED_43 =                      114,
  NE_UNUSED_44 =                      115,
  NE_UNUSED_45 =                      116,
  NE_UNUSED_46 =                      117,
  NE_UNUSED_47 =                      118,
  NE_UNUSED_48 =                      119,
  NE_UNUSED_49 =                      120,
  NE_UNUSED_50 =                      121,
  NE_UNUSED_51 =                      122,
  NE_UNUSED_52 =                      122,
  NE_UNUSED_53 =                      123,
  NE_UNUSED_54 =                      124,
  NE_UNUSED_55 =                      125,
  NE_UNUSED_56 =                      126,
  NE_UNUSED_57 =                      127,
  NE_COUNT = 128, // How many we have for arrays etc
};

/*
#ifdef _SIM_
#define _INPUTDEBUG_
#endif
*/

#endif
