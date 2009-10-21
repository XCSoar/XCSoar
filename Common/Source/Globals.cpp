#include "XCSoar.h"
// Junkyard of globals to be eliminated
#include "Task.h"

//Task Information

Start_t        task_start_points;
StartStats_t   task_start_stats;

// Specials
#if defined(PNA) || defined(FIVV)
bool needclipping=false; // flag to activate extra clipping for some PNAs
#endif

#include "SettingsAirspace.hpp"
int AirspacePriority[AIRSPACECLASSCOUNT];

double airspace_QNH;
