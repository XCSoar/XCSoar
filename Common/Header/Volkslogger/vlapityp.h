/***********************************************************************
**
**   vlapityp.h
**
**   This file is part of libkfrgcs.
**
************************************************************************
**
**   Copyright (c):  2002 by Garrecht Ingenieurgesellschaft
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

/*

  This is the platform-dependent definition of types used in VLAPI.CPP and
  VLAPI.H.

  The present definitions are appropriate for the Borland C++ v3.1 compiler.

  It requires modification under certain circumstances, e.g. when the
  compiler used has other word sizes, other memory models etc.

  The "huge"-pointer is for "limited"-memory compilers, like MS-DOS
  without DOS-extender;
  when a large flat memory model is used, this will probably
  not be necessary.


*/

#ifndef VLAPITYP_H
#define VLAPITYP_H

#ifdef VLAPI2_EXPORTS
#define VLAPI2_API __declspec(dllexport)
#else
#define VLAPI2_API __declspec(dllimport)
#endif

typedef unsigned char
	boolean;


//typedef int
//	error;

// 8-bit unsigned
typedef unsigned char
  byte;


// 16-bit unsigned
typedef unsigned int
  word;


// 32-bit signed
typedef long
	int32;


// 16-bit signed
typedef int
	int16;


// under 16-bit Borland C++ for DOS, pointers which point to data structures
// greater than 64KB need the prefix "huge" to work correctly.
// comment this out and uncomment the empty defintion
// for other compilers which don't need the "huge" prefix

#ifdef FAR
#undef FAR
#endif


//#ifdef __MSDOS__
//#ifdef __BORLANDC__
//#define HUGE huge
//#define FAR far
//#else
//#define HUGE
//#define FAR
//#endif
//#else
//#define HUGE
//#define FAR
//#endif

// pointer to a byte variable, which can be anywhere in the memory space
typedef byte *
  lpb;

#endif
