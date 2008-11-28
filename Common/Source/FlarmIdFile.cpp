#include "StdAfx.h"
#include "XCSoar.h"
#ifdef NEWFLARMDB
#include "FlarmIdFile.h"
#include "Utils.h"

FlarmIdFile::FlarmIdFile(void)
{
  //HANDLE hFile;
  TCHAR path[MAX_PATH];

  TCHAR flarmIdFileName[MAX_PATH] = TEXT("\0");

  LocalPath(path);

  wsprintf(flarmIdFileName,
	   TEXT("%s\\%s"),
	   path,
	   TEXT("data.fln"));

  //hFile = CreateFile(flarmIdFileName, GENERIC_READ,
  //	FILE_SHARE_READ, NULL, OPEN_EXISTING,
  //	     FILE_ATTRIBUTE_NORMAL, 0);
  FILE*	hFile = _wfopen(flarmIdFileName, TEXT("rt"));

  TCHAR res[100];
  TCHAR text[50];

  DWORD fileLength;
	
  //GetFileSize(hFile, &fileLength);
  //SetFilePointer(hFile, 7, NULL, FILE_BEGIN) ; 
  fseek (hFile , 0 , SEEK_END);
  fileLength = ftell (hFile);
  fseek (hFile , 7 , SEEK_SET);

  int itemCount = 0;
  while(fileLength - ftell(hFile) > 87)
    {
      FlarmId *flarmId = new FlarmId;

      GetItem(hFile, flarmId);

      flarmIds[flarmId->GetId()] = flarmId;

      itemCount++;
    };

  wsprintf(text,TEXT("%d FlarmNet ids found\n"), itemCount);
  StartupStore(text);

  fclose(hFile);
}

FlarmIdFile::~FlarmIdFile(void)
{
}

void FlarmIdFile::GetItem(HANDLE hFile, FlarmId *flarmId)
{
  GetAsString(hFile, 6, flarmId->id);
  GetAsString(hFile, 21, flarmId->name);
  GetAsString(hFile, 21, flarmId->airfield);
  GetAsString(hFile, 21, flarmId->type);
  GetAsString(hFile, 7, flarmId->reg);
  GetAsString(hFile, 3, flarmId->cn);
  GetAsString(hFile, 7, flarmId->freq);
  //SetFilePointer(hFile, 1, NULL, FILE_CURRENT) ;

  int i = 0;
  int maxSize = sizeof(flarmId->cn) / sizeof(TCHAR);
  while(flarmId->cn[i] != 0 && i < maxSize)
    {
      if (flarmId->cn[i] == 32)
	{
	  flarmId->cn[i] = 0;
	}
      i++;
    }

  fseek((FILE*)hFile, 1, SEEK_CUR);
}



void FlarmIdFile::GetAsString(HANDLE hFile, int charCount, TCHAR *res)
{
  int bytesToRead = charCount * 2;
  char bytes[100];
  //DWORD bytesRead; 

  //ReadFile(hFile, bytes, bytesToRead, &bytesRead, NULL);
  fread(bytes, 1, bytesToRead, (FILE*)hFile);
    	
  TCHAR *curChar = res;
  for (int z = 0; z < bytesToRead; z += 2)
    {
      char tmp[3];
      tmp[0] = bytes[z];
      tmp[1] = bytes[z+1];
      tmp[2] = 0;

      int i;
      sscanf(tmp, "%2x", &i);

      *curChar = (unsigned char)i;
      curChar ++;
        
    }     
  *curChar = 0;
		
}
FlarmId* FlarmIdFile::GetFlarmIdItem(long id)
{	
  FlarmIdMap::iterator iterFind = flarmIds.find(id);
  if( iterFind != flarmIds.end() )
    {
      return flarmIds[id];		
    }
	
  return NULL;
}

FlarmId* FlarmIdFile::GetFlarmIdItem(TCHAR *cn)
{
  FlarmId *itemTemp = NULL;
  FlarmIdMap::iterator iterFind = flarmIds.begin();
  while( iterFind != flarmIds.end() )
    {
      itemTemp = (FlarmId*)(iterFind->second );
      if(wcscmp(itemTemp->cn, cn) == 0)
	{
	  return itemTemp;
	}	
      iterFind++;
    }
	
  return NULL;
}

long FlarmId::GetId() 
{ 
  long res;

  swscanf(id, TEXT("%6x"), &res);

  return res;
};


#endif
