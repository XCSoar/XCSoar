/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "FileReader.hpp"
#include "ComboList.hpp"
#include "LocalPath.hpp"
#include "Util/StringUtil.hpp"
#include "Compatibility/string.h"
#include "OS/PathName.hpp"
#include "OS/FileUtil.hpp"

#if defined(_WIN32_WCE) && !defined(GNAV)
#include "OS/FlashCardEnumerator.hpp"
#endif

#include <windef.h> /* for MAX_PATH */
#include <assert.h>
#include <stdlib.h>

#ifndef WIN32
#define _cdecl
#endif

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

class FileVisitor: public File::Visitor
{
private:
  DataFieldFileReader &datafield;

public:
  FileVisitor(DataFieldFileReader &_datafield) : datafield(_datafield) {}

  void
  Visit(const TCHAR* path, const TCHAR* filename)
  {
    if (!IsInternalFile(filename))
      datafield.AddFile(filename, path);
  }
};

DataFieldFileReader::Item::~Item()
{
  free(path);
}

DataFieldFileReader::DataFieldFileReader(DataAccessCallback OnDataAccess)
  :DataField(Type::FILE, true, OnDataAccess),
   // Set selection to zero
   mValue(0),
   loaded(false), postponed_sort(false),
   postponed_value(_T("")) {}

int
DataFieldFileReader::GetAsInteger() const
{
  if (!postponed_value.empty())
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
    if (!postponed_patterns.full() &&
        _tcslen(filter) < PatternList::value_type::MAX_SIZE) {
      postponed_patterns.append() = filter;
      return;
    } else
      EnsureLoaded();
  }

  FileVisitor fv(*this);
  VisitDataFiles(filter, fv);

  Sort();
}

void
DataFieldFileReader::Lookup(const TCHAR *Text)
{
  if (!loaded) {
    if (_tcslen(Text) < postponed_value.MAX_SIZE) {
      postponed_value = Text;
      return;
    } else
      EnsureLoaded();
  }

  mValue = 0;
  // Iterate through the filelist
  for (unsigned i = 1; i < files.size(); i++) {
    // If Text == pathfile
    if (_tcscmp(Text, files[i].path) == 0) {
      // -> set selection to current element
      mValue = i;
    }
  }
}

int
DataFieldFileReader::GetNumFiles() const
{
  EnsureLoadedDeconst();

  return files.size();
}

const TCHAR *
DataFieldFileReader::GetPathFile() const
{
  if (!loaded)
    return postponed_value;

  if (mValue >= files.size())
    return _T("");

  const TCHAR *path = files[mValue].path;
  return path != NULL ? path : _T("");
}

void
DataFieldFileReader::AddFile(const TCHAR *filename, const TCHAR *path)
{
  assert(loaded);

  // TODO enhancement: remove duplicates?

  // if too many files -> cancel
  if (files.full())
    return;

  Item &item = files.append();
  item.path = _tcsdup(path);
  item.filename = BaseName(item.path);
  if (item.filename == NULL)
    item.filename = item.path;
}

void
DataFieldFileReader::AddNull()
{
  assert(!files.full());

  files.append();
}

const TCHAR *
DataFieldFileReader::GetAsString() const
{
  if (!loaded)
    return postponed_value;

  if (mValue < files.size())
    return files[mValue].path;
  else
    return _T("");
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
    return files[mValue].filename;
  else
    return _T("");
}

void
DataFieldFileReader::Set(int Value)
{
  if (Value > 0)
    EnsureLoaded();
  else
    postponed_value.clear();

  if ((unsigned)Value < files.size()) {
    mValue = Value;
    Modified();
  }
}

void
DataFieldFileReader::Inc()
{
  EnsureLoaded();

  if (mValue < files.size() - 1) {
    mValue++;
    Modified();
  }
}

void
DataFieldFileReader::Dec()
{
  if (mValue > 0) {
    mValue--;
    Modified();
  }
}

static int _cdecl
DataFieldFileReaderCompare(const void *elem1, const void *elem2)
{
  if (((const DataFieldFileReader::Item *)elem1)->filename == NULL)
    return -1;

  if (((const DataFieldFileReader::Item *)elem2)->filename == NULL)
    return 1;

  // Compare by filename
  return _tcscmp(((const DataFieldFileReader::Item *)elem1)->filename,
                 ((const DataFieldFileReader::Item *)elem2)->filename);
}

void
DataFieldFileReader::Sort()
{
  if (!loaded) {
    postponed_sort = true;
    return;
  }

  // Sort the filelist (except for the first (empty) element)
  qsort(files.begin(), files.size(), sizeof(Item), DataFieldFileReaderCompare);
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
    const TCHAR *path = files[i].filename;
    if (path == NULL)
      path = _T("");

    /* is a file with the same base name present in another data
       directory? */

    bool found = false;
    for (unsigned j = 1; j < files.size(); j++) {
      if (j != i && _tcscmp(path, files[j].filename) == 0) {
        found = true;
        break;
      }
    }

    if (found) {
      /* yes - append the absolute path to allow the user to see the
         difference */
      _tcscpy(buffer, path);
      _tcscat(buffer, _T(" ("));
      _tcscat(buffer, files[i].path);
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
DataFieldFileReader::GetItem(unsigned index) const
{
  EnsureLoadedDeconst();

  return files[index].path;
}

void
DataFieldFileReader::EnsureLoaded()
{
  if (loaded)
    return;

  loaded = true;

  for (auto i = postponed_patterns.begin(), end = postponed_patterns.end();
       i != end; ++i)
    ScanDirectoryTop(*i);

  if (postponed_sort)
    Sort();

  if (!StringIsEmpty(postponed_value))
    Lookup(postponed_value);
}
