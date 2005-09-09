/* AUTOMATICALLY GENERATED FILE - DO NOT EDIT BY HAND - see Common/Data/Input/xci2cpp.pl */
int event_id;
int mode_id;

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
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Simulation\r\nNothing is real!"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_STARTUP_SIMULATOR] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Maintain effective\r\nLOOKOUT at all times"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_STARTUP_REAL] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Takeoff "), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_TAKEOFF] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Landing"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_LANDING] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPlaySound, TEXT("\\My Documents\\FinalGlide.wav"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Above Final Glide"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_FLIGHTMODE_FINALGLIDE_ABOVE] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPlaySound, TEXT("\\My Documents\\Tiptoe.wav"), event_id);
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Marginal Final Glide"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
GC2Event[mode_id][GCE_FLIGHTMODE_FINALGLIDE_BELOW] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventScreenModes, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSnailTrail, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventSnailTrail, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Dropped marker"), event_id);
event_id = InputEvents::makeEvent(&eventMarkLocation, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Menu1"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT("Main"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventZoom, TEXT("auto show"), event_id);
event_id = InputEvents::makeEvent(&eventZoom, TEXT("auto toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_LEFT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPan, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventPan, TEXT("supertoggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_RIGHT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventZoom, TEXT("out"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_DOWN] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventZoom, TEXT("in"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_UP] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventClearAirspaceWarnings, TEXT(""), event_id);
event_id = InputEvents::makeEvent(&eventClearStatusMessages, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_RETURN] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSelectInfoBox, TEXT("previous"), event_id);
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
makeLabel(mode_id,TEXT("<<"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSelectInfoBox, TEXT("next"), event_id);
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
makeLabel(mode_id,TEXT(">>"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventChangeInfoBoxType, TEXT("previous"), event_id);
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
makeLabel(mode_id,TEXT("<type"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventChangeInfoBoxType, TEXT("next"), event_id);
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
makeLabel(mode_id,TEXT("type>"),3,event_id);
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
event_id = InputEvents::makeEvent(&eventMode, TEXT("Nav1"), event_id);
mode_id = InputEvents::mode2int(TEXT("Menu1"), true);
makeLabel(mode_id,TEXT("Nav/"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Display1"), event_id);
mode_id = InputEvents::mode2int(TEXT("Menu1"), true);
makeLabel(mode_id,TEXT("Display/"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Config1"), event_id);
mode_id = InputEvents::mode2int(TEXT("Menu1"), true);
makeLabel(mode_id,TEXT("Config/"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Menu2"), event_id);
mode_id = InputEvents::mode2int(TEXT("Menu1"), true);
makeLabel(mode_id,TEXT("Menu/"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Info1"), event_id);
mode_id = InputEvents::mode2int(TEXT("Menu2"), true);
makeLabel(mode_id,TEXT("Info/"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
mode_id = InputEvents::mode2int(TEXT("Menu2"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
mode_id = InputEvents::mode2int(TEXT("Menu2"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Menu2"), true);
makeLabel(mode_id,TEXT("/Menu"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Dropped marker"), event_id);
event_id = InputEvents::makeEvent(&eventMarkLocation, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav1"), true);
makeLabel(mode_id,TEXT("Mark"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
event_id = InputEvents::makeEvent(&eventWaypointDetails, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav1"), true);
makeLabel(mode_id,TEXT("Waypt details"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
event_id = InputEvents::makeEvent(&eventAbortTask, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav1"), true);
makeLabel(mode_id,TEXT("Abort task"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Nav2"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav1"), true);
makeLabel(mode_id,TEXT("Nav/"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventLogger, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventLogger, TEXT("toggle ask"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav2"), true);
makeLabel(mode_id,TEXT("Log"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventAdjustWaypoint, TEXT("previous"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav2"), true);
makeLabel(mode_id,TEXT("Waypt previous"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventAdjustWaypoint, TEXT("next"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav2"), true);
makeLabel(mode_id,TEXT("Waypt next"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Nav2"), true);
makeLabel(mode_id,TEXT("/Nav"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventClearWarningsAndTerrain, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("Display1"), true);
makeLabel(mode_id,TEXT("Terrain"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
event_id = InputEvents::makeEvent(&eventScreenModes, TEXT("togglefull"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display1"), true);
makeLabel(mode_id,TEXT("Full screen"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPan, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventPan, TEXT("supertoggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display1"), true);
makeLabel(mode_id,TEXT("Pan"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Display2"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display1"), true);
makeLabel(mode_id,TEXT("Display/"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPan, TEXT("up"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display1"), true);
Key2Event[mode_id][VK_UP] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPan, TEXT("down"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display1"), true);
Key2Event[mode_id][VK_DOWN] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPan, TEXT("left"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display1"), true);
Key2Event[mode_id][VK_LEFT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPan, TEXT("right"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display1"), true);
Key2Event[mode_id][VK_RIGHT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventZoom, TEXT("auto show"), event_id);
event_id = InputEvents::makeEvent(&eventZoom, TEXT("auto toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display2"), true);
makeLabel(mode_id,TEXT("Auto Zoom"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSnailTrail, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventSnailTrail, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display2"), true);
makeLabel(mode_id,TEXT("Snail"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
mode_id = InputEvents::mode2int(TEXT("Display2"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Display2"), true);
makeLabel(mode_id,TEXT("/Display"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMcCready, TEXT("up"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config1"), true);
makeLabel(mode_id,TEXT("McCready +"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMcCready, TEXT("down"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config1"), true);
makeLabel(mode_id,TEXT("McCready -"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMcCready, TEXT("auto show"), event_id);
event_id = InputEvents::makeEvent(&eventMcCready, TEXT("auto toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config1"), true);
makeLabel(mode_id,TEXT("McCready Auto"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Config2"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config1"), true);
makeLabel(mode_id,TEXT("Config/"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Bugs"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config2"), true);
makeLabel(mode_id,TEXT("Bugs/"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("Ballast"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config2"), true);
makeLabel(mode_id,TEXT("Ballast/"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSounds, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventSounds, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config2"), true);
makeLabel(mode_id,TEXT("Audio"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Config2"), true);
makeLabel(mode_id,TEXT("/Config"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventScreenModes, TEXT("toggleauxiliary"), event_id);
mode_id = InputEvents::mode2int(TEXT("Info1"), true);
makeLabel(mode_id,TEXT("Aux Info"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
event_id = InputEvents::makeEvent(&eventStatus, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("Info1"), true);
makeLabel(mode_id,TEXT("Status"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
event_id = InputEvents::makeEvent(&eventAnalysis, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("Info1"), true);
makeLabel(mode_id,TEXT("Analysis"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Info1"), true);
makeLabel(mode_id,TEXT("/Info"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventBugs, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventBugs, TEXT("max"), event_id);
mode_id = InputEvents::mode2int(TEXT("Bugs"), true);
makeLabel(mode_id,TEXT("Clean"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventBugs, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventBugs, TEXT("up"), event_id);
mode_id = InputEvents::mode2int(TEXT("Bugs"), true);
makeLabel(mode_id,TEXT("Bugs -"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventBugs, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventBugs, TEXT("down"), event_id);
mode_id = InputEvents::mode2int(TEXT("Bugs"), true);
makeLabel(mode_id,TEXT("Bugs +"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Bugs"), true);
makeLabel(mode_id,TEXT("/Bugs"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventBallast, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventBallast, TEXT("min"), event_id);
mode_id = InputEvents::mode2int(TEXT("Ballast"), true);
makeLabel(mode_id,TEXT("Empty"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventBallast, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventBallast, TEXT("down"), event_id);
mode_id = InputEvents::mode2int(TEXT("Ballast"), true);
makeLabel(mode_id,TEXT("Ballast -"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventBallast, TEXT("show"), event_id);
event_id = InputEvents::makeEvent(&eventBallast, TEXT("up"), event_id);
mode_id = InputEvents::mode2int(TEXT("Ballast"), true);
makeLabel(mode_id,TEXT("Ballast +"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("Ballast"), true);
makeLabel(mode_id,TEXT("/Ballast"),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

