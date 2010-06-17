#if !defined(AFX_SIZES_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_SIZES_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#define DISTANCE_ROUNDING 10.0
// Rounding of task distances for entry (sector radius etc)
// 10.0 means rounding to 0.1 user units

#define MINFREESTORAGE 500
// 500 kb must be free for logger to be active this is based on rough
// estimate that a long flight will detailed logging is about 200k,
// and we want to leave a little free.

// max length airspace and waypoint names
#define NAME_SIZE 50

#define NUMSNAILCOLORS 15

// max length of waypoint comment names
#define COMMENT_SIZE 50

#define WAY_POINT_ID_SIZE 20

#define MENU_HEIGHT 26

#define MAXINFOWINDOWS 14

#define REGKEYSIZE 64

#define MAXSATELLITES 12

#define DESCRIPTION_SIZE 30
#define TITLE_SIZE 30
#define FORMAT_SIZE 20
#define MAXTASKPOINTS 10
#define MAXSTARTPOINTS 10

#define MAX_LOADSTRING 100

#define CONTROLHEIGHTRATIO 7.4
#define TITLEHEIGHTRATIO 3.1

#define MENUBUTTONWIDTHRATIO 0.6

// stepsize of pixel grid, should be multiple of 2
#ifdef WINDOWSPC
#define DTQUANT 6
#else
#define DTQUANT 6
#endif

// ratio of smoothed bitmap size to pixel grid
#define OVS 2

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

#define AIRSPACE_SCANSIZE_X 16
#define AIRSPACE_SCANSIZE_H 16
#define OUTSIDE_CHECK_INTERVAL 4

#define READLINE_LENGTH 300

// Size of Status message cache - Note 1000 messages may not be enough...
// TODO If we continue with the reading one at a time - then consider using
// a pointer structure and build on the fly, thus no limit, but also only
// RAM used as required - easy to do with struct above - just point to next.
// (NOTE: This is used for all the caches for now - temporary)
#define MAXSTATUSMESSAGECACHE 1000

#define MAXISOLINES 32

#define ERROR_TIME 1.0e6

// used by map window
#define WPCIRCLESIZE        2

enum {
  MAX_IGC_BUFF = 255,
};

#define SCALELISTSIZE  30

#endif
