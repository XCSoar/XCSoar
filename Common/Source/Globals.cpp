
#include "XCSoar.h"
///////////////////////////////////////////////////////
// Junkyard of globals to be eliminated
#include "SettingsTask.hpp"
#include "Task.h"


bool  EnableMultipleStartPoints = false;
int   StartHeightRef = 0; // MSL
int   SelectedWaypoint = -1;
bool  ForceFinalGlide= false;
bool  EnableFAIFinishHeight = false;
int   FinishLine=1;
DWORD FinishRadius=1000;
// Assigned Area Task
double AATTaskLength = 120;
BOOL  AATEnabled = FALSE;
DWORD FinishMinHeight = 0;
DWORD StartMaxHeight = 0;
DWORD StartMaxSpeed = 0;
DWORD StartMaxHeightMargin = 0;
DWORD StartMaxSpeedMargin = 0;
int   ActiveWayPoint = -1;
int   AutoAdvance = 1;
bool  AdvanceArmed = false;
// Waypoint Database
int   SectorType = 1; // FAI sector
DWORD SectorRadius = 500;
int   StartLine = 1;
DWORD StartRadius = 3000;

//Task Information
Task_t Task = {{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0}};
Start_t StartPoints;
TaskStats_t TaskStats;

int HomeWaypoint = -1;
int AirfieldsHomeWaypoint = -1; // VENTA3 force Airfields home to be HomeWaypoint if
                                // an H flag in waypoints file is not available..
// Specials
double QFEAltitudeOffset = 0;
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

int AIRSPACEWARNINGS = TRUE;
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
TCHAR TeammateCode[10];
TCHAR TeamFlarmCNTarget[4]; // CN of the glider to track
int TeamFlarmIdTarget;      // FlarmId of the glider to track
double TeammateLatitude;
double TeammateLongitude;
bool TeammateCodeValid = false;
