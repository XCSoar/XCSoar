#ifndef XCSOAR_SCREEN_BLANK_HPP
#define XCSOAR_SCREEN_BLANK_HPP

extern bool EnableAutoBlank;
extern int DisplayTimeOut;
extern bool ScreenBlanked;
extern bool ForceShutdown;

void BlankDisplay(bool doblank);

#endif
