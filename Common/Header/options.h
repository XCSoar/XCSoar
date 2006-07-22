#ifndef OPTIONS_H
#define OPTIONS_H

#define   MONOCHROME_SCREEN     1             // optimize for monochrom screen
#define   EXPERIMENTAL          0             // ????
#define   ALTERNATEWINDVECTOR   1             // winpilot style windverctor (at the airplane)

#define   LOGGDEVICEINSTREAM    0             // log device in stream
#define   LOGGDEVCOMMANDLINE    NULL          // device in-stream logger command line
                                              // ie TEXT("-logA=\\Speicherkarte\\logA.log ""-logB=\\SD Card\\logB.log""")
#define   AIRSPACEUSEBINFILE    0             // use and maintain binary airspace file

#define   NOCIRCLEONSTARTLINE    1             // draw no circle if task start sector is a line
#define   NOCIRCLEONFINISHLINE   1             // draw no circle if task finish sector is a line
#define   EXPERIMENTAL_STARTLINE 1             // bether visible start line

// define this to be true for windows PC port
#if !defined(WINDOWSPC)
#define   WINDOWSPC             0
#endif

#define   GAUGEVARIOENABLED     1

#define   FONTQUALITY           NONANTIALIASED_QUALITY

#if (WINDOWSPC>0)
// leak checking
#define CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#define DISABLEAUDIOVARIO

#if (GNAV)
#define NEWAIRSPACEWARNING 1
// use exception handling
#define HAVEEXCEPTIONS
// disable internally generated sounds
#define DISABLEAUDIO
#else
#define BIGDISPLAY
#if (NEWINFOBOX>0)
#define NEWAIRSPACEWARNING 1
#else
#define NEWAIRSPACEWARNING 0
#endif
#endif


#ifdef BIGDISPLAY
#define IBLSCALE(x) ((x)*InfoBoxLayout::scale)
#else
#define IBLSCALE(x) (x)
#endif


#endif
