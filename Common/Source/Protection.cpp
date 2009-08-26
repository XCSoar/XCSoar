#include "XCSoar.h"
#include "Blackboard.hpp"
#include "MapWindow.h"
#include "Trigger.hpp"
#include "Protection.hpp"
#include "InfoBoxManager.h"
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "SettingsUser.hpp"
#include "Device/Parser.h"
#include "GaugeVarioAltA.h"
#include "Logger.h"
#include "Calculations.h"

#include <assert.h>

static Trigger gpsUpdatedTriggerEvent(TEXT("gpsUpdatedTriggerEvent"));
static Trigger dataTriggerEvent(TEXT("dataTriggerEvent"));
static Trigger varioTriggerEvent(TEXT("varioTriggerEvent"));
Trigger closeTriggerEvent(TEXT("mapCloseEvent"));
Trigger drawTriggerEvent(TEXT("drawTriggerEvent"));

CRITICAL_SECTION  CritSec_FlightData;
bool csFlightDataInitialized = false;
CRITICAL_SECTION  CritSec_EventQueue;
bool csEventQueueInitialized = false;
CRITICAL_SECTION  CritSec_TerrainDataGraphics;
bool csTerrainDataGraphicsInitialized = false;
CRITICAL_SECTION  CritSec_TerrainDataCalculations;
bool csTerrainDataCalculationsInitialized = false;
CRITICAL_SECTION  CritSec_NavBox;
bool csNavBoxInitialized = false;
CRITICAL_SECTION  CritSec_Comm;
bool csCommInitialized = false;
CRITICAL_SECTION  CritSec_TaskData;
bool csTaskDataInitialized = false;

//////////

void TriggerGPSUpdate()
{
  gpsUpdatedTriggerEvent.trigger();
  dataTriggerEvent.trigger();
}

void TriggerVarioUpdate()
{
  varioTriggerEvent.trigger(); // was pulse
}

void TriggerAll(void) {
  dataTriggerEvent.trigger();
  drawTriggerEvent.trigger();
  varioTriggerEvent.trigger();
}

void TriggerRedraws() {
  if (MapWindow::IsDisplayRunning()) {
    if (gpsUpdatedTriggerEvent.test()) {
      MapWindow::dirtyEvent.trigger();
      if (!drawTriggerEvent.test()) {
	drawTriggerEvent.trigger();
      }
      // only ask for redraw if the thread was waiting,
      // this causes the map thread to try to synchronise
      // with the calculation thread, which is desirable
      // to reduce latency
      // it also ensures that if the display is lagging,
      // it will have a chance to catch up.
    }
  }
}


void InitialiseProtection(void) {
  InitializeCriticalSection(&CritSec_EventQueue);
  csEventQueueInitialized = true;
  InitializeCriticalSection(&CritSec_TaskData);
  csTaskDataInitialized = true;
  InitializeCriticalSection(&CritSec_FlightData);
  csFlightDataInitialized = true;
  InitializeCriticalSection(&CritSec_NavBox);
  csNavBoxInitialized = true;
  InitializeCriticalSection(&CritSec_Comm);
  csCommInitialized = true;
  InitializeCriticalSection(&CritSec_TerrainDataGraphics);
  csTerrainDataGraphicsInitialized = true;
  InitializeCriticalSection(&CritSec_TerrainDataCalculations);
  csTerrainDataCalculationsInitialized = true;
}


void DeleteProtection(void) {
  DeleteCriticalSection(&CritSec_EventQueue);
  csEventQueueInitialized = false;
  DeleteCriticalSection(&CritSec_TaskData);
  csTaskDataInitialized = false;
  DeleteCriticalSection(&CritSec_FlightData);
  csFlightDataInitialized = false;
  DeleteCriticalSection(&CritSec_NavBox);
  csNavBoxInitialized = false;
  DeleteCriticalSection(&CritSec_Comm);
  csCommInitialized = false;
  DeleteCriticalSection(&CritSec_TerrainDataCalculations);
  csTerrainDataGraphicsInitialized = false;
  DeleteCriticalSection(&CritSec_TerrainDataGraphics);
  csTerrainDataCalculationsInitialized = false;
}


/////////////////////////////////////////////////////////////////

void LockNavBox() {
}

void UnlockNavBox() {
}

static int csCount_TaskData = 0;
static int csCount_FlightData = 0;
static int csCount_EventQueue = 0;

