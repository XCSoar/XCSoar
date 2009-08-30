/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "StatusMessage.hpp"
#include "Message.h"
#include "XCSoar.h"
#include "Registry.hpp"
#include "UtilsText.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "SettingsUser.hpp"
#include "UtilsSystem.hpp"
#include "Language.hpp"

#include <stdio.h>

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

void _init_Status(int num);


StatusMessageSTRUCT StatusMessageData[MAXSTATUSMESSAGECACHE];
int StatusMessageData_Size = 0;

void StatusFileInit() {
  StartupStore(TEXT("StatusFileInit\n"));

  // DEFAULT - 0 is loaded as default, and assumed to exist
  StatusMessageData[0].key = TEXT("DEFAULT");
  StatusMessageData[0].doStatus = true;
  StatusMessageData[0].doSound = true;
  StatusMessageData[0].sound = TEXT("IDR_WAV_DRIP");
  StatusMessageData_Size=1;
#ifdef VENTA_DEBUG_EVENT // VENTA- longer statusmessage delay in event debug mode
	StatusMessageData[0].delay_ms = 10000;  // 10 s
#else
    StatusMessageData[0].delay_ms = 2500; // 2.5 s
#endif

  // Load up other defaults - allow overwrite in config file
#include "Status_defaults.cpp"

}

void ReadStatusFile() {

  StartupStore(TEXT("Loading status file\n"));

  TCHAR szFile1[MAX_PATH] = TEXT("\0");
  FILE *fp=NULL;

  // Open file from registry
  GetRegistryString(szRegistryStatusFile, szFile1, MAX_PATH);
  ExpandLocalPath(szFile1);

  SetRegistryString(szRegistryStatusFile, TEXT("\0"));

  if (_tcslen(szFile1)>0)
    fp  = _tfopen(szFile1, TEXT("rt"));

  // Unable to open file
  if (fp == NULL)
    return;

  // TODO code: Safer sizes, strings etc - use C++ (can scanf restrict length?)
  TCHAR buffer[2049];	// Buffer for all
  TCHAR key[2049];	// key from scanf
  TCHAR value[2049];	// value from scanf
  int ms;				// Found ms for delay
  const TCHAR **location;	// Where to put the data
  int found;			// Entries found from scanf
  bool some_data;		// Did we find some in the last loop...

  // Init first entry
  _init_Status(StatusMessageData_Size);
  some_data = false;

  /* Read from the file */
  while (
	 (StatusMessageData_Size < MAXSTATUSMESSAGECACHE)
	 && _fgetts(buffer, 2048, fp)
	 && ((found = _stscanf(buffer, TEXT("%[^#=]=%[^\n]\n"), key, value)) != EOF)
	 ) {
    // Check valid line? If not valid, assume next record (primative, but works ok!)
    if ((found != 2) || !key || !value) {

      // Global counter (only if the last entry had some data)
      if (some_data) {
	StatusMessageData_Size++;
	some_data = false;
	_init_Status(StatusMessageData_Size);
      }

    } else {

      location = NULL;

      if (_tcscmp(key, TEXT("key")) == 0) {
	some_data = true;	// Success, we have a real entry
	location = &StatusMessageData[StatusMessageData_Size].key;
      } else if (_tcscmp(key, TEXT("sound")) == 0) {
	StatusMessageData[StatusMessageData_Size].doSound = true;
	location = &StatusMessageData[StatusMessageData_Size].sound;
      } else if (_tcscmp(key, TEXT("delay")) == 0) {
	if (_stscanf(value, TEXT("%d"), &ms) == 1)
	  StatusMessageData[StatusMessageData_Size].delay_ms = ms;
      } else if (_tcscmp(key, TEXT("hide")) == 0) {
	if (_tcscmp(value, TEXT("yes")) == 0)
	  StatusMessageData[StatusMessageData_Size].doStatus = false;
      }

      // Do we have somewhere to put this && is it currently empty ? (prevent lost at startup)
      if (location && (_tcscmp(*location, TEXT("")) == 0)) {
	// TODO code: this picks up memory lost from no entry, but not duplicates - fix.
	if (*location) {
	  // JMW fix memory leak
          free((void*)*location);
	}
	*location = StringMallocParse(value);
      }
    }

  }

  // How many we really got (blank next just in case)
  StatusMessageData_Size++;
  _init_Status(StatusMessageData_Size);

  // file was ok, so save it to registry
  ContractLocalPath(szFile1);
  SetRegistryString(szRegistryStatusFile, szFile1);

  fclose(fp);
}

// Create a blank entry (not actually used)
void _init_Status(int num) {
  StatusMessageData[num].key = TEXT("");
  StatusMessageData[num].doStatus = true;
  StatusMessageData[num].doSound = false;
  StatusMessageData[num].sound = TEXT("");
  StatusMessageData[num].delay_ms = 2500;  // 2.5 s
}


// DoMessage is designed to delegate what to do for a message
// The "what to do" can be defined in a configuration file
// Defaults for each message include:
//	- Text to display (including multiple languages)
//	- Text to display extra - NOT multiple language
//		(eg: If Airspace Warning - what details - airfield name is in data file, already
//		covers multiple languages).
//	- ShowStatusMessage - including font size and delay
//	- Sound to play - What sound to play
//	- Log - Keep the message on the log/history window (goes to log file and history)
//
// TODO code: (need to discuss) Consider moving almost all this functionality into AddMessage ?

void AddStatusMessage(const TCHAR* text, const TCHAR *data) {
  Message::Lock();

  StatusMessageSTRUCT LocalMessage;
  LocalMessage = StatusMessageData[0];

  int i;
  // Search from end of list (allow overwrites by user)
  for (i=StatusMessageData_Size - 1; i>0; i--) {
    if (_tcscmp(text, StatusMessageData[i].key) == 0) {
      LocalMessage = StatusMessageData[i];
      break;
    }
  }

  if (EnableSoundModes && LocalMessage.doSound)
    PlayResource(LocalMessage.sound);

  // TODO code: consider what is a sensible size?
  TCHAR msgcache[1024];
  if (LocalMessage.doStatus) {

    _tcscpy(msgcache, gettext(text));
    if (data != NULL) {
      _tcscat(msgcache, TEXT(" "));
      _tcscat(msgcache, data);
    }

    Message::AddMessage(LocalMessage.delay_ms, 1, msgcache);
  }

  Message::Unlock();
}


void StatusMessageStart(bool first) {
  static int olddelay = 2000;
  if (first) {
    // NOTE: Must show errors AFTER all windows ready
    olddelay = StatusMessageData[0].delay_ms;
    StatusMessageData[0].delay_ms = 20000; // 20 seconds
  } else {
    StatusMessageData[0].delay_ms = olddelay;
  }
}
