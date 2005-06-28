/*
  XCSoar Glide Computer
  Copyright (C) 2000 - 2004  M Roberts

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
*/
#include "stdafx.h"
#include "Logger.h"
#include "externs.h"
#include "Port.h"
#include <windows.h>
#include <tchar.h>
#include "Utils.h"


static TCHAR szLoggerFileName[MAX_PATH];

int EW_count = 0;


void LogPoint(double Lattitude, double Longditude, double Altitude)
{
  HANDLE hFile;// = INVALID_HANDLE_VALUE; 
  DWORD dwBytesRead;   
	
  SYSTEMTIME st;
  char szBRecord[500];

  int DegLat, DegLon;
  double MinLat, MinLon;
  char NoS, EoW;
	

  DegLat = (int)Lattitude;
  MinLat = Lattitude - DegLat;
  NoS = 'N';
  if(MinLat<0)
    {
      NoS = 'S';
      DegLat *= -1; MinLat *= -1;
    }
  MinLat *= 60;
  MinLat *= 1000;


  DegLon = (int)Longditude ;
  MinLon = Longditude  - DegLon;
  EoW = 'E';
  if(MinLon<0)
    {
      EoW = 'W';
      DegLon *= -1; MinLon *= -1;
    }
  MinLon *=60;
  MinLon *= 1000;

  GetLocalTime(&st);

  hFile = CreateFile(szLoggerFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0); 
	
  sprintf(szBRecord,"B%02d%02d%02d%02d%05.0f%c%03d%05.0f%cA%05d%05d0AA\r\n", st.wHour, st.wMinute, st.wSecond, DegLat, MinLat, NoS, DegLon, MinLon, EoW, (int)Altitude,(int)Altitude);

  SetFilePointer(hFile, 0, NULL, FILE_END); 
  WriteFile(hFile, szBRecord, strlen(szBRecord), &dwBytesRead, NULL); 
	
  CloseHandle(hFile);
}


void StartLogger(TCHAR *strAssetNumber)
{
  SYSTEMTIME st;
  HANDLE hFile;
  int i;

  GetLocalTime(&st);

  for(i=1;i<99;i++)
    {
      wsprintf(szLoggerFileName,TEXT("\\My Documents\\%04d-%02d-%02d-XXX-%02d"),
	       st.wYear, 
	       st.wMonth, 
	       st.wDay, 
	       //		   strAssetNumber[0],strAssetNumber[1],strAssetNumber[2],
	       i);
      _tcscat(szLoggerFileName,TEXT(".IGC"));
		
      hFile = CreateFile(szLoggerFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0); 
      if(hFile!=INVALID_HANDLE_VALUE )
	{
	  CloseHandle(hFile);
	  return;
	}
    }
}


extern TCHAR szRegistryPilotName[];        
extern TCHAR szRegistryAircraftType[];        
extern TCHAR szRegistryAircraftRego[];        


void LoggerHeader(void)
{

  char temp[100];
  HANDLE hFile;
  DWORD dwBytesRead;
  TCHAR PilotName[100];
  TCHAR AircraftType[100];
  TCHAR AircraftRego[100];
  
  hFile = CreateFile(szLoggerFileName, GENERIC_WRITE, 
		     FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 
		     FILE_ATTRIBUTE_NORMAL, 0); 
  
  SetFilePointer(hFile, 0, NULL, FILE_END); 

  GetRegistryString(szRegistryPilotName, PilotName, 100);
  sprintf(temp,"HFPLTPILOT:%S\r\n", PilotName);
  WriteFile(hFile, temp, strlen(temp), &dwBytesRead, NULL); 
  
  GetRegistryString(szRegistryAircraftType, AircraftType, 100);
  sprintf(temp,"HFGTYGLIDERTYPE:%S\r\n", AircraftType);
  WriteFile(hFile, temp, strlen(temp), &dwBytesRead, NULL); 

  GetRegistryString(szRegistryAircraftRego, AircraftRego, 100);
  sprintf(temp,"HFGIDGLIDERID:%S\r\n", AircraftRego);
  WriteFile(hFile, temp, strlen(temp), &dwBytesRead, NULL); 

  CloseHandle(hFile);			

}

