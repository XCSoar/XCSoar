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
 **   $Id: vlapisys_win.cpp,v 1.2 2008/08/23 05:59:04 jwharington Exp $
 **
 ***********************************************************************/

#include "Volkslogger/vla_support.h"
extern void StepProgressDialog(void);
extern BOOL SetProgressStepSize(int nSize);

#include <stdio.h>
#if (WINDOWSPC>0)
#include <time.h>
#endif

int noninteractive=1;

/***********************************************************************
 *
 * vlapi_sys
 *
 **********************************************************************/


/** wait a specified amount of milliseconds (t) */
void VLA_SYS::wait_ms(const int32 t)  {  
  if (t>0) {
    Sleep(t);
  }
}

/** read value of a continous running seconds-timer */
int32 VLA_SYS::get_timer_s()  {  
#if (WINDOWSPC>0)
  return time(NULL);  
#else
  return GetTickCount()/1000;
#endif
}

static unsigned long lLastBaudrate = 0;

/**
 * acquire serial port for communication with VL
 * returns 0 if port was successfully opened
 * otherwise != 0
 */
VLA_ERROR VLA_SYS::serial_open_port()
{

  device->Com->StopRxThread();    // JMW
  device->Com->SetRxTimeout(500); // set RX timeout to 500 [ms]

  ////////////////////////////////////////////////////////////////////////
  //
  // port-configuration
  //

  lLastBaudrate = device->Com->SetBaudrate(9600L); // change to IO
                                                    // Mode baudrate

  SetProgressStepSize(1);

  return VLA_ERR_NOERR;
}


/** release serial port on normal exit */
VLA_ERROR VLA_SYS::serial_close_port()
{

  device->Com->SetBaudrate(lLastBaudrate);            // restore baudrate
  
  device->Com->SetRxTimeout(0);                       // clear timeout
  device->Com->StartRxThread();                       // restart RX thread

  return VLA_ERR_NOERR;
}


/** serial output of single character to the VL */
VLA_ERROR VLA_SYS::serial_out(const byte outbyte)
{
  device->Com->PutChar(outbyte);
  return VLA_ERR_NOERR;
}


/**
 * serial input of single character from the VL
 * returns 0 if character has been received, and -1 when no character
 * was in the receive buffer
 */
VLA_ERROR VLA_SYS::serial_in(byte *inbyte)
{
  int i = device->Com->GetChar();
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
  device->Com->Flush();
  return VLA_ERR_NOERR;
}

/** set communication parameters */
VLA_ERROR VLA_SYS::serial_set_baudrate(const int32 baudrate)
{
  device->Com->SetBaudrate(baudrate);    // change to IO Mode baudrate

  return VLA_ERR_NOERR;
}

// interaction - input functions
//
boolean VLA_SYS::test_user_break()
{
  if(noninteractive)  return 0;

  return 0;
  /*
    if you want the datatransfer to be user-interruptable,
    implement testing a flag or anything which can
    indicate that the user wants to interrupt the datatransfer
  */
}

int16 VLA_SYS::clear_user_break()
{
  //  cerr << "VLA_SYS::clear_user_break()\n";
  if(noninteractive)  return 0;

  return 0;
}

// text for the status line
//
const char *statustext[] = {
  "sending command to FR",
  "command is being processed",
  "datatransfer in progress ... (press <Q> to abort)",
  "writing database & FDF to FR",
  "data transfer interrupted by user - press any key",
  "data transfer unsuccessful, try lower baudrate - press any key",
  "no data was received from FR - press any key",
  "user-interrupt !",
  "please connect the VOLKSLOGGER and press OK",
  "connection established - press OK",
  "connection not established - press OK",
  "intentionally left blank ...",
  "error: command not implemented",
  "error: wrong FR connected",
  "error: no response from FR"
};


void VLA_SYS::progress_reset()
{
  if(noninteractive)  return;
}

void VLA_SYS::progress_set(VLS_TXT_ID txtid)
{
  StepProgressDialog();

  if(noninteractive)  return;
}

void VLA_SYS::show(VLS_TXT_ID txtid)
{
  //JMW  cout << statustext[txtid] << endl;

  if(noninteractive)  return;
  if (txtid == VLS_TXT_NIL)  return;
}


void VLA_SYS::showwait(VLS_TXT_ID txtid)
{
  //JMW  cout << statustext[txtid] << endl;

  if(noninteractive)  return;
}
