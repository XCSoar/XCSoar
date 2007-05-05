/* AUTOMATICALLY GENERATED FILE - DO NOT EDIT BY HAND - see Common/Data/Input/xci2cpp.pl */
int event_id;
int mode_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventBeep, TEXT("1"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Task start"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_TASK_START] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventBeep, TEXT("1"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Task finish"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_TASK_FINISH] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventBeep, TEXT("1"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Next turnpoint"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_TASK_NEXTWAYPOINT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventBeep, TEXT("1"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("In sector, arm advance when ready"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_ARM_READY] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Waiting for GPS Connection"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_GPS_CONNECTION_WAIT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Restarting Comm Ports"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_COMMPORT_RESTART] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Waiting for GPS Fix"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_GPS_FIX_WAIT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Key escape to leave dialogs"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Key enter to press selected button in dialogs"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Key left and right to modify values in dialogs"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Key up and down to move selector in dialogs"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Keys 67890 are horizontal buttons"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Keys 1 NAV 2 DISPLAY 3 CONFIG 4 INFO"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Simulator: Drag mouse to move glider"), event_id);
event_id = InputEvents::makeEvent(&eventTaskLoad, TEXT("Default.tsk"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_STARTUP_SIMULATOR] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Key escape to leave dialogs"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Key enter to press selected button in dialogs"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Key left and right to modify values in dialogs"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Key up and down to move selector in dialogs"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Keys 67890 are horizontal buttons"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Keys 1 NAV 2 DISPLAY 3 CONFIG 4 INFO"), event_id);
event_id = InputEvents::makeEvent(&eventTaskLoad, TEXT("Default.tsk"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_STARTUP_REAL] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventLogger, TEXT("start"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Takeoff"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_TAKEOFF] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Landing"), event_id);
event_id = InputEvents::makeEvent(&eventLogger, TEXT("stop"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_LANDING] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Above Final Glide"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_FLIGHTMODE_FINALGLIDE_ABOVE] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Below Final Glide"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_FLIGHTMODE_FINALGLIDE_BELOW] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Final Glide Through Terrain"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_FLIGHTMODE_FINALGLIDE_TERRAIN] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav1"), true);
Key2Event[mode_id][VK_ESCAPE] = event_id;

mode_id = InputEvents::mode2int(TEXT("Nav2"), true);
Key2Event[mode_id][VK_ESCAPE] = event_id;

mode_id = InputEvents::mode2int(TEXT("Display1"), true);
Key2Event[mode_id][VK_ESCAPE] = event_id;

mode_id = InputEvents::mode2int(TEXT("Display2"), true);
Key2Event[mode_id][VK_ESCAPE] = event_id;

mode_id = InputEvents::mode2int(TEXT("Config1"), true);
Key2Event[mode_id][VK_ESCAPE] = event_id;

mode_id = InputEvents::mode2int(TEXT("Config2"), true);
Key2Event[mode_id][VK_ESCAPE] = event_id;

mode_id = InputEvents::mode2int(TEXT("Info1"), true);
Key2Event[mode_id][VK_ESCAPE] = event_id;

mode_id = InputEvents::mode2int(TEXT("Info2"), true);
Key2Event[mode_id][VK_ESCAPE] = event_id;

mode_id = InputEvents::mode2int(TEXT("Exit"), true);
Key2Event[mode_id][VK_ESCAPE] = event_id;

mode_id = InputEvents::mode2int(TEXT("Menu"), true);
Key2Event[mode_id][VK_ESCAPE] = event_id;

mode_id = InputEvents::mode2int(TEXT("Vario1"), true);
Key2Event[mode_id][VK_ESCAPE] = event_id;

mode_id = InputEvents::mode2int(TEXT("Vario2"), true);
Key2Event[mode_id][VK_ESCAPE] = event_id;

mode_id = InputEvents::mode2int(TEXT("Preview"), true);
Key2Event[mode_id][VK_ESCAPE] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Preview"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_ESCAPE] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPan, TEXT("supertoggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("pan"), true);
Key2Event[mode_id][VK_ESCAPE] = event_id;

event_id = 0;
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

