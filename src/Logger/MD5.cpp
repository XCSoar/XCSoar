/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
  Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Logger/MD5.hpp"
#include <memory.h>
#include <tchar.h>
#include <stdio.h>


void
MD5::InitKey(unsigned long h0in,
       unsigned long h1in,
       unsigned long h2in,
       unsigned long h3in)
{
  h0=h0in;
  h1=h1in;
  h2=h2in;
  h3=h3in;
  MessageLenBits=0;
}
void
MD5::InitDigest(void)
{
  memset(buff512bits,0,64);
  memset(w,0,sizeof(unsigned long) * 16);
  memset(digest,0, 16);

  MessageLenBits=0;
  a=0; b=0; c=0; d=0;
  h0=0; h1=0; h2=0; h3=0;
  f=0; g=0;


}

/*
* Version 1.0.0 first posted to OLC 8/23/2008 supressed 0x0D only, and used Key#1
* Version 1.0.2 which uses the correct suppression filter from IGC spec and uses key #2
* This is the version we want to use, but we're reverting to 1.0.0 until OLC can upgrade to 1.0.3
* Validation program 1.0.3 is backwards compatible and reads either 1.0.0 or 1.0.2
*/
int
MD5::IsValidIGCChar(char c) //returns 1 if Valid IGC Char
{//                                  else 0

  // 1.0.2 filtering (and use key #2, 3 or 4 b/c key #1 used by 1.0.0 has a dupe in it

    if ( c >=0x20  && c <= 0x7E &&
    c != 0x0D &&
    c != 0x0A &&
    c != 0x24 &&
    c != 0x2A &&
    c != 0x2C &&
    c != 0x21 &&
    c != 0x5C &&
    c != 0x5E &&
    c != 0x7E
       )
      return 1;
    else
      return 0;
}

void
MD5::AppendString(const unsigned char *szin, int bSkipInvalidIGCCharsFlag) // must be NULL-terminated string!
{

  int iLen=0;
  iLen = strlen((char * )szin);
  int BuffLeftover = (MessageLenBits / 8) % 64;

  MessageLenBits += ((unsigned long)iLen * 8);

  for (int i = 0; i < iLen; i++) {
    if (bSkipInvalidIGCCharsFlag == 1 && !IsValidIGCChar(szin[i]) ) { // skip OD because when saved to file, OD OA comes back as OA only
      MessageLenBits -= 8; //subtract it out of the buffer pointer
    }
    else {
      buff512bits[BuffLeftover++] =szin[i];  //
      if (BuffLeftover * 8 == 512 ) { // we have a full buffer
        Process512(buff512bits);
        BuffLeftover=0; //and reset buffer
      }
    }
  }
}


void
MD5::Finalize(void)
{

  // append "0" bits until message length in bits ? 448 (mod 512)
  int BuffLeftover = (MessageLenBits / 8) % 64;
  // need at least 64 bits (8 bytes) for length bits at end


  if (BuffLeftover < (64 - 8) ) {
    // append "1" bit to end of buffer
    buff512bits[BuffLeftover] = 0x80;

    // pad with 56 - len to get exactly
    for (int i=BuffLeftover+1; i < 64; i++) { // clear out rest of buffer too
      buff512bits[i] = 0;
    }

    // exactly 64 bits left for message size bits

    // ready to append message length
  }
  else { // >= 56 bits already in buffer

    // append "1" bit to end of buffer
    buff512bits[BuffLeftover] = 0x80;

    // fill buffer w/ 0's and process
    for (int i=BuffLeftover+1; i < 64; i++ ) {
      buff512bits[i] = 0;
    }

    Process512(buff512bits);

    // now  load 1st 56 bytes of buffer w/ all 0's,
    for (int i=0; i < 64; i++) {// clear out rest of buffer too
      buff512bits[i] = 0;
    }
    // ready to append message length
  }

  //append bit length (bit, not byte) of unpadded message as 64-bit little-endian integer to message
  // store 8 bytes of length into last 8 bytes of buffer (little endian least sig bytes first
  // 4 bytes of length we store go in bytes 56-59 of buffer, 60-63 are all 0's (already)
  buff512bits[59] = (unsigned char)((MessageLenBits & 0xFF000000) >> 24);
  buff512bits[58] = (unsigned char)((MessageLenBits & 0x00FF0000) >> 16);
  buff512bits[57] = (unsigned char)((MessageLenBits & 0x0000FF00) >> 8);
  buff512bits[56] = (unsigned char)(MessageLenBits & 0x000000FF);

  Process512(buff512bits);

}

