#ifndef OPTIONS_H
#define OPTIONS_H

#define   MONOCHROME_SCREEN     1             // optimize for monochrom screen
#define   EXPERIMENTAL          0             // ????
#define   ALTERNATEWINDVECTOR   1             // winpilot style windverctor (at the airplane)

#define   LOGGDEVICEINSTREAM    0             // log device in stream
#define   LOGGDEVCOMMANDLINE    NULL          // device in-stream logger command line
                                              // ie TEXT("-logA=\\Speicherkarte\\logA.log ""-logB=\\SD Card\\logB.log""")
#define   AIRSPACEUSEBINFILE    0             // use and maintain binary airspace file

// define this to be true for windows PC port
#define   WINDOWSPC             1
        
#define   GAUGEVARIOENABLED     1

#define   FONTQUALITY           NONANTIALIASED_QUALITY
        
#endif
