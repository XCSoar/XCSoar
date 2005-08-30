int event_id;
int mode_id;

event_id = InputEvents::makeEvent(&eventMainMenu, TEXT(""));
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = InputEvents::makeEvent(&eventMarkLocation, TEXT(""));
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = InputEvents::makeEvent(&eventSelectInfoBox, TEXT(""));
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = InputEvents::makeEvent(&eventMode, TEXT("display1"));
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = InputEvents::makeEvent(&eventScaleZoom, TEXT("-"));
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_DOWN] = event_id;

event_id = InputEvents::makeEvent(&eventScaleZoom, TEXT("+"));
mode_id = InputEvents::mode2int(TEXT("default"), true);
Key2Event[mode_id][VK_UP] = event_id;

event_id = InputEvents::makeEvent(&eventSelectInfoBox, TEXT("previous"));
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
Key2Event[mode_id][VK_APP1] = event_id;

event_id = InputEvents::makeEvent(&eventSelectInfoBox, TEXT("next"));
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
Key2Event[mode_id][VK_APP4] = event_id;

event_id = InputEvents::makeEvent(&eventChangeInfoBoxType, TEXT("previous"));
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
Key2Event[mode_id][VK_APP2] = event_id;

event_id = InputEvents::makeEvent(&eventChangeInfoBoxType, TEXT("next"));
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
Key2Event[mode_id][VK_APP3] = event_id;

event_id = InputEvents::makeEvent(&eventDoInfoKey, TEXT("up"));
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
Key2Event[mode_id][VK_UP] = event_id;

event_id = InputEvents::makeEvent(&eventDoInfoKey, TEXT("down"));
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
Key2Event[mode_id][VK_DOWN] = event_id;

event_id = InputEvents::makeEvent(&eventDoInfoKey, TEXT("left"));
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
Key2Event[mode_id][VK_LEFT] = event_id;

event_id = InputEvents::makeEvent(&eventDoInfoKey, TEXT("right"));
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
Key2Event[mode_id][VK_RIGHT] = event_id;

event_id = InputEvents::makeEvent(&eventDoInfoKey, TEXT("return"));
mode_id = InputEvents::mode2int(TEXT("infobox"), true);
Key2Event[mode_id][VK_RETURN] = event_id;

event_id = InputEvents::makeEvent(&eventClearWarningsAndTerrain, TEXT(""));
mode_id = InputEvents::mode2int(TEXT("display1"), true);
ModeLabel[mode_id][0].label=TEXT("Terrain");
ModeLabel[mode_id][0].location=1;
ModeLabel[mode_id][0].event=event_id;
Key2Event[mode_id][VK_APP1] = event_id;

event_id = InputEvents::makeEvent(&eventScreenMode, TEXT("toggle"));
mode_id = InputEvents::mode2int(TEXT("display1"), true);
ModeLabel[mode_id][1].label=TEXT("Layout");
ModeLabel[mode_id][1].location=2;
ModeLabel[mode_id][1].event=event_id;
Key2Event[mode_id][VK_APP2] = event_id;

event_id = InputEvents::makeEvent(&eventPan, TEXT("toggle"));
mode_id = InputEvents::mode2int(TEXT("display1"), true);
ModeLabel[mode_id][2].label=TEXT("Pan");
ModeLabel[mode_id][2].location=3;
ModeLabel[mode_id][2].event=event_id;
Key2Event[mode_id][VK_APP3] = event_id;

event_id = InputEvents::makeEvent(&eventMode, TEXT("display2"));
mode_id = InputEvents::mode2int(TEXT("display1"), true);
ModeLabel[mode_id][3].label=TEXT("..");
ModeLabel[mode_id][3].location=4;
ModeLabel[mode_id][3].event=event_id;
Key2Event[mode_id][VK_APP4] = event_id;

event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("up"));
mode_id = InputEvents::mode2int(TEXT("display1"), true);
Key2Event[mode_id][VK_UP] = event_id;

event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("down"));
mode_id = InputEvents::mode2int(TEXT("display1"), true);
Key2Event[mode_id][VK_DOWN] = event_id;

event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("left"));
mode_id = InputEvents::mode2int(TEXT("display1"), true);
Key2Event[mode_id][VK_LEFT] = event_id;

event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("right"));
mode_id = InputEvents::mode2int(TEXT("display1"), true);
Key2Event[mode_id][VK_RIGHT] = event_id;

event_id = InputEvents::makeEvent(&eventAutoZoom, TEXT("toggle"));
mode_id = InputEvents::mode2int(TEXT("display2"), true);
ModeLabel[mode_id][0].label=TEXT("AutoZoom");
ModeLabel[mode_id][0].location=1;
ModeLabel[mode_id][0].event=event_id;
Key2Event[mode_id][VK_APP1] = event_id;

event_id = InputEvents::makeEvent(&eventSnailTrail, TEXT("toggle"));
mode_id = InputEvents::mode2int(TEXT("display2"), true);
ModeLabel[mode_id][1].label=TEXT("Snail Trail");
ModeLabel[mode_id][1].location=2;
ModeLabel[mode_id][1].event=event_id;
Key2Event[mode_id][VK_APP2] = event_id;

event_id = InputEvents::makeEvent(&eventSounds, TEXT("toggle"));
mode_id = InputEvents::mode2int(TEXT("display2"), true);
ModeLabel[mode_id][2].label=TEXT("Vario Sounds");
ModeLabel[mode_id][2].location=3;
ModeLabel[mode_id][2].event=event_id;
Key2Event[mode_id][VK_APP3] = event_id;

event_id = InputEvents::makeEvent(&eventMode, TEXT("default"));
mode_id = InputEvents::mode2int(TEXT("display2"), true);
ModeLabel[mode_id][3].label=TEXT("..");
ModeLabel[mode_id][3].location=4;
ModeLabel[mode_id][3].event=event_id;
Key2Event[mode_id][VK_APP4] = event_id;

event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("up"));
mode_id = InputEvents::mode2int(TEXT("display2"), true);
Key2Event[mode_id][VK_UP] = event_id;

event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("down"));
mode_id = InputEvents::mode2int(TEXT("display2"), true);
Key2Event[mode_id][VK_DOWN] = event_id;

event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("left"));
mode_id = InputEvents::mode2int(TEXT("display2"), true);
Key2Event[mode_id][VK_LEFT] = event_id;

event_id = InputEvents::makeEvent(&eventPanCursor, TEXT("right"));
mode_id = InputEvents::mode2int(TEXT("display2"), true);
Key2Event[mode_id][VK_RIGHT] = event_id;

