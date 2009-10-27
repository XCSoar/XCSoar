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


#include "Language.hpp"
#include "LocalPath.hpp"
#include "UtilsText.hpp"
#include "LogFile.hpp"
#include "Registry.hpp"
#include "Sizes.h"

typedef struct {
	TCHAR *key;
	TCHAR *text;
} GetTextSTRUCT;

static GetTextSTRUCT GetTextData[MAXSTATUSMESSAGECACHE];
static int GetTextData_Size = 0;


#ifdef DEBUG_TRANSLATIONS
#include <map>
/*

  WriteMissingTranslations - write all missing translations found
  during runtime to a lanuage file in data dir

*/
template<class _Ty>
struct lessTCHAR: public std::binary_function<_Ty, _Ty, bool>
{	// functor for operator<
  bool operator()(const _Ty& _Left, const _Ty& _Right) const
  {	// apply operator< to operands
    return (_tcscmp(_Left, _Right) < 0);
  }
};

std::map<TCHAR*, TCHAR*, lessTCHAR<TCHAR*> > unusedTranslations;

void WriteMissingTranslations() {
  std::map<TCHAR*, TCHAR*, lessTCHAR<TCHAR*> >::iterator
    s=unusedTranslations.begin(),e=unusedTranslations.end();

  TCHAR szFile1[MAX_PATH] = TEXT("%LOCAL_PATH%\\\\localization_todo.xcl\0");
  FILE *fp=NULL;

  ExpandLocalPath(szFile1);
  fp  = _tfopen(szFile1, TEXT("w+"));

  if (fp != NULL) {
    while (s != e) {
      TCHAR* p = (s->second);
      if (p) {
        while (*p) {
          if (*p != _T('\n')) {
            fwprintf(fp, TEXT("%c"), *p);
          } else {
            fwprintf(fp, TEXT("\\n"));
          }
          p++;
        }
        fwprintf(fp, TEXT("=\n"));
      }
      s++;
    }
    fclose(fp);
  }
}

#endif


/*

  gettext - look up a string of text for the current language

  Currently very simple. Looks up the current string and current language
  to find the appropriate string response. On failure will return the string itself.

  NOTES CACHING:
  	- Could load the whole file or part
	- qsort/bsearch good idea
	- cache misses in data structure for future use

   TODO enhancement: Fast search of text strings

*/

const TCHAR* gettext(const TCHAR* text) {
  int i;
  // return if nothing to do
  if (_tcscmp(text, _T("")) == 0) return (const TCHAR*)text;

  //find a translation
  for (i=0; i<GetTextData_Size; i++) {
    if (!text || !GetTextData[i].key) continue;
    if (_tcscmp(text, GetTextData[i].key) == 0)
      return GetTextData[i].text;
  }

  // return untranslated text if no translation is found.
  // Set a breakpoint here to find untranslated strings

#ifdef DEBUG_TRANSLATIONS
  TCHAR *tmp = _tcsdup(text);
  unusedTranslations[tmp] = tmp;
#endif
  return (const TCHAR*)text;
}

void ReadLanguageFile() {
  StartupStore(TEXT("Loading language file\n"));

  TCHAR szFile1[MAX_PATH] = TEXT("\0");
  FILE *fp=NULL;

  // Open file from registry
  GetRegistryString(szRegistryLanguageFile, szFile1, MAX_PATH);
  ExpandLocalPath(szFile1);

  SetRegistryString(szRegistryLanguageFile, TEXT("\0"));

  if (_tcslen(szFile1)==0) {
    // JMW set default language file if none present
    _tcscpy(szFile1,TEXT("default.xcl"));
  }

  fp  = _tfopen(szFile1, TEXT("rt"));

  if (fp == NULL)
    return;

  // TODO code: Safer sizes, strings etc - use C++ (can scanf restrict length?)
  TCHAR buffer[2049];	// key from scanf
  TCHAR key[2049];	// key from scanf
  TCHAR value[2049];	// value from scanf
  int found;            // Entries found from scanf

  /* Read from the file */
  while (
  	 (GetTextData_Size < MAXSTATUSMESSAGECACHE)
	 && _fgetts(buffer, 2048, fp)
	 && ((found = _stscanf(buffer, TEXT("%[^#=]=%[^\r\n][\r\n]"), key, value)) != EOF)
         ) {
    // Check valid line?
    if ((found != 2) || key[0] == 0 || value[0] == 0) continue;

    GetTextData[GetTextData_Size].key = StringMallocParse(key);
    GetTextData[GetTextData_Size].text = StringMallocParse(value);

    // Global counter
    GetTextData_Size++;
  }

  // file was OK, so save registry
  ContractLocalPath(szFile1);
  SetRegistryString(szRegistryLanguageFile, szFile1);

  fclose(fp);
}

