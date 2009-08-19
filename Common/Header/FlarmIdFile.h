#ifndef FLARMIDFILE_H
#define FLARMIDFILE_H

#include <map>
#include <stdio.h>

class FlarmId
{
public:
  TCHAR id[7];          //Id 6 bytes
  TCHAR name[22];        //Name 15 bytes
  TCHAR airfield[22];    //Airfield 4 bytes
  TCHAR type[22];        //Unknown 1 byte
  TCHAR reg[8];         //Reg 7 bytes
  TCHAR cn[4];          //CN 3 bytes
  TCHAR freq[8];        //Freq 6 bytes
  long GetId();
};

typedef FlarmId* FlarmIdptr;
typedef std::map< long, FlarmIdptr > FlarmIdMap;

class FlarmIdFile
{
private:
  FlarmIdMap flarmIds;
  void GetAsString(HANDLE hFile, int charCount, TCHAR *res);
  void GetItem(HANDLE hFile, FlarmId *flarmId);
public:
  FlarmIdFile(void);
  ~FlarmIdFile(void);
  FlarmId* GetFlarmIdItem(long id);
  FlarmId* GetFlarmIdItem(const TCHAR *cn);
};

#endif

