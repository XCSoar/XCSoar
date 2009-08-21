#ifndef XCSOAR_SCREEN_BLANK_HPP
#define XCSOAR_SCREEN_BLANK_HPP

#if WINDOWSPC < 1 && !defined(GNAV)
#define HAVE_BLANK

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

#else /* !HAVE_BLANK */

enum {
  EnableAutoBlank = false,
  ScreenBlanked = false,
  ForceShutdown = false,
};

static inline  bool
DisplayTimeOutIsFresh()
{
  return true;
}

static inline  void
ResetDisplayTimeOut() {}

static inline void CheckDisplayTimeOut(bool sticky)
{
  (void)sticky;
}

#endif /* !HAVE_BLANK */

#endif
