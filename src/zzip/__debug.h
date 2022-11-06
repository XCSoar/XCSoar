/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include <zzip/conf.h>
#include <zzip/__hints.h>

/* perhaps want to show on syslog(3) ?? */

#if 0 && defined(DEBUG)
#include <stdio.h>
#define MSG1(X1) ZZIP_FOR1 { \
        fprintf(stderr,"\n%s:%i:"X1"\n", ZZIP_FUNC,__LINE__ \
	    ); } ZZIP_END1
#define MSG2(X1,X2) ZZIP_FOR1 { \
        fprintf(stderr,"\n%s:%i:"X1"\n", ZZIP_FUNC,__LINE__ \
		,X2);} ZZIP_END1
#define MSG3(X1,X2,X3) ZZIP_FOR1 { \
        fprintf(stderr,"\n%s:%i:"X1"\n", ZZIP_FUNC,__LINE__ \
		 ,X2,X3); } ZZIP_END1
#define MSG4(X1,X2,X3,X4)   ZZIP_FOR1 { \
        fprintf(stderr,"\n%s:%i:"X1"\n", ZZIP_FUNC,__LINE__ \
		 ,X2,X3,X4); } ZZIP_END1
#define MSG5(X1,X2,X3,X4,X5)   ZZIP_FOR1 { \
        fprintf(stderr,"\n%s:%i:"X1"\n", ZZIP_FUNC,__LINE__ \
		 ,X2,X3,X4,X5); } ZZIP_END1
#define MSG6(X1,X2,X3,X4,X5,X6)   ZZIP_FOR1 { \
        fprintf(stderr,"\n%s:%i:"X1"\n", ZZIP_FUNC,__LINE__ \
		 ,X2,X3,X4,X5,X6); } ZZIP_END1

#else
#define MSG1(X1) {}
#define MSG2(X1,X2) {}
#define MSG3(X1,X2,X3) {}
#define MSG4(X1,X2,X3,X4) {}
#define MSG5(X1,X2,X3,X4,X5) {}
#define MSG6(X1,X2,X3,X4,X5,X6) {}
#endif

#define DBG1(X1)                     MSG1("DEBUG: " X1) 
#define DBG2(X1,X2)                  MSG2("DEBUG: " X1,X2) 
#define DBG3(X1,X2,X3)               MSG3("DEBUG: " X1,X2,X3) 
#define DBG4(X1,X2,X3,X4)            MSG4("DEBUG: " X1,X2,X3,X4) 
#define DBG5(X1,X2,X3,X4,X5)         MSG5("DEBUG: " X1,X2,X3,X4,X5) 
#define DBG6(X1,X2,X3,X4,X5,X6)      MSG6("DEBUG: " X1,X2,X3,X4,X5,X6) 

#define HINT1(X1)                     MSG1("HINT: " X1) 
#define HINT2(X1,X2)                  MSG2("HINT: " X1,X2) 
#define HINT3(X1,X2,X3)               MSG3("HINT: " X1,X2,X3) 
#define HINT4(X1,X2,X3,X4)            MSG4("HINT: " X1,X2,X3,X4) 
#define HINT5(X1,X2,X3,X4,X5)         MSG5("HINT: " X1,X2,X3,X4,X5) 
#define HINT6(X1,X2,X3,X4,X5,X6)      MSG6("HINT: " X1,X2,X3,X4,X5,X6) 

#define NOTE1(X1)                     MSG1("NOTE: " X1) 
#define NOTE2(X1,X2)                  MSG2("NOTE: " X1,X2) 
#define NOTE3(X1,X2,X3)               MSG3("NOTE: " X1,X2,X3) 
#define NOTE4(X1,X2,X3,X4)            MSG4("NOTE: " X1,X2,X3,X4) 
#define NOTE5(X1,X2,X3,X4,X5)         MSG5("NOTE: " X1,X2,X3,X4,X5) 
#define NOTE6(X1,X2,X3,X4,X5,X6)      MSG6("NOTE: " X1,X2,X3,X4,X5,X6) 

#define WARN1(X1)                     MSG1("WARN: " X1) 
#define WARN2(X1,X2)                  MSG2("WARN: " X1,X2) 
#define WARN3(X1,X2,X3)               MSG3("WARN: " X1,X2,X3) 
#define WARN4(X1,X2,X3,X4)            MSG4("WARN: " X1,X2,X3,X4) 
#define WARN5(X1,X2,X3,X4,X5)         MSG5("WARN: " X1,X2,X3,X4,X5) 
#define WARN6(X1,X2,X3,X4,X5,X6)      MSG6("WARN: " X1,X2,X3,X4,X5,X6) 

#define FAIL1(X1)                     MSG1("FAIL: " X1) 
#define FAIL2(X1,X2)                  MSG2("FAIL: " X1,X2) 
#define FAIL3(X1,X2,X3)               MSG3("FAIL: " X1,X2,X3) 
#define FAIL4(X1,X2,X3,X4)            MSG4("FAIL: " X1,X2,X3,X4) 
#define FAIL5(X1,X2,X3,X4,X5)         MSG5("FAIL: " X1,X2,X3,X4,X5) 
#define FAIL6(X1,X2,X3,X4,X5,X6)      MSG6("FAIL: " X1,X2,X3,X4,X5,X6) 



#if 0 && defined(DEBUG)
_zzip_inline static void zzip_debug_xbuf (unsigned char* p, int l)
    /* ZZIP_GNUC_UNUSED */
{
#   define q(a) ((a&0x7F)<32?32:(a&0x7F))
    while (l > 0)
    {
        fprintf (stderr, 
		 "%02x %02x %02x %02x  "
		 "%02x %02x %02x %02x  "
		 "%c%c%c%c %c%c%c%c\n",
		 p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
		 q(p[0]), q(p[1]), q(p[2]), q(p[3]), 
		 q(p[4]), q(p[5]), q(p[6]), q(p[7]));
        p += 8; l -= 8;
    }
#   undef q
}
#endif
