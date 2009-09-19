
#include "XCSoar.h"
///////////////////////////////////////////////////////
// Junkyard of globals to be eliminated
#include "Task.h"

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

double airspace_QNH;

// 12 is number of airspace types
int      iAirspaceMode[AIRSPACECLASSCOUNT] =
  {0,0,0,0,0,0,0,0,0,0,0,1,1,0};