void LockTaskData() {
#ifdef HAVEEXCEPTIONS
  if (!csTaskDataInitialized) throw TEXT("LockTaskData Error");
#endif
  EnterCriticalSection(&CritSec_TaskData);
  csCount_TaskData++;
}

void UnlockTaskData() {
#ifdef HAVEEXCEPTIONS
  if (!csTaskDataInitialized) throw TEXT("LockTaskData Error");
#endif
  if (csCount_TaskData)
    csCount_TaskData--;
  LeaveCriticalSection(&CritSec_TaskData);
}


void LockFlightData() {
#ifdef HAVEEXCEPTIONS
  if (!csFlightDataInitialized) throw TEXT("LockFlightData Error");
#endif
  EnterCriticalSection(&CritSec_FlightData);
  csCount_FlightData++;
}

void UnlockFlightData() {
#ifdef HAVEEXCEPTIONS
  if (!csFlightDataInitialized) throw TEXT("LockFlightData Error");
#endif
  if (csCount_FlightData)
    csCount_FlightData--;
  LeaveCriticalSection(&CritSec_FlightData);
}

void LockTerrainDataCalculations() {
#ifdef HAVEEXCEPTIONS
  if (!csTerrainDataCalculationsInitialized) throw TEXT("LockTerrainDataCalculations Error");
#endif
  EnterCriticalSection(&CritSec_TerrainDataCalculations);
}

void UnlockTerrainDataCalculations() {
#ifdef HAVEEXCEPTIONS
  if (!csTerrainDataCalculationsInitialized) throw TEXT("LockTerrainDataCalculations Error");
#endif
  LeaveCriticalSection(&CritSec_TerrainDataCalculations);
}

void LockTerrainDataGraphics() {
#ifdef HAVEEXCEPTIONS
  if (!csTerrainDataGraphicsInitialized) throw TEXT("LockTerrainDataGraphics Error");
#endif
  EnterCriticalSection(&CritSec_TerrainDataGraphics);
}

void UnlockTerrainDataGraphics() {
#ifdef HAVEEXCEPTIONS
  if (!csTerrainDataGraphicsInitialized) throw TEXT("LockTerrainDataGraphics Error");
#endif
  LeaveCriticalSection(&CritSec_TerrainDataGraphics);
}


void LockEventQueue() {
#ifdef HAVEEXCEPTIONS
  if (!csEventQueueInitialized) throw TEXT("LockEventQueue Error");
#endif
  EnterCriticalSection(&CritSec_EventQueue);
  csCount_EventQueue++;
}

void UnlockEventQueue() {
#ifdef HAVEEXCEPTIONS
  if (!csEventQueueInitialized) throw TEXT("LockEventQueue Error");
#endif
  if (csCount_EventQueue)
    csCount_EventQueue--;
  LeaveCriticalSection(&CritSec_EventQueue);
}


void LockComm() {
#ifdef HAVEEXCEPTIONS
  if (!csCommInitialized) throw TEXT("LockComm Error");
#endif
  EnterCriticalSection(&CritSec_Comm);
}

void UnlockComm() {
#ifdef HAVEEXCEPTIONS
  if (!csCommInitialized) throw TEXT("LockComm Error");
#endif
  LeaveCriticalSection(&CritSec_Comm);
}


DWORD InstrumentThread (LPVOID lpvoid) {
	(void)lpvoid;
  // wait for proper startup signal
  while (!MapWindow::IsDisplayRunning()) {
    Sleep(100);
  }

  while (!closeTriggerEvent.test()) {

    if (closeTriggerEvent.test())
      break; // drop out on exit

    if (varioTriggerEvent.wait(5000)) {
      if (MapWindow::IsDisplayRunning()) {
        if (EnableVarioGauge) {
          GaugeVario::Render();
        }
      }
    }
  }
  return 0;
}


