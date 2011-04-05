/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "DataField/ComboList.hpp"
#include "LocalPath.hpp"
#include "StringUtil.hpp"
#include "Compatibility/string.h"
#include "Compatibility/path.h"
#include "OS/PathName.hpp"

#if defined(_WIN32_WCE) && !defined(GNAV)
#include "OS/FlashCardEnumerator.hpp"
#endif

#include <windef.h> /* for MAX_PATH */
#include <assert.h>
#include <stdlib.h>

#ifdef HAVE_POSIX
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>

#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD 0
#endif
#endif

DataFieldFileReader::Item::~Item()
{
  free(mTextPathFile);
}

/**
 * Checks whether the given string str equals "." or ".."
 * @param str The string to check
 * @return True if string equals "." or ".."
 */
static bool
IsDots(const TCHAR* str)
{
  if (_tcscmp(str, _T(".")) && _tcscmp(str, _T("..")))
    return false;
  return true;
}

/**
 * Checks whether the given string str equals a xcsoar internal file's filename
 * @param str The string to check
 * @return True if string equals a xcsoar internal file's filename
 */
static bool
IsInternalFile(const TCHAR* str)
{
  const TCHAR* const ifiles[] = {
    _T("xcsoar-checklist.txt"),
    _T("xcsoar-flarm.txt"),
    _T("xcsoar-marks.txt"),
    _T("xcsoar-persist.log"),
    _T("xcsoar-startup.log"),
    _T("xcsoar-rasp.dat"),
    NULL
  };

  for (unsigned i = 0; ifiles[i] != NULL; i++)
    if (!_tcscmp(str, ifiles[i]))
      return true;

  return false;
}

DataFieldFileReader::DataFieldFileReader(DataAccessCallback_t OnDataAccess)
  :DataField(_T(""), _T(""), OnDataAccess),
   // Set selection to zero
   mValue(0),
   loaded(false), postponed_sort(false), num_postponed_patterns(0)
{
  // Fill first entry -> always exists and is blank
  files.append();

  postponed_value[0] = _T('\0');

  // This type of DataField supports the combolist
  SupportCombo = true;
}

int
DataFieldFileReader::GetAsInteger(void) const
{
  if (!string_is_empty(postponed_value))
    EnsureLoadedDeconst();

  return mValue;
}

void
DataFieldFileReader::SetAsInteger(int Value)
{
  Set(Value);
}

void
DataFieldFileReader::ScanDirectoryTop(const TCHAR* filter)
{
  if (!loaded) {
    if (num_postponed_patterns < sizeof(postponed_patterns) / sizeof(postponed_patterns[0]) &&
        _tcslen(filter) < sizeof(postponed_patterns[0]) / sizeof(postponed_patterns[0][0])) {
      _tcscpy(postponed_patterns[num_postponed_patterns++], filter);
      return;
    } else
      EnsureLoaded();
  }

  const TCHAR *data_path = GetPrimaryDataPath();
  ScanDirectories(data_path, filter);

  {
    TCHAR buffer[MAX_PATH];
    const TCHAR *home_path = GetHomeDataPath(buffer);
    if (home_path != NULL && _tcscmp(data_path, home_path) != 0)
      ScanDirectories(home_path, filter);
  }

#if defined(_WIN32_WCE) && !defined(GNAV)
  TCHAR FlashPath[MAX_PATH];
  FlashCardEnumerator enumerator;
  const TCHAR *name;
  while ((name = enumerator.next()) != NULL) {
    _stprintf(FlashPath, _T(DIR_SEPARATOR_S "%s" DIR_SEPARATOR_S "%s"), name, XCSDATADIR);
    if (_tcscmp(data_path, FlashPath) == 0)
      /* don't scan primary data path twice */
      continue;

    ScanDirectories(FlashPath, filter);
  }
#endif /* _WIN32_WCE && !GNAV*/

  Sort();
}