mode_id = InputEvents::mode2int(TEXT("Display1"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

mode_id = InputEvents::mode2int(TEXT("Display2"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

mode_id = InputEvents::mode2int(TEXT("Config1"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

mode_id = InputEvents::mode2int(TEXT("Config2"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

mode_id = InputEvents::mode2int(TEXT("Info1"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

mode_id = InputEvents::mode2int(TEXT("Info2"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

mode_id = InputEvents::mode2int(TEXT("Exit"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

mode_id = InputEvents::mode2int(TEXT("Menu"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

mode_id = InputEvents::mode2int(TEXT("Vario1"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

mode_id = InputEvents::mode2int(TEXT("Vario2"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

mode_id = InputEvents::mode2int(TEXT("pan"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

mode_id = InputEvents::mode2int(TEXT("Preview"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

mode_id = InputEvents::mode2int(TEXT("Nav1"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

mode_id = InputEvents::mode2int(TEXT("Nav2"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

mode_id = InputEvents::mode2int(TEXT("Config1"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

mode_id = InputEvents::mode2int(TEXT("Config2"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

mode_id = InputEvents::mode2int(TEXT("Info1"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

mode_id = InputEvents::mode2int(TEXT("Info2"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

mode_id = InputEvents::mode2int(TEXT("Exit"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

mode_id = InputEvents::mode2int(TEXT("Menu"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

mode_id = InputEvents::mode2int(TEXT("Vario1"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

mode_id = InputEvents::mode2int(TEXT("Vario2"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

mode_id = InputEvents::mode2int(TEXT("pan"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

mode_id = InputEvents::mode2int(TEXT("Preview"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

mode_id = InputEvents::mode2int(TEXT("Nav1"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

mode_id = InputEvents::mode2int(TEXT("Nav2"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

mode_id = InputEvents::mode2int(TEXT("Display1"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

mode_id = InputEvents::mode2int(TEXT("Display2"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

mode_id = InputEvents::mode2int(TEXT("Info1"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

mode_id = InputEvents::mode2int(TEXT("Info2"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

mode_id = InputEvents::mode2int(TEXT("Exit"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

mode_id = InputEvents::mode2int(TEXT("Menu"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

mode_id = InputEvents::mode2int(TEXT("pan"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

mode_id = InputEvents::mode2int(TEXT("Preview"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

mode_id = InputEvents::mode2int(TEXT("Nav1"), true);
makeLabel(mode_id,TEXT(""),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

mode_id = InputEvents::mode2int(TEXT("Nav2"), true);
makeLabel(mode_id,TEXT(""),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

mode_id = InputEvents::mode2int(TEXT("Display1"), true);
makeLabel(mode_id,TEXT(""),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

mode_id = InputEvents::mode2int(TEXT("Display2"), true);
makeLabel(mode_id,TEXT(""),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

mode_id = InputEvents::mode2int(TEXT("Config1"), true);
makeLabel(mode_id,TEXT(""),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

mode_id = InputEvents::mode2int(TEXT("Config2"), true);
makeLabel(mode_id,TEXT(""),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

mode_id = InputEvents::mode2int(TEXT("Vario1"), true);
makeLabel(mode_id,TEXT(""),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

mode_id = InputEvents::mode2int(TEXT("Vario2"), true);
makeLabel(mode_id,TEXT(""),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

mode_id = InputEvents::mode2int(TEXT("pan"), true);
makeLabel(mode_id,TEXT(""),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

mode_id = InputEvents::mode2int(TEXT("Preview"), true);
makeLabel(mode_id,TEXT(""),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPan, TEXT("down"), event_id);
mode_id = InputEvents::mode2int(TEXT("pan"), true);
Key2Event[mode_id][VK_DOWN] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPan, TEXT("up"), event_id);
mode_id = InputEvents::mode2int(TEXT("pan"), true);
Key2Event[mode_id][VK_UP] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPan, TEXT("left"), event_id);
mode_id = InputEvents::mode2int(TEXT("pan"), true);
Key2Event[mode_id][VK_LEFT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPan, TEXT("right"), event_id);
mode_id = InputEvents::mode2int(TEXT("pan"), true);
Key2Event[mode_id][VK_RIGHT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPan, TEXT("supertoggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("pan"), true);
makeLabel(mode_id,TEXT("Pan\nOff"),5,event_id);
Key2Event[mode_id]['6'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventZoom, TEXT("in"), event_id);
mode_id = InputEvents::mode2int(TEXT("pan"), true);
makeLabel(mode_id,TEXT("Zoom\nin"),6,event_id);
Key2Event[mode_id]['7'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventZoom, TEXT("out"), event_id);
mode_id = InputEvents::mode2int(TEXT("pan"), true);
makeLabel(mode_id,TEXT("Zoom\nout"),7,event_id);
Key2Event[mode_id]['8'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventNearestWaypointDetails, TEXT("pan"), event_id);
mode_id = InputEvents::mode2int(TEXT("pan"), true);
makeLabel(mode_id,TEXT("Nearest\nWaypoint "),8,event_id);
Key2Event[mode_id]['9'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventNull, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("pan"), true);
makeLabel(mode_id,TEXT(""),9,event_id);
Key2Event[mode_id]['0'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Nav1"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Display1"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Config1"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Info1"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventZoom, TEXT("out"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_DOWN] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventZoom, TEXT("in"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_UP] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventFLARMRadar, TEXT("toggle"), event_id);
event_id = InputEvents::makeEvent(&eventClearAirspaceWarnings, TEXT(""), event_id);
event_id = InputEvents::makeEvent(&eventClearStatusMessages, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_RETURN] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMarkLocation, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_LEFT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventScreenModes, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_RIGHT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSetup, TEXT("Basic"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),5,event_id);
Key2Event[mode_id]['6'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventCalculator, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT("$(CheckTask) "),6,event_id);
Key2Event[mode_id]['7'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSetup, TEXT("Task"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT("$(CheckWaypointFile)"),7,event_id);
Key2Event[mode_id]['8'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMarkLocation, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),8,event_id);
Key2Event[mode_id]['9'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventArmAdvance, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventArmAdvance, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT("$(CheckTask) "),9,event_id);
Key2Event[mode_id]['0'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSetup, TEXT("Basic"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Preview"), true);
makeLabel(mode_id,TEXT("Setup\nBasic"),5,event_id);
Key2Event[mode_id]['6'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventCalculator, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("Preview"), true);
makeLabel(mode_id,TEXT("Task\nCalc$(CheckTask)"),6,event_id);
Key2Event[mode_id]['7'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSetup, TEXT("Task"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Preview"), true);
makeLabel(mode_id,TEXT("Task\nEdit$(CheckWaypointFile)"),7,event_id);
Key2Event[mode_id]['8'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMarkLocation, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("Preview"), true);
makeLabel(mode_id,TEXT("Mark\nLocation"),8,event_id);
Key2Event[mode_id]['9'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventArmAdvance, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventArmAdvance, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("Preview"), true);
makeLabel(mode_id,TEXT("$(CheckTask)Advance\n$(AdvanceArmed)"),9,event_id);
Key2Event[mode_id]['0'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventNull, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventNull, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
makeLabel(mode_id,TEXT(""),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventNull, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventNull, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventDoInfoKey, TEXT("up"), event_id);
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
Key2Event[mode_id][VK_UP] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventDoInfoKey, TEXT("down"), event_id);
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
Key2Event[mode_id][VK_DOWN] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventDoInfoKey, TEXT("left"), event_id);
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
Key2Event[mode_id][VK_LEFT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventDoInfoKey, TEXT("right"), event_id);
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
Key2Event[mode_id][VK_RIGHT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventDoInfoKey, TEXT("return"), event_id);
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
Key2Event[mode_id][VK_RETURN] = event_id;

event_id = 0;
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
makeLabel(mode_id,TEXT(""),5,event_id);
Key2Event[mode_id]['6'] = event_id;

event_id = 0;
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
makeLabel(mode_id,TEXT(""),6,event_id);
Key2Event[mode_id]['7'] = event_id;

event_id = 0;
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
makeLabel(mode_id,TEXT(""),7,event_id);
Key2Event[mode_id]['8'] = event_id;

event_id = 0;
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
makeLabel(mode_id,TEXT(""),8,event_id);
Key2Event[mode_id]['9'] = event_id;

event_id = 0;
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
makeLabel(mode_id,TEXT(""),9,event_id);
Key2Event[mode_id]['0'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Nav2"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav1"), true);
makeLabel(mode_id,TEXT("Nav"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventCalculator, TEXT(""), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav1"), true);
makeLabel(mode_id,TEXT("Task\nCalc$(CheckTask)"),5,event_id);
Key2Event[mode_id]['6'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventArmAdvance, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventArmAdvance, TEXT("toggle"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav1"), true);
makeLabel(mode_id,TEXT("$(CheckTask)Advance\n$(AdvanceArmed)"),6,event_id);
Key2Event[mode_id]['7'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventAdjustWaypoint, TEXT("previous"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav1"), true);
makeLabel(mode_id,TEXT("$(WaypointPrevious)"),7,event_id);
Key2Event[mode_id]['8'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventAdjustWaypoint, TEXT("next"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav1"), true);
makeLabel(mode_id,TEXT("$(WaypointNext)"),8,event_id);
Key2Event[mode_id]['9'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
event_id = InputEvents::makeEvent(&eventWaypointDetails, TEXT("select"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav1"), true);
makeLabel(mode_id,TEXT("Waypoint\nLookup$(CheckWaypointFile)"),9,event_id);
Key2Event[mode_id]['0'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav2"), true);
makeLabel(mode_id,TEXT("Nav"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSetup, TEXT("Task"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav2"), true);
makeLabel(mode_id,TEXT("Task\nEdit$(CheckWaypointFile)"),5,event_id);
Key2Event[mode_id]['6'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Task saved"), event_id);
event_id = InputEvents::makeEvent(&eventTaskSave, TEXT("persist/Default.tsk"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav2"), true);
makeLabel(mode_id,TEXT("Task\nSave"),6,event_id);
Key2Event[mode_id]['7'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
event_id = InputEvents::makeEvent(&eventAbortTask, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav2"), true);
makeLabel(mode_id,TEXT("Task\n$(TaskAbortToggleActionName)$(CheckWaypointFile)"),7,event_id);
Key2Event[mode_id]['8'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventFlightMode, TEXT("finalglide toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav2"), true);
makeLabel(mode_id,TEXT("$(CheckTask)Final\n$(FinalForceToggleActionName)"),8,event_id);
Key2Event[mode_id]['9'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSetup, TEXT("Teamcode"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav2"), true);
makeLabel(mode_id,TEXT("Team\nCode"),9,event_id);
Key2Event[mode_id]['0'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Display2"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display1"), true);
makeLabel(mode_id,TEXT("Display"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventZoom, TEXT("in"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display1"), true);
makeLabel(mode_id,TEXT("Zoom\nIn"),5,event_id);
Key2Event[mode_id]['6'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventZoom, TEXT("out"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display1"), true);
makeLabel(mode_id,TEXT("Zoom\nOut"),6,event_id);
Key2Event[mode_id]['7'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Dropped marker"), event_id);
event_id = InputEvents::makeEvent(&eventLogger, TEXT("note Mark"), event_id);
event_id = InputEvents::makeEvent(&eventMarkLocation, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("Display1"), true);
makeLabel(mode_id,TEXT("Mark\nDrop"),7,event_id);
Key2Event[mode_id]['8'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventScreenModes, TEXT("togglefull"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display1"), true);
makeLabel(mode_id,TEXT("Full Scrn\nToggle"),8,event_id);
Key2Event[mode_id]['9'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPan, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventPan, TEXT("supertoggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display1"), true);
makeLabel(mode_id,TEXT("Pan\nOn"),9,event_id);
Key2Event[mode_id]['0'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display2"), true);
makeLabel(mode_id,TEXT("Display"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventZoom, TEXT("auto show"), event_id);
event_id = InputEvents::makeEvent(&eventZoom, TEXT("auto toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display2"), true);
makeLabel(mode_id,TEXT("Zoom\n$(ZoomAutoToggleActionName)"),5,event_id);
Key2Event[mode_id]['6'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSnailTrail, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventSnailTrail, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display2"), true);
makeLabel(mode_id,TEXT("Trail\n$(SnailTrailToggleName)"),6,event_id);
Key2Event[mode_id]['7'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventTerrainTopology, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventTerrainTopology, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display2"), true);
makeLabel(mode_id,TEXT("$(TerrainTopologyToggleName)"),7,event_id);
Key2Event[mode_id]['8'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventBrightness, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("Display2"), true);
makeLabel(mode_id,TEXT("Bright"),8,event_id);
Key2Event[mode_id]['9'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventDeclutterLabels, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventDeclutterLabels, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display2"), true);
makeLabel(mode_id,TEXT("Labels\n$(MapLabelsToggleActionName)"),9,event_id);
Key2Event[mode_id]['0'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Config2"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config1"), true);
makeLabel(mode_id,TEXT("Config"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMacCready, TEXT("up"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config1"), true);
makeLabel(mode_id,TEXT("MacCready\n+"),5,event_id);
Key2Event[mode_id]['6'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMacCready, TEXT("down"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config1"), true);
makeLabel(mode_id,TEXT("MacCready\n-"),6,event_id);
Key2Event[mode_id]['7'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMacCready, TEXT("auto show"), event_id);
event_id = InputEvents::makeEvent(&eventMacCready, TEXT("auto toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config1"), true);
makeLabel(mode_id,TEXT("$(CheckTask)MacCready\n$(MacCreadyToggleActionName)"),7,event_id);
Key2Event[mode_id]['8'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSetup, TEXT("Basic"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config1"), true);
makeLabel(mode_id,TEXT("Setup\nBasic"),8,event_id);
Key2Event[mode_id]['9'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSetup, TEXT("Wind"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config1"), true);
makeLabel(mode_id,TEXT("Setup\nWind"),9,event_id);
Key2Event[mode_id]['0'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config2"), true);
makeLabel(mode_id,TEXT("Config"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Vario1"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config2"), true);
makeLabel(mode_id,TEXT("Vario/"),5,event_id);
Key2Event[mode_id]['6'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSetup, TEXT("System"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config2"), true);
makeLabel(mode_id,TEXT("Setup\nSystem$(CheckSettingsLockout)"),6,event_id);
Key2Event[mode_id]['7'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSetup, TEXT("Airspace"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config2"), true);
makeLabel(mode_id,TEXT("Settings\nAirspace$(CheckAirspace)"),7,event_id);
Key2Event[mode_id]['8'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventLogger, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventLogger, TEXT("toggle ask"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config2"), true);
makeLabel(mode_id,TEXT("Logger\n$(LoggerActive)"),8,event_id);
Key2Event[mode_id]['9'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSetup, TEXT("Replay"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config2"), true);
makeLabel(mode_id,TEXT("Logger\nReplay$(CheckReplay)"),9,event_id);
Key2Event[mode_id]['0'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Vario2"), event_id);
mode_id = InputEvents::mode2int(TEXT("Vario1"), true);
makeLabel(mode_id,TEXT("Vario"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSetup, TEXT("Switches"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Vario1"), true);
makeLabel(mode_id,TEXT("Airframe\nSwitches"),5,event_id);
Key2Event[mode_id]['6'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSetup, TEXT("Voice"), event_id);
mode_id = InputEvents::mode2int(TEXT("Vario1"), true);
makeLabel(mode_id,TEXT("Setup Audio"),6,event_id);
Key2Event[mode_id]['7'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventAdjustVarioFilter, TEXT("xdemo"), event_id);
mode_id = InputEvents::mode2int(TEXT("Vario1"), true);
makeLabel(mode_id,TEXT("Manual\nDemo"),7,event_id);
Key2Event[mode_id]['8'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Not yet implemented"), event_id);
mode_id = InputEvents::mode2int(TEXT("Vario1"), true);
makeLabel(mode_id,TEXT("Setup\nStall"),8,event_id);
Key2Event[mode_id]['9'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventAdjustVarioFilter, TEXT("accel"), event_id);
mode_id = InputEvents::mode2int(TEXT("Vario1"), true);
makeLabel(mode_id,TEXT("Accel "),9,event_id);
Key2Event[mode_id]['0'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Vario2"), true);
makeLabel(mode_id,TEXT("Vario"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventAdjustVarioFilter, TEXT("zero"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Vario ASI zeroed"), event_id);
mode_id = InputEvents::mode2int(TEXT("Vario2"), true);
makeLabel(mode_id,TEXT("Zero\nASI"),5,event_id);
Key2Event[mode_id]['6'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Accelerometer leveled"), event_id);
mode_id = InputEvents::mode2int(TEXT("Vario2"), true);
makeLabel(mode_id,TEXT("Accel\nZero"),6,event_id);
Key2Event[mode_id]['7'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventAdjustVarioFilter, TEXT("save"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Stored to EEPROM"), event_id);
mode_id = InputEvents::mode2int(TEXT("Vario2"), true);
makeLabel(mode_id,TEXT("Store"),7,event_id);
Key2Event[mode_id]['8'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventAdjustVarioFilter, TEXT("demostf"), event_id);
mode_id = InputEvents::mode2int(TEXT("Vario2"), true);
makeLabel(mode_id,TEXT("Cruise\nDemo"),8,event_id);
Key2Event[mode_id]['9'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventAdjustVarioFilter, TEXT("democlimb"), event_id);
mode_id = InputEvents::mode2int(TEXT("Vario2"), true);
makeLabel(mode_id,TEXT("Climb\nDemo"),9,event_id);
Key2Event[mode_id]['0'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Info2"), event_id);
mode_id = InputEvents::mode2int(TEXT("Info1"), true);
makeLabel(mode_id,TEXT("Info"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
event_id = InputEvents::makeEvent(&eventWaypointDetails, TEXT("current"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Info1"), true);
makeLabel(mode_id,TEXT("$(CheckTask)Waypoint\nDetails"),5,event_id);
Key2Event[mode_id]['6'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
event_id = InputEvents::makeEvent(&eventNearestWaypointDetails, TEXT("aircraft"), event_id);
mode_id = InputEvents::mode2int(TEXT("Info1"), true);
makeLabel(mode_id,TEXT("$(CheckWaypointFile)Nearest\nWaypoint"),6,event_id);
Key2Event[mode_id]['7'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
event_id = InputEvents::makeEvent(&eventNearestAirspaceDetails, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("Info1"), true);
makeLabel(mode_id,TEXT("Nearest\nAirspace$(CheckAirspace)"),7,event_id);
Key2Event[mode_id]['8'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventChecklist, TEXT(""), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Info1"), true);
makeLabel(mode_id,TEXT("Check\nlist"),8,event_id);
Key2Event[mode_id]['9'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventAnalysis, TEXT(""), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Info1"), true);
makeLabel(mode_id,TEXT("Analysis"),9,event_id);
Key2Event[mode_id]['0'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Info2"), true);
makeLabel(mode_id,TEXT("Info"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatus, TEXT("system"), event_id);
mode_id = InputEvents::mode2int(TEXT("Info2"), true);
makeLabel(mode_id,TEXT("Status\nSystem"),5,event_id);
Key2Event[mode_id]['6'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatus, TEXT("Aircraft"), event_id);
mode_id = InputEvents::mode2int(TEXT("Info2"), true);
makeLabel(mode_id,TEXT("Status\nAircraft"),6,event_id);
Key2Event[mode_id]['7'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatus, TEXT("task"), event_id);
mode_id = InputEvents::mode2int(TEXT("Info2"), true);
makeLabel(mode_id,TEXT("$(CheckTask)Status\nTask"),7,event_id);
Key2Event[mode_id]['8'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventScreenModes, TEXT("toggleauxiliary"), event_id);
mode_id = InputEvents::mode2int(TEXT("Info2"), true);
makeLabel(mode_id,TEXT("Aux Info\n$(AuxInfoToggleActionName)"),8,event_id);
Key2Event[mode_id]['9'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventRepeatStatusMessage, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("Info2"), true);
makeLabel(mode_id,TEXT("Message\nRepeat"),9,event_id);
Key2Event[mode_id]['0'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Menu"), event_id);
mode_id = InputEvents::mode2int(TEXT("Exit"), true);
makeLabel(mode_id,TEXT("Menu"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Nav1"), event_id);
mode_id = InputEvents::mode2int(TEXT("Menu"), true);
makeLabel(mode_id,TEXT("Nav"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Display1"), event_id);
mode_id = InputEvents::mode2int(TEXT("Menu"), true);
makeLabel(mode_id,TEXT("Display"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Config1"), event_id);
mode_id = InputEvents::mode2int(TEXT("Menu"), true);
makeLabel(mode_id,TEXT("Config"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Info1"), event_id);
mode_id = InputEvents::mode2int(TEXT("Menu"), true);
makeLabel(mode_id,TEXT("Info"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
mode_id = InputEvents::mode2int(TEXT("Menu"), true);
makeLabel(mode_id,TEXT(""),5,event_id);
Key2Event[mode_id]['6'] = event_id;

event_id = 0;
mode_id = InputEvents::mode2int(TEXT("Menu"), true);
makeLabel(mode_id,TEXT(""),6,event_id);
Key2Event[mode_id]['7'] = event_id;

event_id = 0;
mode_id = InputEvents::mode2int(TEXT("Menu"), true);
makeLabel(mode_id,TEXT(""),7,event_id);
Key2Event[mode_id]['8'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Menu"), true);
makeLabel(mode_id,TEXT("Cancel"),8,event_id);
Key2Event[mode_id]['9'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventExit, TEXT("system"), event_id);
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Menu"), true);
makeLabel(mode_id,TEXT("Exit"),9,event_id);
Key2Event[mode_id]['0'] = event_id;

