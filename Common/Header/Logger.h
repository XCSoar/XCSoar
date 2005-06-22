#if !defined(AFX_LOGGER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_LOGGER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
void DoLogger(TCHAR *strAssetNumber);
void StartLogger(TCHAR *strAssetNumber);
void LogPoint(double Lattitude, double Longditude, double Altitude);
void AddDeclaration(double Lattitude, double Longditude, TCHAR *ID);
void StartDeclaration(void);
void EndDeclaration(void);
void LoggerHeader(void);

void EW_Strings(double Lattitude, double Longditude, TCHAR *ID);
void EW_Download(TCHAR *strAssetNumber);

#endif