bool
DataFieldFileReader::ScanDirectories(const TCHAR* sPath, const TCHAR* filter)
{
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
    if (IsInternalFile(ent->d_name))
      continue;

    _tcscpy(FileName + FileNameLength, ent->d_name);

    struct stat st;
    if (stat(FileName, &st) < 0)
      continue;

    if (S_ISDIR(st.st_mode))
      ScanDirectories(FileName, filter);
    else if (S_ISREG(st.st_mode) &&
             fnmatch(filter, ent->d_name, FNM_CASEFOLD) == 0)
      addFile(ent->d_name, FileName);
  }

  closedir(dir);
#else /* !HAVE_POSIX */
  HANDLE hFind; // file handle
  WIN32_FIND_DATA FindFileData;

  TCHAR DirPath[MAX_PATH];
  TCHAR FileName[MAX_PATH];

  if (sPath) {
    _tcscpy(DirPath, sPath);
    _tcscpy(FileName, sPath);
  } else {
    DirPath[0] = 0;
    FileName[0] = 0;
  }

  ScanFiles(FileName, filter);

  _tcscat(DirPath, _T(DIR_SEPARATOR_S));
  _tcscat(FileName, _T(DIR_SEPARATOR_S "*"));

  hFind = FindFirstFile(FileName, &FindFileData); // find the first file
  if (hFind == INVALID_HANDLE_VALUE)
    return false;

  _tcscpy(FileName, DirPath);

  if (!IsDots(FindFileData.cFileName) &&
      !IsInternalFile(FindFileData.cFileName)) {
    _tcscat(FileName, FindFileData.cFileName);

    if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      // we have found a directory, recurse
      //      if (!IsSystemDirectory(FileName)) {
      if (!ScanDirectories(FileName, filter)) {
        // none deeper
      }
      //      }
    }
  }
  _tcscpy(FileName, DirPath);

  bool bSearch = true;
  while (bSearch) { // until we finds an entry
    if (FindNextFile(hFind, &FindFileData)) {
      if (IsDots(FindFileData.cFileName))
        continue;
      if (IsInternalFile(FindFileData.cFileName))
        continue;
      if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        // we have found a directory, recurse
        _tcscat(FileName, FindFileData.cFileName);
        //	if (!IsSystemDirectory(FileName)) {
        if (!ScanDirectories(FileName, filter)) {
          // none deeper
        }
        //	}
      }
      _tcscpy(FileName, DirPath);
    } else {
      if (GetLastError() == ERROR_NO_MORE_FILES) // no more files there
        bSearch = false;
      else {
        // some error occured, close the handle and return false
        FindClose(hFind);
        return false;
      }
    }
  }
  FindClose(hFind); // closing file handle

#endif /* !HAVE_POSIX */

  return true;
}

#ifndef HAVE_POSIX
bool
DataFieldFileReader::ScanFiles(const TCHAR* sPath, const TCHAR* filter)
{
  HANDLE hFind; // file handle
  WIN32_FIND_DATA FindFileData;

  TCHAR DirPath[MAX_PATH];
  TCHAR FileName[MAX_PATH];

  if (sPath)
    _tcscpy(DirPath, sPath);
  else
    DirPath[0] = 0;

  _tcscat(DirPath, _T(DIR_SEPARATOR_S));
  _tcscat(DirPath, filter);

  if (sPath)
    _tcscpy(FileName, sPath);
  else
    FileName[0] = 0;

  _tcscat(FileName, _T(DIR_SEPARATOR_S));

  hFind = FindFirstFile(DirPath, &FindFileData); // find the first file
  if (hFind == INVALID_HANDLE_VALUE)
    return false;

  _tcscpy(DirPath, FileName);

  // found first one
  if (!IsDots(FindFileData.cFileName) &&
      !IsInternalFile(FindFileData.cFileName)) {
    _tcscat(FileName, FindFileData.cFileName);

    if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      // do nothing
    } else {
      // DO SOMETHING WITH FileName
      if (checkFilter(FindFileData.cFileName, filter)) {
        addFile(FindFileData.cFileName, FileName);
      }
    }
    _tcscpy(FileName, DirPath);
  }

  bool bSearch = true;
  while (bSearch) { // until we finds an entry
    if (FindNextFile(hFind, &FindFileData)) {
      if (IsDots(FindFileData.cFileName))
        continue;
      if (IsInternalFile(FindFileData.cFileName))
        continue;
      _tcscat(FileName, FindFileData.cFileName);

      if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        // do nothing
      } else {
        // DO SOMETHING WITH FileName
        if (checkFilter(FindFileData.cFileName, filter)) {
          addFile(FindFileData.cFileName, FileName);
        }
      }
      _tcscpy(FileName, DirPath);
    } else {
      if (GetLastError() == ERROR_NO_MORE_FILES) // no more files there
        bSearch = false;
      else {
        // some error occured, close the handle and return false
        FindClose(hFind);
        return false;
      }
    }
  }
  FindClose(hFind); // closing file handle

  return true;
}
#endif /* !HAVE_POSIX */

