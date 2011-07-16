/***********************************************************************
 **
 **   vlapisys_linux.cpp
 **
 **   This file is part of libkfrgcs.
 **
 ************************************************************************
 **
 **   Copyright (c):  2002 by Heiner Lamprecht
 **
 **   This file is distributed under the terms of the General Public
 **   Licence. See the file COPYING for more information.
 **
 **   $Id$
 **
 ***********************************************************************/

#include "vla_support.h"
#include "Device/Port.hpp"

#include <stdio.h>
#if defined(HAVE_POSIX) || !defined(_WIN32_WCE)
#include <time.h>
#endif

/***********************************************************************
 *
 * vlapi_sys
 *
 **********************************************************************/


/** serial output of single character to the VL */
VLA_ERROR VLA_SYS::serial_out(const byte outbyte)
{
  port->Write(outbyte);
  return VLA_ERR_NOERR;
}


/**
 * serial input of single character from the VL
 * returns 0 if character has been received, and -1 when no character
 * was in the receive buffer
 */
VLA_ERROR VLA_SYS::serial_in(byte *inbyte)
{
  int i = port->GetChar();
  if (i != EOF) {
    *inbyte = i;
    return VLA_ERR_NOERR;
  } else {
    return VLA_ERR_NOCHAR;
  }
}


/** clear serial input- and output-buffers */
VLA_ERROR VLA_SYS::serial_empty_io_buffers()
{
  port->Flush();
  return VLA_ERR_NOERR;
}
