/***********************************************************************
**
**   vla_support.cpp
**
**   This file is part of libkfrgcs.
**
************************************************************************
**
**   Copyright (c):  2002 by Heiner Lamprecht,
**                           Garrecht Ingenieurgesellschaft
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#ifndef VLA_SUPPORT
#define VLA_SUPPORT

#include "vlapityp.h"
#include "vlapierr.h"

#include <stddef.h>

class Port;
class OperationEnvironment;

/*
	This class contains target system independent base functions
	for communication with the VOLKSLOGGER.
	These functions are critical and normally don't need to be changed
	by the user. If you think that something has to be changed here,
	contact GARRECHT for comments on your intended changes.
*/
class VLA_XFR {
protected:
  Port &port;
  OperationEnvironment &env;

  int32 databaud; // Baudrate as integer (e.g. 115200)

public:
  VLA_XFR(Port &port, unsigned _databaud, OperationEnvironment &env);
};

#endif