void
DataFieldFileReader::Lookup(const TCHAR *Text)
{
  if (!loaded) {
    if (_tcslen(Text) < sizeof(postponed_value) / sizeof(postponed_value[0])) {
      _tcscpy(postponed_value, Text);
      return;
    } else
      EnsureLoaded();
  }

  mValue = 0;
  // Iterate through the filelist
  for (unsigned i = 1; i < files.size(); i++) {
    // If Text == pathfile
    if (_tcscmp(Text, files[i].mTextPathFile) == 0) {
      // -> set selection to current element
      mValue = i;
    }
  }
}

int
DataFieldFileReader::GetNumFiles(void) const
{
  EnsureLoadedDeconst();

  return files.size();
}

const TCHAR *
DataFieldFileReader::GetPathFile(void) const
{
  if (!loaded)
    return postponed_value;

  if (mValue > 0 && mValue <= files.size())
    return files[mValue].mTextPathFile;

  return _T("");
}

#ifndef HAVE_POSIX /* we use fnmatch() on POSIX */
bool
DataFieldFileReader::checkFilter(const TCHAR *filename, const TCHAR *filter)
{
  // filter = e.g. "*.igc" or "config/*.prf"
  // todo: make filters like "config/*.prf" work

  const TCHAR *ptr;
  TCHAR upfilter[MAX_PATH];

  // if invalid or short filter "*" -> return true
  // todo: check for asterisk
  if (!filter || string_is_empty(filter + 1))
    return true;

  // Copy filter without first char into upfilter
  // *.igc         ->  .igc
  // config/*.prf  ->  onfig/*.prf
  _tcscpy(upfilter, filter + 1);

  // Search for upfilter in filename (e.g. ".igc" in "934CFAE1.igc") and
  //   save the position of the first occurence in ptr
  ptr = _tcsstr(filename, upfilter);
  if (ptr) {
    // If upfilter was found
    if (_tcslen(ptr) == _tcslen(upfilter)) {
      // If upfilter was found at the very end of filename
      // -> filename matches filter
      return true;
    }
  }

  // Convert upfilter to uppercase
  _tcsupr(upfilter);

  // And do it all again
  ptr = _tcsstr(filename, upfilter);
  if (ptr) {
    if (_tcslen(ptr) == _tcslen(upfilter)) {
      return true;
    }
  }

  // If still no match found -> filename does not match the filter
  return false;
}
#endif /* !HAVE_POSIX */

void
DataFieldFileReader::addFile(const TCHAR *Text, const TCHAR *PText)
{
  assert(loaded);

  // TODO enhancement: remove duplicates?

  // if too many files -> cancel
  if (files.full())
    return;

  Item &item = files.append();
  item.mTextPathFile = _tcsdup(PText);
  item.mTextFile = BaseName(item.mTextPathFile);
  if (item.mTextFile == NULL)
    item.mTextFile = item.mTextPathFile;
}