void StartDeclaration(void)
{
  char start[] = "C0000000N00000000ETAKEOFF (not defined)\r\n";
  HANDLE hFile;
  DWORD dwBytesRead;

  hFile = CreateFile(szLoggerFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0); 
	
  SetFilePointer(hFile, 0, NULL, FILE_END); 
  WriteFile(hFile, start, strlen(start), &dwBytesRead, NULL); 
	
  CloseHandle(hFile);			
}

void EndDeclaration(void)
{
  char start[] = "C0000000N00000000ELANDING (not defined)\r\n";
  HANDLE hFile;
  DWORD dwBytesRead;

  hFile = CreateFile(szLoggerFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0); 
	
  SetFilePointer(hFile, 0, NULL, FILE_END); 
  WriteFile(hFile, start, strlen(start), &dwBytesRead, NULL); 
	
  CloseHandle(hFile);			
}

void AddDeclaration(double Lattitude, double Longditude, TCHAR *ID)
{
  DWORD dwBytesRead;   
  HANDLE hFile;
	
  SYSTEMTIME st;
  char szCRecord[500];

  char IDString[100];
  int i;
	
  int DegLat, DegLon;
  double MinLat, MinLon;
  char NoS, EoW;
	
  for(i=0;i<(int)_tcslen(ID);i++)
    {
      IDString[i] = (char)ID[i];
    }
  IDString[i] = '\0';

  DegLat = (int)Lattitude;
  MinLat = Lattitude - DegLat;
  NoS = 'N';
  if(MinLat<0)
    {
      NoS = 'S';
      DegLat *= -1; MinLat *= -1;
    }
  MinLat *= 60;
  MinLat *= 1000;


  DegLon = (int)Longditude ;
  MinLon = Longditude  - DegLon;
  EoW = 'E';
  if(MinLon<0)
    {
      EoW = 'W';
      DegLon *= -1; MinLon *= -1;
    }
  MinLon *=60;
  MinLon *= 1000;

  GetLocalTime(&st);

  hFile = CreateFile(szLoggerFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0); 
	
  sprintf(szCRecord,"C%02d%05.0f%c%03d%05.0f%c%s\r\n", DegLat, MinLat, NoS, DegLon, MinLon, EoW, IDString);

  SetFilePointer(hFile, 0, NULL, FILE_END); 
  WriteFile(hFile, szCRecord, strlen(szCRecord), &dwBytesRead, NULL); 
	
  CloseHandle(hFile);			
}


