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

#ifndef XCSOAR_DATA_FIELD_FILE_READER_HPP
#define XCSOAR_DATA_FIELD_FILE_READER_HPP

#include "DataField/Base.hpp"

#include <stdlib.h>

#define DFE_MAX_FILES 100

typedef struct {
  TCHAR *mTextFile;
  TCHAR *mTextPathFile;
} DataFieldFileReaderEntry;

class DataFieldFileReader: public DataField {

 private:
  unsigned int nFiles;
  unsigned int mValue;
  DataFieldFileReaderEntry fields[DFE_MAX_FILES];

  public:
  DataFieldFileReader(const TCHAR *EditFormat,
		      const TCHAR *DisplayFormat,
		      void(*OnDataAccess)(DataField *Sender, DataAccessKind_t Mode)):
      DataField(EditFormat, DisplayFormat, OnDataAccess){
      mValue = 0;
      fields[0].mTextFile= NULL;
      fields[0].mTextPathFile= NULL; // first entry always exists and is blank
      nFiles = 1;

      SupportCombo=true;
      (mOnDataAccess)(this, daGet);

    };
    ~DataFieldFileReader() {
	for (unsigned int i=1; i<nFiles; i++) {
	  if (fields[i].mTextFile) {
	    free(fields[i].mTextFile);
	    fields[i].mTextFile= NULL;
	  }
	  if (fields[i].mTextPathFile) {
	    free(fields[i].mTextPathFile);
	    fields[i].mTextPathFile= NULL;
	  }
	}
	nFiles = 1;
    }

  void Inc(void);
  void Dec(void);
  int CreateComboList(void);

  void addFile(const TCHAR *fname, const TCHAR *fpname);
  bool checkFilter(const TCHAR *fname, const TCHAR* filter);
  int GetNumFiles(void);

  int GetAsInteger(void);
  TCHAR *GetAsString(void);
  TCHAR *GetAsDisplayString(void);
  void Lookup(const TCHAR* text);
  TCHAR* GetPathFile(void);

  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif
  void Set(int Value);
  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif
  int SetAsInteger(int Value);

  void Sort();
  void ScanDirectoryTop(const TCHAR *filter);

 protected:
  bool ScanFiles(const TCHAR *pattern, const TCHAR *filter);
  bool ScanDirectories(const TCHAR *pattern, const TCHAR *filter);

};

#endif
