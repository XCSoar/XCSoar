
#include "XCSoar.h"
///////////////////////////////////////////////////////
// Junkyard of globals to be eliminated
#include "Task.h"

// state stuff
int   SelectedWaypoint = -1;

//Task Information

Start_t        task_start_points;
StartStats_t   task_start_stats;

// Specials
#if defined(PNA) || defined(FIVV)
bool needclipping=false; // flag to activate extra clipping for some PNAs
#endif

// user interface settings
bool EnableSoundVario = true;
bool EnableSoundModes = true;
bool EnableSoundTask = true;

//////////////////////////////////////////////////////
#include "SettingsAirspace.hpp"
int AirspacePriority[AIRSPACECLASSCOUNT];

int AIRSPACEWARNINGS = 0;
int WarningTime = 30;
int AcknowledgementTime = 30;
int AltitudeMode = ALLON;
int ClipAltitude = 1000;
int AltWarningMargin = 100;
double airspace_QNH;

// 12 is number of airspace types
int      iAirspaceBrush[AIRSPACECLASSCOUNT] =
  {2,0,0,0,3,3,3,3,0,3,2,3,3,3};
int      iAirspaceColour[AIRSPACECLASSCOUNT] =
  {5,0,0,10,0,0,10,2,0,10,9,3,7,7};
int      iAirspaceMode[AIRSPACECLASSCOUNT] =
  {0,0,0,0,0,0,0,0,0,0,0,1,1,0};

/////////////////////////////////////////////////////

// Team code info

