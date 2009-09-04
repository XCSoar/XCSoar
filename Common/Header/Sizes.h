#if !defined(AFX_SIZES_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_SIZES_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#define MIN_WAIT_TIME 100
// min time in ms to wait in main thread loops

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

#define POLARSIZE 3

#define FLARM_MAX_TRAFFIC 15
#define MAXSATELLITES 12

#define DESCRIPTION_SIZE 30
#define TITLE_SIZE 30
#define FORMAT_SIZE 20
#define MAXTASKPOINTS 10
#define MAXSTARTPOINTS 10

#define MAX_LOADSTRING 100

#define TRAILSIZE 1000
// 1000 points at 3.6 seconds average = one hour
#define TRAILSHRINK 5
// short trail is 10 minutes approx

#define GLOBALFONT "Tahoma"
//#define GLOBALFONT "DejaVu Sans Condensed"
//#define GLOBALFONT "HelmetCondensed"

// ratio of height of screen to main font height
#define FONTHEIGHTRATIO 9
// ratio of width of screen to main font width
#define FONTWIDTHRATIO 22

// ratio of title font height to main font height
#define TITLEFONTHEIGHTRATIO 3.0
// ratio of title font width to main font width
#define TITLEFONTWIDTHRATIO 2.9 //1.8

#define CDIFONTHEIGHTRATIO 0.6
#define CDIFONTWIDTHRATIO 0.75

#define MAPFONTHEIGHTRATIO 0.39
#define MAPFONTWIDTHRATIO 0.39

#define CONTROLHEIGHTRATIO 7.4
#define TITLEHEIGHTRATIO 3.1

#define STATISTICSFONTHEIGHTRATIO 0.7
#define STATISTICSFONTWIDTHRATIO 0.7

#define MENUBUTTONWIDTHRATIO 0.6


//////////////

// size of terrain cache
#ifdef WINDOWSPC
#define MAXTERRAINCACHE 8192*2
#else
#define MAXTERRAINCACHE 4096
#endif

// stepsize of pixel grid, should be multiple of 2
#ifdef WINDOWSPC
#define DTQUANT 6
#else
#define DTQUANT 6
#endif

// ratio of smoothed bitmap size to pixel grid
#define OVS 2

// number of radials to do range footprint calculation on
#define NUMTERRAINSWEEPS 20

// number of points along final glide to scan for terrain
#define NUMFINALGLIDETERRAIN 30

// ratio of border size to trigger shape cache reload
#define BORDERFACTOR 1.4

// maximum number of topologies
#define MAXTOPOLOGY 20

// timeout in quarter seconds of infobox focus
#define FOCUSTIMEOUTMAX 24*4

// timeout in quarter seconds of menu button
#define MENUTIMEOUTMAX 8*4

// timeout of display/battery mode in quarter seconds
#define DISPLAYTIMEOUTMAX 60*4

// invalid value for terrain
#define TERRAIN_INVALID -1000

///////////////////////

#define NUMAIRSPACECOLORS 16
#define NUMAIRSPACEBRUSHES 8

#define AIRSPACE_SCANSIZE_X 16
#define AIRSPACE_SCANSIZE_H 16
#define OUTSIDE_CHECK_INTERVAL 4

////////////////////////

#define NUMBUTTONLABELS 16

#define READLINE_LENGTH 300

// Size of Status message cache - Note 1000 messages may not be enough...
// TODO If we continue with the reading one at a time - then consider using
// a pointer structure and build on the fly, thus no limit, but also only
// RAM used as required - easy to do with struct above - just point to next.
// (NOTE: This is used for all the caches for now - temporary)
#define MAXSTATUSMESSAGECACHE 1000

#define MAXISOLINES 32

#define ERROR_TIME 1.0e6

// used by RasterRASP
#define MAX_WEATHER_MAP 16
#define MAX_WEATHER_TIMES 48

// used by map window
#define WPCIRCLESIZE        2

////

enum {
  MAX_IGC_BUFF = 255,
};

#define SCALELISTSIZE  30
#define MAXLABELBLOCKS 100

#endif