const TCHAR *
DataFieldFileReader::GetAsString(void) const
{
  if (!loaded)
    return postponed_value;

  if (mValue < files.size())
    return files[mValue].mTextPathFile;
  else
    return NULL;
}

const TCHAR *
DataFieldFileReader::GetAsDisplayString() const
{
  if (!loaded) {
    /* get basename from postponed_value */
    const TCHAR *p = BaseName(postponed_value);
    if (p == NULL)
      p = postponed_value;

    return p;
  }

  if (mValue < files.size())
    return files[mValue].mTextFile;
  else
    return NULL;
}

void
DataFieldFileReader::Set(int Value)
{
  if (Value > 0)
    EnsureLoaded();
  else
    postponed_value[0] = _T('\0');

  if ((unsigned)Value <= files.size()) {
    mValue = Value;
    (mOnDataAccess)(this, daChange);
  }
}

void
DataFieldFileReader::Inc(void)
{
  EnsureLoaded();

  if (mValue < files.size() - 1) {
    mValue++;
    (mOnDataAccess)(this, daChange);
  }
}

void
DataFieldFileReader::Dec(void)
{
  if (mValue > 0) {
    mValue--;
    (mOnDataAccess)(this, daChange);
  }
}

static int _cdecl
DataFieldFileReaderCompare(const void *elem1, const void *elem2)
{
  // Compare by filename
  return _tcscmp(((const DataFieldFileReader::Item *)elem1)->mTextFile,
                 ((const DataFieldFileReader::Item *)elem2)->mTextFile);
}

void
DataFieldFileReader::Sort(void)
{
  if (!loaded) {
    postponed_sort = true;
    return;
  }

  // Sort the filelist (except for the first (empty) element)
  qsort(files.begin() + 1, files.size() - 1, sizeof(Item),
        DataFieldFileReaderCompare);
  /* by the way, we're not using std::sort() here, because this
     function would require the Item class to be copyable */
}

ComboList *
DataFieldFileReader::CreateComboList() const
{
  /* sorry for the const_cast .. this method keeps the promise of not
     modifying the object, given that one does not count filling the
     (cached) file list as "modification" */
  EnsureLoadedDeconst();

  ComboList *cl = new ComboList();

  TCHAR buffer[MAX_PATH];

  for (unsigned i = 0; i < files.size(); i++) {
    const TCHAR *path = files[i].mTextFile;
    if (path == NULL)
      path = _T("");

    /* is a file with the same base name present in another data
       directory? */

    bool found = false;
    for (unsigned j = 1; j < files.size(); j++) {
      if (j != i && _tcscmp(path, files[j].mTextFile) == 0) {
        found = true;
        break;
      }
    }

    if (found) {
      /* yes - append the absolute path to allow the user to see the
         difference */
      _tcscpy(buffer, path);
      _tcscat(buffer, _T(" ("));
      _tcscat(buffer, files[i].mTextPathFile);
      _tcscat(buffer, _T(")"));
      path = buffer;
    }

    cl->Append(i, path);
    if (i == mValue) {
      cl->ComboPopupItemSavedIndex = i;
    }
  }

  return cl;
}

unsigned
DataFieldFileReader::size() const
{
  EnsureLoadedDeconst();

  return files.size();
}

const TCHAR *
DataFieldFileReader::getItem(unsigned index) const
{
  EnsureLoadedDeconst();

  return files[index].mTextPathFile;
}

void
DataFieldFileReader::EnsureLoaded()
{
  if (loaded)
    return;

  loaded = true;

  for (unsigned i = 0; i < num_postponed_patterns; ++i)
    ScanDirectoryTop(postponed_patterns[i]);

  if (postponed_sort)
    Sort();

  if (!string_is_empty(postponed_value))
    Lookup(postponed_value);
}
