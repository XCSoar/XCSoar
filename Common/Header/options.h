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

#define DISABLEAUDIOVARIO

#if defined(GNAV)
#define DISABLEAUDIOVARIO
// use exception handling
#ifndef ALTAIRPROTOTYPE
#ifndef __MINGW32__
#define HAVEEXCEPTIONS
#endif
#endif
// disable internally generated sounds
#define DISABLEAUDIO
#else
#ifndef BIGDISPLAY
#define BIGDISPLAY
#endif
#endif


#ifdef BIGDISPLAY
#define IBLSCALE(x) (   (InfoBoxLayout::IntScaleFlag) ? ((x)*InfoBoxLayout::scale) : ((int)((x)*InfoBoxLayout::dscale)))

#else
#define IBLSCALE(x) (x)
#endif

#ifdef __MINGW32__
#if (WINDOWSPC==0)
#define NEWFLARMDB
#endif
#endif

#ifdef PNA
#define BUG_IN_CLIPPING
#define NOLINETO
#endif

#endif