DWORD CalculationThread (LPVOID lpvoid) {
	(void)lpvoid;
  bool need_calculations_slow;

  NMEA_INFO     tmp_GPS_INFO;
  DERIVED_INFO  tmp_CALCULATED_INFO;

  need_calculations_slow = false;

  // wait for proper startup signal
  while (!MapWindow::IsDisplayRunning()) {
    Sleep(100);
  }

  while (!closeTriggerEvent.test()) {

    dataTriggerEvent.wait(5000);
    if (closeTriggerEvent.test())
      break; // drop out on exit

    // set timer to determine latency (including calculations)
    if (gpsUpdatedTriggerEvent.test()) {
      //      MapWindow::UpdateTimeStats(true);
    }

    // make local copy before editing...
    LockFlightData();
    if (gpsUpdatedTriggerEvent.test()) { // timeout on FLARM objects
      FLARM_RefreshSlots(&GPS_INFO);
    }
    memcpy(&tmp_GPS_INFO,&GPS_INFO,sizeof(NMEA_INFO));
    memcpy(&tmp_CALCULATED_INFO,&CALCULATED_INFO,sizeof(DERIVED_INFO));

    UnlockFlightData();

    // Do vario first to reduce audio latency
    if (GPS_INFO.VarioAvailable) {
      if (DoCalculationsVario(&tmp_GPS_INFO,&tmp_CALCULATED_INFO)) {

      }
      // assume new vario data has arrived, so infoboxes
      // need to be redrawn
    } else {
      // run the function anyway, because this gives audio functions
      // if no vario connected
      if (gpsUpdatedTriggerEvent.test()) {
	if (DoCalculationsVario(&tmp_GPS_INFO,&tmp_CALCULATED_INFO)) {
	}
	TriggerVarioUpdate(); // emulate vario update
      }
    }

    if (gpsUpdatedTriggerEvent.test()) {
      if(DoCalculations(&tmp_GPS_INFO,&tmp_CALCULATED_INFO)){

        DisplayMode_t lastDisplayMode = DisplayMode;

	MapWindow::dirtyEvent.trigger();
        need_calculations_slow = true;

        switch (UserForceDisplayMode){
          case dmCircling:
            DisplayMode = dmCircling;
          break;
          case dmCruise:
            DisplayMode = dmCruise;
          break;
          case dmFinalGlide:
            DisplayMode = dmFinalGlide;
          break;
          case dmNone:
            if (tmp_CALCULATED_INFO.Circling){
              DisplayMode = dmCircling;
            } else if (tmp_CALCULATED_INFO.FinalGlide){
              DisplayMode = dmFinalGlide;
            } else
              DisplayMode = dmCruise;
          break;
        }

        if (lastDisplayMode != DisplayMode){
          MapWindow::SwitchZoomClimb();
        }

      }
      InfoBoxesSetDirty(true);
    }

    if (closeTriggerEvent.test())
      break; // drop out on exit

    TriggerRedraws();

    if (closeTriggerEvent.test())
      break; // drop out on exit

#if defined(_SIM_)
    need_calculations_slow |= (EnableBestAlternate && ReplayLogger::IsEnabled());
    // VENTA3, needed for BestAlternate SIM
#endif

    if (need_calculations_slow) {
      DoCalculationsSlow(&tmp_GPS_INFO,&tmp_CALCULATED_INFO);
      need_calculations_slow = false;
    }

    if (closeTriggerEvent.test())
      break; // drop out on exit

    // values changed, so copy them back now: ONLY CALCULATED INFO
    // should be changed in DoCalculations, so we only need to write
    // that one back (otherwise we may write over new data)
    LockFlightData();
    memcpy(&CALCULATED_INFO,&tmp_CALCULATED_INFO,sizeof(DERIVED_INFO));
    UnlockFlightData();

    gpsUpdatedTriggerEvent.reset();
  }
  return 0;
}


void CreateCalculationThread(void) {
  HANDLE hCalculationThread;
  DWORD dwCalcThreadID;

  // Create a read thread for performing calculations
  if ((hCalculationThread =
      CreateThread (NULL, 0,
        (LPTHREAD_START_ROUTINE )CalculationThread,
         0, 0, &dwCalcThreadID)) != NULL)
  {
    SetThreadPriority(hCalculationThread, THREAD_PRIORITY_NORMAL);
    CloseHandle (hCalculationThread);
  } else {
    assert(1);
  }

  HANDLE hInstrumentThread;
  DWORD dwInstThreadID;

  if ((hInstrumentThread =
      CreateThread (NULL, 0,
       (LPTHREAD_START_ROUTINE )InstrumentThread,
        0, 0, &dwInstThreadID)) != NULL)
  {
    SetThreadPriority(hInstrumentThread, THREAD_PRIORITY_NORMAL);
    CloseHandle (hInstrumentThread);
  } else {
    assert(1);
  }

}
