#include "DataField/FileReader.hpp"
#include "LocalPath.hpp"
#include "Compatibility/string.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>

bool IsDots(const TCHAR* str) {
  if(_tcscmp(str,TEXT(".")) && _tcscmp(str,TEXT(".."))) return false;
  return true;
}

int DataFieldFileReader::GetAsInteger(void){
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
#ifndef GNAV
#if (WINDOWSPC<1) && !defined(__MINGW32__)
#ifndef OLDPPC
// non altair, (non windowspc e non mingw32) e non ppc2002
  static bool first = true;

  bool bContinue = true;     // If true, continue searching
                             // If false, stop searching.
  HANDLE hFlashCard;         // Search handle for storage cards
  WIN32_FIND_DATA FlashCardTmp; // Structure for storing card
                                      // information temporarily
  TCHAR FlashPath[MAX_PATH] = TEXT("\0");

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
#if (WINDOWSPC<1)
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
#endif // WINDOWSPC<1
#endif // MINGW
#endif // NOT OLDPPC
#endif // NOT ALTAIRSYNC
  Sort();

}


bool DataFieldFileReader::ScanDirectories(const TCHAR* sPath,
					  const TCHAR* filter) {

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

  return true;
}


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

void DataFieldFileReader::Lookup(const TCHAR *Text) {
  int i=0;
  mValue = 0;
  for (i=1; i<(int)nFiles; i++) {
    if (_tcscmp(Text,fields[i].mTextPathFile)==0) {
      mValue = i;
    }
  }
}

int DataFieldFileReader::GetNumFiles(void) {
  return nFiles;
}

TCHAR* DataFieldFileReader::GetPathFile(void) {
  if ((mValue<=nFiles)&&(mValue)) {
    return fields[mValue].mTextPathFile;
  }
  return (TCHAR*)TEXT("\0");
}


bool DataFieldFileReader::checkFilter(const TCHAR *filename,
				      const TCHAR *filter) {
  TCHAR *ptr;
  TCHAR upfilter[MAX_PATH];
  // checks if the filename matches the filter exactly

  if (!filter || (_tcslen(filter+1)==0)) {
    // invalid or short filter, pass
    return true;
  }

  _tcscpy(upfilter,filter+1);

  // check if trailing part of filter (*.exe => .exe) matches end
  ptr = _tcsstr((TCHAR*)filename, upfilter);
  if (ptr) {
    if (_tcslen(ptr)==_tcslen(upfilter)) {
      return true;
    }
  }

  _tcsupr(upfilter);
  ptr = _tcsstr((TCHAR*)filename, upfilter);
  if (ptr) {
    if (_tcslen(ptr)==_tcslen(upfilter)) {
      return true;
    }
  }

  return false;
}


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


TCHAR *DataFieldFileReader::GetAsString(void){
  if (mValue<nFiles) {
    return(fields[mValue].mTextFile);
  } else {
    return NULL;
  }
}


TCHAR *DataFieldFileReader::GetAsDisplayString(void){
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
