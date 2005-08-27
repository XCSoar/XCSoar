#include "stdafx.h"
#include "XCSoar.h"
#include "InputEvents.h"
#include "Utils.h"
#include "VarioSound.h"
#include "Terrain.h"

int TrailActive = TRUE;


void InputEvents::DoMarkLocation() {
  // ARH Let the user know what's happened
  DoStatusMessage(TEXT("Dropped marker"));

  LockFlightData();
  MarkLocation(GPS_INFO.Longditude, GPS_INFO.Lattitude);
  UnlockFlightData();
  
}

void InputEvents::ToggleSounds() {
  EnableSoundVario = !EnableSoundVario;
  VarioSound_EnableSound((BOOL)EnableSoundVario);
  
  // ARH Let the user know what's happened
  if (EnableSoundVario)
    DoStatusMessage(TEXT("Vario Sounds ON"));
  else
    DoStatusMessage(TEXT("Vario Sounds OFF"));  
}

void InputEvents::ToggleSnailTrail() {
  TrailActive ++;
  if (TrailActive>2) {
    TrailActive=0;
  }
  
  if (TrailActive==0)
    DoStatusMessage(TEXT("SnailTrail OFF"));
  if (TrailActive==1) 
    DoStatusMessage(TEXT("SnailTrail ON Long"));
  if (TrailActive==2) 
    DoStatusMessage(TEXT("SnailTrail ON Short"));
  
}

void InputEvents::ToggleScreenModes() {
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
