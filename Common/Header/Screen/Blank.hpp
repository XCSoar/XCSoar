#ifndef XCSOAR_SCREEN_BLANK_HPP
#define XCSOAR_SCREEN_BLANK_HPP

extern bool EnableAutoBlank;
extern int DisplayTimeOut;
extern bool ScreenBlanked;
extern bool ForceShutdown;

void BlankDisplay(bool doblank);

static inline  bool
DisplayTimeOutIsFresh()
{
  return DisplayTimeOut == 0;
}

static inline  void
ResetDisplayTimeOut()
{
  DisplayTimeOut = 0;
}

void CheckDisplayTimeOut(bool sticky);

#endif
