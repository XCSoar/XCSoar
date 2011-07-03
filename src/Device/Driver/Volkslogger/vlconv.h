/***********************************************************************
**
**   vlconv.h
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

#ifndef VLCONV_H
#define VLCONV_H

#include "vlapityp.h"

#include <stdio.h>
#include <time.h>

/* Untertypen des Haupttyps Variabel */
#define FLDPLT	     0x01
#define FLDPLT1      0x01
#define FLDPLT2      0x02
#define FLDPLT3      0x03
#define FLDPLT4      0x04
#define FLDGTY       0x05
#define FLDGID       0x06
#define FLDCID       0x07
#define FLDCCL       0x08
#define FLDTZN	     0x09

#define FLDNTP       0x10
#define FLDFDT       0x11
#define FLDTID	     0x12
#define FLDTKF       0x20
#define FLDSTA       0x21
#define FLDFIN       0x22
#define FLDLDG       0x23
// Landing gibts nicht
#define FLDTP1       0x31
#define FLDTP2       0x32
#define FLDTP3       0x33
#define FLDTP4       0x34
#define FLDTP5       0x35
#define FLDTP6       0x36
#define FLDTP7       0x37
#define FLDTP8       0x38
#define FLDTP9       0x39
#define FLDTP10      0x3a
#define FLDTP11      0x3b
#define FLDTP12      0x3c

#define FLDHDR       0x50
#define FLDEPEV      0x60
#define FLDETKF      0x61


/*
convert_gcs
  function:
    converts a given flight log from VOLKSLOGGER binary to IGC-format
  input values:
    converter version
    output stream handle
    pointer to binary file buffer
    OO-fillin
    serial-number (reference)
    position of signature in binary file (reference)
  return value:
    length of binary file
*/
int32 convert_gcs(int16, FILE *, lpb, int16, word *, long *);


/*
DIRENTRY
  descriptive information about each single flight log in the VL
*/
struct DIRENTRY {
	unsigned int  serno; // FR serial number
	tm   firsttime; // time of first fix
	tm   lasttime;  // time of last fix
  long recordingtime; // length of trace in seconds
  int	 takeoff;				// set to one if VL has detected a takeoff
											//  (v>10kt) inside this trace
  char filename[15];  // filename as required by IGC
  char pilot[65];					// flight-info, can be enhanced by all
	char competitionid[4];	// headerfields, but these here might be
	char gliderid[8];				// enough
};

struct DIRECTORY {
	int nflights;
	DIRENTRY *flights;
};


//const int MAXDIRENTRY = 100;

int conv_dir(DIRENTRY* flights, lpb dirbuffer, int countonly);


#endif
