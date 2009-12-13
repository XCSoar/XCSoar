/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "DataField/FileReader.hpp"
#include "LocalPath.hpp"
#include "StringUtil.hpp"
#include "Compatibility/string.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>

#ifdef HAVE_POSIX
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>
#endif

bool IsDots(const TCHAR* str) {
  if(_tcscmp(str,TEXT(".")) && _tcscmp(str,TEXT(".."))) return false;
  return true;
}

int
DataFieldFileReader::GetAsInteger(void) const
{
  return mValue;
}


int DataFieldFileReader::SetAsInteger(int Value){
  Set(Value);
  return mValue;
}



void DataFieldFileReader::ScanDirectoryTop(const TCHAR* filter) {

#ifdef ALTAIRSYNC
  ScanDirectories(TEXT("\\NOR Flash"),filter);
#else
  TCHAR buffer[MAX_PATH] = TEXT("\0");
  LocalPath(buffer);
  ScanDirectories(buffer,filter);
#ifndef HAVE_POSIX
#ifndef GNAV
#if !defined(WINDOWSPC) && !defined(__GNUC__)
#ifndef OLDPPC
// non altair, (non windowspc e non mingw32) e non ppc2002
  static bool first = true;

  bool bContinue = true;     // If true, continue searching
                             // If false, stop searching.
  HANDLE hFlashCard;         // Search handle for storage cards
  WIN32_FIND_DATA FlashCardTmp; // Structure for storing card
                                      // information temporarily
  TCHAR FlashPath[MAX_PATH];

  hFlashCard = FindFirstFlashCard (&FlashCardTmp);
  if (hFlashCard == INVALID_HANDLE_VALUE) {
    Sort();
    return;
  }
  _stprintf(FlashPath,TEXT("/%s/%S"),FlashCardTmp.cFileName, XCSDATADIR); // VENTA3 CHECK should it be double //??
  ScanDirectories(FlashPath,filter);
  if (first) {
    StartupStore(TEXT("%s\n"), FlashPath);
  }
  while (bContinue) {
      // Search for the next storage card.
      bContinue = FindNextFlashCard (hFlashCard, &FlashCardTmp);
      if (bContinue) {
        _stprintf(FlashPath,TEXT("/%s/%S"),FlashCardTmp.cFileName, XCSDATADIR);
        ScanDirectories(FlashPath,filter);
        if (first) {
          StartupStore(TEXT("%s\n"), FlashPath);
        }
      }
  }
  FindClose (hFlashCard);          // Close the search handle.

  first = false;
#endif
#else // mingw32 and PC
#ifndef WINDOWSPC
// VENTA2-FIVV SDCARD FIX
#ifdef FIVV
  // Scan only XCSoarData in the root directory where the xcsoar.exe is placed!
  // In large SD card this was leading great confusion since .dat files are ALSO
  // used by other software, namely TOMTOM!
  TCHAR tBuffer[MAX_PATH];
  _stprintf(tBuffer,TEXT("%s%S"),gmfpathname(), XCSDATADIR );
  if (_tcscmp(buffer,tBuffer) != 0) {
    ScanDirectories(tBuffer,filter);
  }
#else
 // To Do: RLD appending "XCSoarData" to card names is a "quick fix" for the upcoming stable release 5.2.3? and
 // the better solution involves changing multiple files, and will remove this list altogether
  ScanDirectories(TEXT("\\Carte de stockage\\XCSoarData"),filter);
  ScanDirectories(TEXT("\\Storage Card\\XCSoarData"),filter);
  ScanDirectories(TEXT("\\SD-MMC Card\\XCSoarData"),filter);
  ScanDirectories(TEXT("\\SD Karte\\XCSoarData"),filter);
  ScanDirectories(TEXT("\\CF Karte\\XCSoarData"),filter);
  ScanDirectories(TEXT("\\SD Card\\XCSoarData"),filter);
  ScanDirectories(TEXT("\\CF Card\\XCSoarData"),filter);
  ScanDirectories(TEXT("\\Speicherkarte\\XCSoarData"),filter);
  ScanDirectories(TEXT("\\SDMMC\\XCSoarData"),filter);
#endif // FIVV
#endif // !WINDOWSPC
#endif // MINGW
#endif // NOT OLDPPC
#endif // NOT ALTAIRSYNC
#endif /* !HAVE_POSIX */
  Sort();

}


