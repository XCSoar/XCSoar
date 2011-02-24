#ifndef XCSOAR_SIZES_H
#define XCSOAR_SIZES_H

#define MINFREESTORAGE 500
// 500 kb must be free for logger to be active this is based on rough
// estimate that a long flight will detailed logging is about 200k,
// and we want to leave a little free.

// max length airspace and waypoint names
#define NAME_SIZE 50

// max length of waypoint comment names
#define COMMENT_SIZE 50

#define REGKEYSIZE 64

#define MAXSATELLITES 12

#define FORMAT_SIZE 20
#define MAXSTARTPOINTS 10

#define MAX_LOADSTRING 100

#define CONTROLHEIGHTRATIO 7.4

// number of points along final glide to scan for terrain
#define NUMFINALGLIDETERRAIN 30

// timeout in quarter seconds of infobox focus
#define FOCUSTIMEOUTMAX 24*4

// timeout in quarter seconds of menu button
#define MENUTIMEOUTMAX 8*4

// timeout of display/battery mode in quarter seconds
#define DISPLAYTIMEOUTMAX 60*4

#define NUMAIRSPACECOLORS 16
#define NUMAIRSPACEBRUSHES 8

// Size of Status message cache - Note 1000 messages may not be enough...
// TODO If we continue with the reading one at a time - then consider using
// a pointer structure and build on the fly, thus no limit, but also only
// RAM used as required - easy to do with struct above - just point to next.
// (NOTE: This is used for all the caches for now - temporary)
#define MAXSTATUSMESSAGECACHE 1000

#define ERROR_TIME 1.0e6

// used by map window
#define WPCIRCLESIZE        2

#endif