// Function Added to delare task to EW logger
void EW_Strings(double Lattitude, double Longditude, TCHAR *ID)
{
  char EWRecord[100];
  char EWRecord_CS[100];

  char IDString[100];
  int i;
	
  int DegLat, DegLon;
  double MinLat, MinLon;
  char NoS, EoW;

  short EoW_Flag, NoS_Flag, EW_Flags;	
	
	
  for(i=0;i<(int)_tcslen(ID);i++)
    {
      IDString[i] = (char)ID[i];
    }
  IDString[i] = '\0';

  char IDString_trunc[10];
  int j;
	
	
  for(j=0;j<3;j++)
    {
      IDString_trunc[j] = (char)ID[j];
    }
  IDString_trunc[j] = '\0';	
	
	
	
  DegLat = (int)Lattitude;
  MinLat = Lattitude - DegLat;
  NoS = 'N';
  if(MinLat<0)
    {
      NoS = 'S';
      DegLat *= -1; MinLat *= -1;
    }
  MinLat *= 60;
  MinLat *= 1000;


  DegLon = (int)Longditude ;
  MinLon = Longditude  - DegLon;
  EoW = 'E';
  if(MinLon<0)
    {
      EoW = 'W';
      DegLon *= -1; MinLon *= -1;
    }
  MinLon *=60;
  MinLon *= 1000;

  //	Calc E/W and N/S flags

  //	Clear flags
  EoW_Flag = 0;
  NoS_Flag = 0;
  EW_Flags = 0;


  if (EoW == 'W')
    {
      EoW_Flag = 0x08;
    }
  else 
    { 
      EoW_Flag = 0x04;
    }
  if (NoS == 'N')
    {
      NoS_Flag = 0x01;
    }
  else 
    {	
      NoS_Flag = 0x02;
    }
  //  Do the calculation
  EW_Flags = EoW_Flag | NoS_Flag;

  // Temporary buffer to calculate checksum

  sprintf(EWRecord,"#STP0%X%X%X%X202020%02X%02X%04X%02X%04X", EW_count, IDString_trunc[0], IDString_trunc[1],IDString_trunc[2], EW_Flags, DegLat, (int)MinLat/10, DegLon, (int)MinLon/10);

  int l,len, m;
  unsigned char CalcCheckSum = 0;

  for(m=1; m<32; m++)
    {
      CalcCheckSum = CalcCheckSum ^ EWRecord[m];
    }
	
  sprintf(EWRecord_CS,"#STP0%X%X%X%X202020%02X%02X%04X%02X%04X%02X\r\n", EW_count, IDString_trunc[0], IDString_trunc[1],IDString_trunc[2], EW_Flags, DegLat, (int)MinLat/10, DegLon, (int)MinLon/10, CalcCheckSum);
  EW_count = EW_count + 1;

  len = strlen(EWRecord_CS);

  for(l=0;l<(len);l++)
    Port1Write ((BYTE)EWRecord_CS[l]);

}





void DoLogger(TCHAR *strAssetNumber)
{
  TCHAR TaskMessage[1024];
  int i;

  if(LoggerActive)
    {
      if(MessageBox(hWndMapWindow,TEXT("Stop Logger"),TEXT("Stop Logger"),MB_YESNO|MB_ICONQUESTION) == IDYES)
	{
	  LoggerActive = FALSE;
	}
    }
  else
    {
      _tcscpy(TaskMessage,TEXT("Start Logger With Declaration\r\n"));
      for(i=0;i<MAXTASKPOINTS;i++)
	{
	  if(Task[i].Index == -1)
	    {
	      if(i==0)
		{
		  _tcscat(TaskMessage,TEXT("None"));
		}
	      break;
	    }
	  _tcscat(TaskMessage,WayPointList[ Task[i].Index ].Name);
	  _tcscat(TaskMessage,TEXT("\r\n"));
	}
		
      if(MessageBox(hWndMapWindow,TaskMessage,TEXT("Start Logger"),MB_YESNO|MB_ICONQUESTION) == IDYES)
	{
	  LoggerActive = TRUE;
	  StartLogger(strAssetNumber);
	  LoggerHeader();
	  StartDeclaration();
	  for(i=0;i<MAXTASKPOINTS;i++)
	    {
	      if(Task[i].Index == -1) break;
	      AddDeclaration(WayPointList[Task[i].Index].Lattitude , WayPointList[Task[i].Index].Longditude  , WayPointList[Task[i].Index].Name );
	    }
	  EndDeclaration();
	}
    }
}



/*

HFDTE141203
HFFXA100
HFPLTPILOT:JOHN WHARINGTON
HFGTYGLIDERTYPE:LS 3
HFGIDGLIDERID:VH-WUE
HFDTM100GPSDATUM:WGS84
HFRFWFIRMWAREVERSION:3.6
HFRHWHARDWAREVERSION:3.4
HFFTYFR TYPE:GARRECHT INGENIEURGESELLSCHAFT,VOLKSLOGGER 1.0
HFCIDCOMPETITIONID:WUE
HFCCLCOMPETITIONCLASS:FAI
HFCIDCOMPETITIONID:WUE
HFCCLCOMPETITIONCLASS:15M
*/
