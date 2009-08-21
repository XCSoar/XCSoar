#ifndef XCSOAR_LOCAL_PATH_HPP
#define XCSOAR_LOCAL_PATH_HPP

#include <tchar.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>

void LocalPath(TCHAR* buf, const TCHAR* file = TEXT(""),
               int loc = CSIDL_PERSONAL);

void LocalPathS(char* buf, const TCHAR* file = TEXT(""),
                int loc = CSIDL_PERSONAL);

void ExpandLocalPath(TCHAR* filein);
void ContractLocalPath(TCHAR* filein);

#endif
