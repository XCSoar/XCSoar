#ifndef OPTIONS_H
#define OPTIONS_H

#define   MONOCHROME_SCREEN     1             // optimize for monochrom screen
#define   EXPERIMENTAL          0             // ????

#define   LOGGDEVICEINSTREAM    0             // log device in stream
#define   LOGGDEVCOMMANDLINE    NULL          // device in-stream logger command line
                                              // ie TEXT("-logA=\\Speicherkarte\\logA.log ""-logB=\\SD Card\\logB.log""")
#define   AIRSPACEUSEBINFILE    0             // use and maintain binary airspace file

// define this to be true for windows PC port
#if !defined(WINDOWSPC)
#define   WINDOWSPC             0
#endif

#define   FONTQUALITY           NONANTIALIASED_QUALITY

#if (WINDOWSPC>0)
#if _DEBUG
// leak checking
#define CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#endif

#if (GNAV)
#define DISABLEAUDIOVARIO
// use exception handling
#ifndef ALTAIRPROTOTYPE
#define HAVEEXCEPTIONS
#endif
// disable internally generated sounds
#define DISABLEAUDIO
#else
#define BIGDISPLAY
#endif


#ifdef BIGDISPLAY
#define IBLSCALE(x) ((x)*InfoBoxLayout::scale)
#else
#define IBLSCALE(x) (x)
#endif

#endif
