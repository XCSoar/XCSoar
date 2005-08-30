#ifndef INPUTEVENTS_H
#define INPUTEVENTS_H

#include "XCSoar.h"
#include "externs.h"

class InputEvents {
 public:
  static void readFile();
  static int mode2int(TCHAR *mode, bool create);
  static void setMode(TCHAR *mode);
  static TCHAR* getMode();
  static int getModeID();
  static bool processKey(int key);
  static bool processNmea(TCHAR *data);
  static bool processButton(int bindex);
  static int makeEvent(void (*event)(TCHAR *), TCHAR *misc);
  static void makeLabel(int mode_id, TCHAR *label, int location, int event_id);
  static void eventGo(int event_id);
  static void eventScreenModes(TCHAR *misc);
  static void eventSnailTrail(TCHAR *misc);
  static void eventSounds(TCHAR *misc);
  static void eventMarkLocation(TCHAR *misc);
  static void eventFullScreen(TCHAR *misc);
  static void eventAutoZoom(TCHAR *misc);
  static void eventScaleZoom(TCHAR *misc);
  static void eventPan(TCHAR *misc);
  static void eventClearWarningsAndTerrain(TCHAR *misc);
  static void eventSelectInfoBox(TCHAR *misc);
  static void eventChangeInfoBoxType(TCHAR *misc);
  static void eventDoInfoKey(TCHAR *misc);
  static void eventPanCursor(TCHAR *misc);
  static void eventMainMenu(TCHAR *misc);
  static void eventMode(TCHAR *misc);

};


#endif