bool DataFieldFileReader::ScanDirectories(const TCHAR* sPath,
					  const TCHAR* filter) {
#ifdef HAVE_POSIX

  DIR *dir = opendir(sPath);
  if (dir == NULL)
    return false;

  TCHAR FileName[MAX_PATH];
  _tcscpy(FileName, sPath);
  size_t FileNameLength = _tcslen(FileName);
  FileName[FileNameLength++] = '/';

  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL) {
    if (IsDots(ent->d_name))
      continue;

    _tcscpy(FileName + FileNameLength, ent->d_name);

    struct stat st;
    if (stat(FileName, &st) < 0)
      continue;

    if (S_ISDIR(st.st_mode))
      ScanDirectories(FileName, filter);
    else if (S_ISREG(st.st_mode) && fnmatch(filter, ent->d_name, 0))
      addFile(ent->d_name, FileName);
  }

#else /* !HAVE_POSIX */

  HANDLE hFind;  // file handle
  WIN32_FIND_DATA FindFileData;

  TCHAR DirPath[MAX_PATH];
  TCHAR FileName[MAX_PATH];

  if (sPath) {
    _tcscpy(DirPath,sPath);
    _tcscpy(FileName,sPath);
  } else {
    DirPath[0]= 0;
    FileName[0]= 0;
  }

  ScanFiles(FileName, filter);

  _tcscat(DirPath,TEXT("\\"));
  _tcscat(FileName,TEXT("\\*"));

  hFind = FindFirstFile(FileName,&FindFileData); // find the first file
  if(hFind == INVALID_HANDLE_VALUE) {
    return false;
  }
  _tcscpy(FileName,DirPath);

  if(!IsDots(FindFileData.cFileName)) {
    _tcscat(FileName,FindFileData.cFileName);

    if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      // we have found a directory, recurse
      //      if (!IsSystemDirectory(FileName)) {
	if(!ScanDirectories(FileName,filter)) {
	  // none deeper
	}
	//      }
    }
  }
  _tcscpy(FileName,DirPath);

  bool bSearch = true;
  while(bSearch) { // until we finds an entry
    if(FindNextFile(hFind,&FindFileData)) {
      if(IsDots(FindFileData.cFileName)) continue;
      if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
	// we have found a directory, recurse
	_tcscat(FileName,FindFileData.cFileName);
	//	if (!IsSystemDirectory(FileName)) {
	  if(!ScanDirectories(FileName,filter)) {
	    // none deeper
	  }
	  //	}
      }
      _tcscpy(FileName,DirPath);
    }
    else {
      if(GetLastError() == ERROR_NO_MORE_FILES) // no more files there
	bSearch = false;
      else {
	// some error occured, close the handle and return false
	FindClose(hFind);
	return false;
      }
    }
  }
  FindClose(hFind);  // closing file handle

#endif /* !HAVE_POSIX */

  return true;
}

#ifndef HAVE_POSIX
bool DataFieldFileReader::ScanFiles(const TCHAR* sPath,
				    const TCHAR* filter) {
  HANDLE hFind;  // file handle
  WIN32_FIND_DATA FindFileData;

  TCHAR DirPath[MAX_PATH];
  TCHAR FileName[MAX_PATH];

  if (sPath) {
    _tcscpy(DirPath,sPath);
  } else {
    DirPath[0]= 0;
  }
  _tcscat(DirPath,TEXT("\\"));
  _tcscat(DirPath,filter);
  if (sPath) {
    _tcscpy(FileName,sPath);
  } else {
    FileName[0]= 0;
  }
  _tcscat(FileName,TEXT("\\"));

  hFind = FindFirstFile(DirPath,&FindFileData); // find the first file
  if(hFind == INVALID_HANDLE_VALUE) return false;
  _tcscpy(DirPath,FileName);


  // found first one
  if(!IsDots(FindFileData.cFileName)) {
    _tcscat(FileName,FindFileData.cFileName);

    if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      // do nothing
    }
    else {
      // DO SOMETHING WITH FileName
      if (checkFilter(FindFileData.cFileName, filter)) {
	addFile(FindFileData.cFileName, FileName);
      }
    }
    _tcscpy(FileName,DirPath);
  }

  bool bSearch = true;
  while(bSearch) { // until we finds an entry
    if(FindNextFile(hFind,&FindFileData)) {
      if(IsDots(FindFileData.cFileName)) continue;
      _tcscat(FileName,FindFileData.cFileName);

      if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
	// do nothing
      }
      else {
	// DO SOMETHING WITH FileName
	if (checkFilter(FindFileData.cFileName, filter)) {
	  addFile(FindFileData.cFileName, FileName);
	}
      }
      _tcscpy(FileName,DirPath);
    }
    else {
      if(GetLastError() == ERROR_NO_MORE_FILES) // no more files there
	bSearch = false;
      else {
	// some error occured, close the handle and return false
	FindClose(hFind);
	return false;
      }

    }

  }
  FindClose(hFind);  // closing file handle

  return true;
}
#endif /* !HAVE_POSIX */

