/* AUTOMATICALLY GENERATED FILE - DO NOT EDIT BY HAND - see Common/Data/Input/xci2cpp.pl */
int event_id;
int mode_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMainMenu, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventStatusMessage, TEXT("Dropped marker"), event_id);
event_id = InputEvents::makeEvent(&eventMarkLocation, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSelectInfoBox, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("display1"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
makeLabel(mode_id,TEXT(""),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventScaleZoom, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_DOWN] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventScaleZoom, TEXT("+"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_UP] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMarkLocation, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id]['L'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventScreenModes, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id]['R'] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSelectInfoBox, TEXT("<<"), event_id);
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
event_id = InputEvents::makeEvent(&eventClearWarningsAndTerrain, TEXT(""), event_id);
mode_id = InputEvents::mode2int(TEXT("display1"), true);
makeLabel(mode_id,TEXT("Terrain"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventScreenModes, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("display1"), true);
makeLabel(mode_id,TEXT("Layout"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPan, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("display1"), true);
makeLabel(mode_id,TEXT("Pan"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("display2"), event_id);
mode_id = InputEvents::mode2int(TEXT("display1"), true);
makeLabel(mode_id,TEXT(".."),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("up"), event_id);
mode_id = InputEvents::mode2int(TEXT("display1"), true);
Key2Event[mode_id][VK_UP] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("down"), event_id);
mode_id = InputEvents::mode2int(TEXT("display1"), true);
Key2Event[mode_id][VK_DOWN] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("left"), event_id);
mode_id = InputEvents::mode2int(TEXT("display1"), true);
Key2Event[mode_id][VK_LEFT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("right"), event_id);
mode_id = InputEvents::mode2int(TEXT("display1"), true);
Key2Event[mode_id][VK_RIGHT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventAutoZoom, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("display2"), true);
makeLabel(mode_id,TEXT("AutoZoom"),1,event_id);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSnailTrail, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("display2"), true);
makeLabel(mode_id,TEXT("Snail Trail"),2,event_id);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventSounds, TEXT("toggle"), event_id);
mode_id = InputEvents::mode2int(TEXT("display2"), true);
makeLabel(mode_id,TEXT("Vario Sounds"),3,event_id);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventMode, TEXT("default"), event_id);
mode_id = InputEvents::mode2int(TEXT("display2"), true);
makeLabel(mode_id,TEXT(".."),4,event_id);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("up"), event_id);
mode_id = InputEvents::mode2int(TEXT("display2"), true);
Key2Event[mode_id][VK_UP] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("down"), event_id);
mode_id = InputEvents::mode2int(TEXT("display2"), true);
Key2Event[mode_id][VK_DOWN] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("left"), event_id);
mode_id = InputEvents::mode2int(TEXT("display2"), true);
Key2Event[mode_id][VK_LEFT] = event_id;

event_id = 0;
event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("right"), event_id);
mode_id = InputEvents::mode2int(TEXT("display2"), true);
Key2Event[mode_id][VK_RIGHT] = event_id;

