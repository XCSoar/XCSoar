/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_INPUT_EVENTS_HPP
#define XCSOAR_INPUT_EVENTS_HPP

#include <tchar.h>

#include "Thread/Mutex.hpp"
#include "Math/fixed.hpp"

typedef void (*pt2Event)(const TCHAR *);

namespace InputEvents
{
  enum mode {
    MODE_DEFAULT,
    MODE_PAN,
    MODE_INFOBOX,
    MODE_MENU
  };

  void ProcessTimer();
  void ShowMenu();
  void HideMenu();
  void ResetMenuTimeOut();

  void readFile();
  void setMode(mode mode);
  void setMode(const TCHAR *mode);
  mode getModeID();
  unsigned findKey(const TCHAR *data);
  int findGCE(const TCHAR *data);
  int findNE(const TCHAR *data);
  pt2Event findEvent(const TCHAR *);

  /**
   * Looks up the specified key code, and returns the associated event
   * id.  Returns 0 if the key was not found.
   */
  unsigned key_to_event(mode mode, unsigned key_code);
  unsigned gesture_to_event(mode mode, const TCHAR *data);

  bool processKey(unsigned key);
  bool processGesture(const TCHAR *data);
  bool processNmea(unsigned key);
  bool processButton(unsigned bindex);
  bool processGlideComputer(unsigned gce_id);
  void processGo(unsigned event_id);
  void makeLabel(mode mode_id, const TCHAR *label,
                        unsigned location, unsigned event_id);

  void drawButtons(mode Mode);


  // helpers (temporary)

  void sub_TerrainTopography(int vswitch);
  void sub_Pan(int vswitch);
  void sub_PanCursor(int dx, int dy);
  void sub_AutoZoom(int vswitch);
  void sub_ScaleZoom(int vswitch);
  void sub_SetZoom(fixed value);

  // -------

  void eventAbortTask(const TCHAR *misc);
  void eventAdjustForecastTemperature(const TCHAR *misc);
  void eventAdjustVarioFilter(const TCHAR *misc);
  void eventAdjustWaypoint(const TCHAR *misc);
  void eventAnalysis(const TCHAR *misc);
  void eventArmAdvance(const TCHAR *misc);
  void eventAudioDeadband(const TCHAR *misc);
  void eventBallast(const TCHAR *misc);
  void eventBugs(const TCHAR *misc);
  void eventCalculator(const TCHAR *misc);
  void eventChangeInfoBoxType(const TCHAR *misc);
  void eventChecklist(const TCHAR *misc);
  void eventClearAirspaceWarnings(const TCHAR *misc);
  void eventClearStatusMessages(const TCHAR *misc);
  void eventClearWarningsOrTerrainTopology(const TCHAR *misc);
  void eventDoInfoKey(const TCHAR *misc);
  void eventLogger(const TCHAR *misc);
  void eventMacCready(const TCHAR *misc);
  void eventMainMenu(const TCHAR *misc);
  void eventMarkLocation(const TCHAR *misc);
  void eventMode(const TCHAR *misc);
  void eventNearestAirspaceDetails(const TCHAR *misc);
  void eventNearestWaypointDetails(const TCHAR *misc);
  void eventNull(const TCHAR *misc);
  void eventPan(const TCHAR *misc);
  void eventPlaySound(const TCHAR *misc);
  void eventProfileLoad(const TCHAR *misc);
  void eventProfileSave(const TCHAR *misc);
  void eventRepeatStatusMessage(const TCHAR *misc);
  void eventRun(const TCHAR *misc);
  void eventScreenModes(const TCHAR *misc);
  void eventSelectInfoBox(const TCHAR *misc);
  void eventSendNMEA(const TCHAR *misc);
  void eventSendNMEAPort1(const TCHAR *misc);
  void eventSendNMEAPort2(const TCHAR *misc);
  void eventSetup(const TCHAR *misc);
  void eventSnailTrail(const TCHAR *misc);
  void eventAirSpace(const TCHAR *misc); // VENTA3
  void eventSounds(const TCHAR *misc);
  void eventStatus(const TCHAR *misc);
  void eventStatusMessage(const TCHAR *misc);
  void eventTaskLoad(const TCHAR *misc);
  void eventTaskSave(const TCHAR *misc);
  void eventTaskTransition(const TCHAR *misc);
  void eventTerrainTopography(const TCHAR *misc);
  void eventTerrainTopology(const TCHAR *misc);
  void eventWaypointDetails(const TCHAR *misc);
  void eventZoom(const TCHAR *misc);
  void eventBrightness(const TCHAR *misc);
  void eventDeclutterLabels(const TCHAR *misc);
  void eventExit(const TCHAR *misc);
  void eventFLARMRadar(const TCHAR *misc);
  void eventThermalAssistant(const TCHAR *misc);
  void eventBeep(const TCHAR *misc);
  void eventUserDisplayModeForce(const TCHAR *misc);
  void eventAirspaceDisplayMode(const TCHAR *misc);
  void eventAutoLogger(const TCHAR *misc);
  void eventGotoLookup(const TCHAR *misc);
  void eventAddWaypoint(const TCHAR *misc);
  void eventOrientation(const TCHAR *misc);
  void eventFlarmTraffic(const TCHAR *misc);
  void eventFlarmDetails(const TCHAR *misc);
  void eventCredits(const TCHAR *misc);
  // -------
};


// GCE = Glide Computer Event
enum {
  GCE_AIRSPACE_ENTER,
  GCE_AIRSPACE_LEAVE,
  GCE_COMMPORT_RESTART,
  GCE_FLARM_NOTRAFFIC,
  GCE_FLARM_TRAFFIC,
  GCE_FLARM_NEWTRAFFIC,
  GCE_FLIGHTMODE_CLIMB,
  GCE_FLIGHTMODE_CRUISE,
  GCE_FLIGHTMODE_FINALGLIDE,
  GCE_FLIGHTMODE_FINALGLIDE_TERRAIN,
  GCE_FLIGHTMODE_FINALGLIDE_ABOVE,
  GCE_FLIGHTMODE_FINALGLIDE_BELOW,
  GCE_GPS_CONNECTION_WAIT,
  GCE_GPS_FIX_WAIT,
  GCE_HEIGHT_MAX,
  GCE_LANDING,
  GCE_STARTUP_REAL,
  GCE_STARTUP_SIMULATOR,
  GCE_TAKEOFF,
  GCE_TASK_NEXTWAYPOINT,
  GCE_TASK_START,
  GCE_TASK_FINISH,
  GCE_TEAM_POS_REACHED,
  GCE_ARM_READY,
  GCE_POLAR_CHANGED,
  GCE_ALTERNATE_CHANGED,
  GCE_LANDABLE_UNREACHABLE,
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
  NE_UNUSED_37 =                      108,	// XXX Duplicate of above
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
  NE_UNUSED_52 =                      122,	// XXX Duplicate of above
  NE_UNUSED_53 =                      123,
  NE_UNUSED_54 =                      124,
  NE_UNUSED_55 =                      125,
  NE_UNUSED_56 =                      126,
  NE_UNUSED_57 =                      127,
  NE_COUNT = 132, // How many we have for arrays etc // XXX Increased arbitrarily for duplicates above
};

#endif