void DataFieldFileReader::Lookup(const TCHAR *Text) {
  int i=0;
  mValue = 0;
  for (i=1; i<(int)nFiles; i++) {
    if (_tcscmp(Text,fields[i].mTextPathFile)==0) {
      mValue = i;
    }
  }
}

int
 DataFieldFileReader::GetNumFiles(void) const
{
  return nFiles;
}

const TCHAR *
DataFieldFileReader::GetPathFile(void) const
{
  if ((mValue<=nFiles)&&(mValue)) {
    return fields[mValue].mTextPathFile;
  }
  return TEXT("\0");
}

#ifndef HAVE_POSIX /* we use fnmatch() on POSIX */
bool DataFieldFileReader::checkFilter(const TCHAR *filename,
				      const TCHAR *filter) {
  TCHAR *ptr;
  TCHAR upfilter[MAX_PATH];
  // checks if the filename matches the filter exactly

  if (!filter || string_is_empty(filter + 1))
    // invalid or short filter, pass
    return true;

  _tcscpy(upfilter,filter+1);

  // check if trailing part of filter (*.exe => .exe) matches end
  ptr = _tcsstr(filename, upfilter);
  if (ptr) {
    if (_tcslen(ptr)==_tcslen(upfilter)) {
      return true;
    }
  }

  _tcsupr(upfilter);
  ptr = _tcsstr(filename, upfilter);
  if (ptr) {
    if (_tcslen(ptr)==_tcslen(upfilter)) {
      return true;
    }
  }

  return false;
}
#endif /* !HAVE_POSIX */

void DataFieldFileReader::addFile(const TCHAR *Text,
				  const TCHAR *PText) {
  // TODO enhancement: remove duplicates?
  if (nFiles<DFE_MAX_FILES) {
    fields[nFiles].mTextFile = (TCHAR*)malloc((_tcslen(Text)+1)*sizeof(TCHAR));
    _tcscpy(fields[nFiles].mTextFile, Text);

    fields[nFiles].mTextPathFile = (TCHAR*)malloc((_tcslen(PText)+1)*sizeof(TCHAR));
    _tcscpy(fields[nFiles].mTextPathFile, PText);

    nFiles++;
  }
}

const TCHAR *
DataFieldFileReader::GetAsString(void) const
{
  if (mValue<nFiles) {
    return(fields[mValue].mTextFile);
  } else {
    return NULL;
  }
}


void DataFieldFileReader::Set(int Value){
  if (Value<=(int)nFiles) {
    mValue = Value;
  }
  if (Value<0) {
    mValue = 0;
  }
}


void DataFieldFileReader::Inc(void){
  if (mValue<nFiles-1) {
    mValue++;
    (mOnDataAccess)(this, daChange);
  }
}


void DataFieldFileReader::Dec(void){
  if (mValue>0) {
    mValue--;
    (mOnDataAccess)(this, daChange);
  }
}


static int _cdecl DataFieldFileReaderCompare(const void *elem1,
                                             const void *elem2 ){
  return _tcscmp(((DataFieldFileReaderEntry*)elem1)->mTextFile,
                 ((DataFieldFileReaderEntry*)elem2)->mTextFile);
}


void DataFieldFileReader::Sort(void){
  qsort(fields+1, nFiles-1, sizeof(DataFieldFileReaderEntry),
        DataFieldFileReaderCompare);
}

int DataFieldFileReader::CreateComboList(void) {
  unsigned int i=0;
  for (i=0; i < nFiles; i++){
    mComboList.ComboPopupItemList[i] = mComboList.CreateItem(
                                          i,
                                          i,
                                          fields[i].mTextFile,
                                          fields[i].mTextFile);
    if (i == mValue) {
      mComboList.ComboPopupItemSavedIndex=i;
    }
  }
  mComboList.ComboPopupItemCount=i;
  return mComboList.ComboPopupItemCount;
}
