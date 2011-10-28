/* Copyright_License {

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

#include "Logger/LoggerGRecord.hpp"
#include "Logger/MD5.hpp"
#include "IO/FileSource.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/TextWriter.hpp"

#include <tchar.h>
#include <string.h>

const TCHAR *
GRecord::GetVersion() const
{
  return _T("Version 1.0.2");
}

void
GRecord::Initialize()
{  // key #1 used w/ Vali 1.0.0
   // key #2 used w/ Vali 1.0.2
  return Init(2);  // OLC uses key #2 since 9/1/2008
}

/**
 * @return returns true if record is appended, false if skipped
 */
bool
GRecord::AppendRecordToBuffer(const char *record)
{
  const unsigned char *szIn = (const unsigned char *)record;

  if (!IncludeRecordInGCalc(szIn))
    return false;

  AppendStringToBuffer(szIn);
  return true;
}

void
GRecord::AppendStringToBuffer(const unsigned char * szIn)
{
  for (int i = 0; i < 4; i++) {
    md5[i].AppendString(szIn, 1); // skip whitespace flag=1
  }
}

void
GRecord::FinalizeBuffer()
{
  for (int i = 0; i < 4; i++) {
    md5[i].Finalize();
  }
}

void
GRecord::GetDigest(char *szOutput)
{
  for (int idig=0; idig <=3; idig++) {
    md5[idig].GetDigest(szOutput + idig * 32);
  }

  szOutput[128]='\0';
}


void
GRecord::Init(int iKey)
{

  unsigned int i=0;
  for (i=0; i< BUFF_LEN; i++) {
    filename[i]=0;
  }

  for (i=0; i < 3; i++) {
    md5[i].InitDigest();
  }

  switch ( iKey) // 4 different 512 bit keys
  {
  case 1: // key 1
    md5[0].InitKey(0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476);
    md5[1].InitKey(0x48327203, 0x3948ebea, 0x9a9b9c9e, 0xb3bed89a);

    md5[2].InitKey(0x67452301, 0xefcdab89,  0x98badcfe, 0x10325476);
    md5[3].InitKey( 0xc8e899e8, 0x9321c28a, 0x438eba12, 0x8cbe0aee);
    break;

  case 2: // key 2

    md5[0].InitKey(0x1C80A301,0x9EB30b89,0x39CB2Afe,0x0D0FEA76);
    md5[1].InitKey(0x48327203,0x3948ebea,0x9a9b9c9e,0xb3bed89a);

    md5[2].InitKey(0x67452301,0xefcdab89,0x98badcfe,0x10325476);
    md5[3].InitKey(0xc8e899e8,0x9321c28a,0x438eba12,0x8cbe0aee);
    break;

  case 3: // key 3

    md5[0].InitKey(0x7894abde,0x9cb4e90a,0x0bc8f0ea,0x03a9e01a);
    md5[1].InitKey(0x3c4a4c93,0x9cbf7ae3,0xa9bcd0ea,0x9a8c2aaa);

    md5[2].InitKey(0x3c9ae1f1,0x9fe02a1f,0x3fc9a497,0x93cad3ef);
    md5[3].InitKey(0x41a0c8e8,0xf0e37acf,0xd8bcabe2,0x9bed015a);
    break;

  default:  // key 1
    md5[0].InitKey(0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476);
    md5[1].InitKey(0x48327203, 0x3948ebea, 0x9a9b9c9e, 0xb3bed89a);

    md5[2].InitKey(0x67452301, 0xefcdab89,  0x98badcfe, 0x10325476);
    md5[3].InitKey( 0xc8e899e8, 0x9321c28a, 0x438eba12, 0x8cbe0aee);
    break;

  }
}

void
GRecord::SetFileName(const TCHAR *szFileNameIn)
{
  _tcscpy(filename,szFileNameIn);
}

bool GRecord::IncludeRecordInGCalc(const unsigned char *szIn)
{ //returns false if record is not to be included in G record calc (see IGC specs)
  bool bValid;
  TCHAR c1;

  bValid=false;
  c1=szIn[0];
  switch ( c1 )
  {
  case 'L':
    if (memcmp(szIn+1,XCSOAR_IGC_CODE,3) ==0)
      bValid=1; // only include L records made by XCS
    break;

  case 'G':
    break;

  case 'H':
    if ((szIn[1] != 'O') && (szIn[1] != 'P'))
      bValid=1;
    break;

  default:
    bValid=1;
  }
  return bValid;
}

bool
GRecord::LoadFileToBuffer()
{ //loads a file into the data buffer
  FileLineReaderA reader(filename);
  if (reader.error())
    return false;

  char *line;

  while ((line = reader.read()) != NULL)
    AppendRecordToBuffer(line);

  return true;
}



bool
GRecord::AppendGRecordToFile(bool bValid) // writes error if invalid G Record
{
  TextWriter writer(filename, true);
  if (writer.error())
    return false;

  char szDigest[BUFF_LEN];
  GetDigest(szDigest);

  if (bValid) {

    int iLine; //
    int iNumCharsPerLine;
    iNumCharsPerLine=16;
    static char sDig16[BUFF_LEN];
    sDig16[0]='G';
    for ( iLine = 0; iLine < (128/iNumCharsPerLine); iLine++) {// 0 - 15
      for (int iChar = 0; iChar < iNumCharsPerLine; iChar++) {
        sDig16[iChar+1] = szDigest[iChar + iNumCharsPerLine*iLine];
      }

      sDig16[iNumCharsPerLine+1]=0; // +1 is the initial "G"

      writer.writeln(sDig16);
    }
  }
  else {
    static const char sMessage[] = "G Record Invalid";
    writer.writeln(sMessage);
  }

  return true;

}

bool
GRecord::ReadGRecordFromFile(char *szOutput, size_t max_length)
{// returns in szOutput the G Record from the file referenced by FileName member
  FileLineReaderA reader(filename);
  if (reader.error())
    return false;

  unsigned int iLenDigest=0;
  char *data;
  while ((data = reader.read()) != NULL) {
    if (data[0] != 'G')
      continue;

    for (const char *p = data + 1; *p != '\0'; ++p) {
      szOutput[iLenDigest++] = *p;
      if (iLenDigest >= max_length)
        /* G record too large */
        return false;
    }
  } // read

  szOutput[iLenDigest] = '\0';
  return true;
}


bool GRecord::VerifyGRecordInFile()
{ // assumes FileName member is set
  // Load File into Buffer (assume name is already set)
  LoadFileToBuffer();

  // load Existing Digest "old"
  char szOldGRecord[BUFF_LEN];
  if (!ReadGRecordFromFile(szOldGRecord, BUFF_LEN))
    return false;

  // recalculate digest from buffer
  FinalizeBuffer();

  char szNewGRecord[BUFF_LEN];
  GetDigest(szNewGRecord);

  return strcmp(szOldGRecord, szNewGRecord) == 0;
}
