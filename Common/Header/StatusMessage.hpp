#ifndef XCSOAR_STATUS_MESSAGE_HPP
#define XCSOAR_STATUS_MESSAGE_HPP

#include <tchar.h>

typedef struct {
	const TCHAR *key;		/* English key */
	const TCHAR *sound;		/* What sound entry to play */
	const TCHAR *nmea_gps;		/* NMEA Sentence - to GPS serial */
	const TCHAR *nmea_vario;		/* NMEA Sentence - to Vario serial */
	bool doStatus;
	bool doSound;
	int delay_ms;		/* Delay for DoStatusMessage */
	int iFontHeightRatio;	// TODO - not yet used
	bool docenter;		// TODO - not yet used
	int *TabStops;		// TODO - not yet used
	int disabled;		/* Disabled - currently during run time */
} StatusMessageSTRUCT;

extern StatusMessageSTRUCT StatusMessageData[];
extern int StatusMessageData_Size;

void ReadStatusFile(void);
void StatusFileInit(void);
void _init_Status(int num);

#endif