int
MD5::GetDigestMaxlen(void)
{
  return 16;
}

void
MD5::Process512(unsigned char * s512in)
{ // assume exactly 512 bytes

//Initialize hash value for this chunk:
  a=h0;
  b=h1;
  c=h2;
  d=h3;

  // copy the 64 chars into the 16 unsigned longs
  for (int j=0; j < 16; j++) {
    w[j] = (((unsigned long)s512in[(j*4)+3]) << 24) |
          (((unsigned long)s512in[(j*4)+2]) << 16) |
          (((unsigned long)s512in[(j*4)+1]) << 8) |
          ((unsigned long)s512in[(j*4)]);
  }
//Main loop:
  for (int i=0; i < 64; i++) {
    if (i <= 15) {
      f = (b & c) | ((~b) & d);
      g = i;
    }
    else if (i <= 31) {
      f = (d & b) | ((~d) & c);
      g = (5*i + 1) % 16;
    }
    else if (i <= 47) {
      f = b ^ c ^ d;
      g = (3 * i + 5) % 16;
    }
    else {
      f = c ^ (b | (~d));
      g = (7 * i) % 16;
    }

    unsigned long temp = d;
    d = c;
    c = b;
    b = b + leftrotate((a + f + k[i] + w[g]) , r[i]);
    a = temp;
  }


  //Add this chunk's hash to result so far:
  h0 = h0 + a;
  h1 = h1 + b;
  h2 = h2 + c;
  h3 = h3 + d;
}



int
MD5::GetDigest(TCHAR * szOut)
{ // extract 4 bytes from each unsigned long

  digest[0] = (unsigned char) (h0 & 0xFF);
  digest[1] = (unsigned char)((h0 >> 8) & 0xFF);
  digest[2] = (unsigned char)((h0 >> 16) & 0xFF);
  digest[3] = (unsigned char)((h0 >> 24) & 0xFF);

  digest[4] = (unsigned char) (h1 & 0xFF);
  digest[5] = (unsigned char)((h1 >> 8) & 0xFF);
  digest[6] = (unsigned char)((h1 >> 16) & 0xFF);
  digest[7] = (unsigned char)((h1 >> 24) & 0xFF);

  digest[8] =  (unsigned char) (h2 & 0xFF);
  digest[9] =  (unsigned char)((h2 >> 8) & 0xFF);
  digest[10] = (unsigned char)((h2 >> 16) & 0xFF);
  digest[11] = (unsigned char)((h2 >> 24) & 0xFF);

  digest[12] = (unsigned char) (h3 & 0xFF);
  digest[13] = (unsigned char)((h3 >> 8) & 0xFF);
  digest[14] = (unsigned char)((h3 >> 16) & 0xFF);
  digest[15] = (unsigned char)((h3 >> 24) & 0xFF);

  char sztemp[8];

  int i;
  for (i = 0; i < 16; i++) {
    sprintf(sztemp,"%02x", digest[i]);
    szOut[i*2+0]= sztemp[0];
    szOut[i*2+1]= sztemp[1];
  }

  return 1;
}

unsigned long
MD5::leftrotate (unsigned long x, unsigned long c)
{
    return (x << c) | (x >> (32-c));
}
