
#include "stdafx.h"
#include <shlobj.h>

#include "imgdecmp.h"
#undef GetProcAddress

HRESULT DecompressImageIndirect(DecompressImageInfo *pParams){
	(void)pParams;
  return (TRUE);
}
//BOOL SHGetSpecialFolderPathW(HWND hwndOwner, LPTSTR lpszPath, int nFolder, BOOL fCreate);
FARPROC GetProcAddressW(HMODULE hModule, LPCWSTR lpProcName){
  char sTmp[512];
  wcstombs(sTmp, lpProcName, 512);
  return(GetProcAddress(hModule, sTmp));
}

/*
//WINSHELLAPI BOOL WINAPI SHGetSpecialFolderPathW(HWND hwndOwner, LPWSTR lpszPath, int nFolder, BOOL fCreate){
BOOL SHGetSpecialFolderPathW(HWND hwndOwner, LPTSTR lpszPath, int nFolder, BOOL fCreate){
  static TCHAR buffer[MAX_PATH];
  // todo
  _tcscpy(buffer, TEXT("C:\Dokumente und Einstellungen\sam\Eigene Dateien"));
}
*/
