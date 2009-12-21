/*
Copyright_License {

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

#include "Screen/shapelib/maperror.h"
#include "Screen/shapelib/mapprimitive.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef NEED_NONBLOCKING_STDERR
#include <fcntl.h>
#endif

#ifndef USE_THREAD

errorObj *msGetErrorObj()
{
    static errorObj ms_error = {MS_NOERR, "", "", NULL};

    return &ms_error;
}
#endif

#ifdef USE_THREAD

typedef struct te_info
{
    struct te_info *next;
    int             thread_id;
    errorObj        ms_error;
} te_info_t;

errorObj *msGetErrorObj()
{
    static te_info_t *error_list = NULL;
    te_info_t *link;
    int        thread_id;
    errorObj   *ret_obj;

    msAcquireLock( TLOCK_ERROROBJ );

    thread_id = msGetThreadId();

    /* find link for this thread */

    for( link = error_list;
         link != NULL && link->thread_id != thread_id
             && link->next != NULL && link->next->thread_id != thread_id;
         link = link->next ) {}

    /* If the target thread link is already at the head of the list were ok */
    if( error_list != NULL && error_list->thread_id == thread_id )
    {
    }

    /* We don't have one ... initialize one. */
    else if( link == NULL || link->next == NULL )
    {
        te_info_t *new_link;
        errorObj   error_obj = { MS_NOERR, "", "", NULL };

        new_link = (te_info_t *) malloc(sizeof(te_info_t));
        new_link->next = error_list;
        new_link->thread_id = thread_id;
        new_link->ms_error = error_obj;

        error_list = new_link;
    }

    /* If the link is not already at the head of the list, promote it */
    else if( link != NULL && link->next != NULL )
    {
        te_info_t *target = link->next;

        link->next = link->next->next;
        target->next = error_list;
        error_list = target;
    }

    ret_obj = &(error_list->ms_error);

    msReleaseLock( TLOCK_ERROROBJ );

    return ret_obj;
}
#endif

/* msInsertErrorObj()
**
** We maintain a chained list of errorObj in which the first errorObj is
** the most recent (i.e. a stack).  msErrorReset() should be used to clear
** the list.
**
** Note that since some code in MapServer will fetch the head of the list and
** keep a handle on it for a while, the head of the chained list is static
** and never changes.
** A new errorObj is always inserted after the head, and only if the
** head of the list already contains some information.  i.e. If the static
** errorObj at the head of the list is empty then it is returned directly,
** otherwise a new object is inserted after the head and the data that was in
** the head is moved to the new errorObj, freeing the head errorObj to receive
** the new error information.
*/
static errorObj *
msInsertErrorObj(void)
{
  errorObj *ms_error;
  ms_error = msGetErrorObj();

  if (ms_error->code != MS_NOERR)
  {
      /* Head of the list already in use, insert a new errorObj after the head
       * and move head contents to this new errorObj, freeing the errorObj
       * for reuse.
       */
      errorObj *new_error;
      new_error = (errorObj *)malloc(sizeof(errorObj));

      /* Note: if malloc() failed then we simply do nothing and the head will
       * be overwritten by the caller... we cannot produce an error here
       * since we are already inside a msSetError() call.
       */
      if (new_error)
      {
          new_error->next = ms_error->next;
          new_error->code = ms_error->code;
          strcpy(new_error->routine, ms_error->routine);
          strcpy(new_error->message, ms_error->message);

          ms_error->next = new_error;
          ms_error->code = MS_NOERR;
          ms_error->routine[0] = '\0';
          ms_error->message[0] = '\0';
      }
  }

  return ms_error;
}

/* msResetErrorList()
**
** Clear the list of error objects.
*/
void msResetErrorList(void)
{
  errorObj *ms_error, *this_error;
  ms_error = msGetErrorObj();

  this_error = ms_error->next;
  while( this_error != NULL)
  {
      errorObj *next_error;

      next_error = this_error->next;
      msFree(this_error);
      this_error = next_error;
  }

  ms_error->next = NULL;
  ms_error->code = MS_NOERR;
  ms_error->routine[0] = '\0';
  ms_error->message[0] = '\0';
}

void msSetError(int code, const char *message_fmt, const char *routine, ...)
{
  errorObj *ms_error = msInsertErrorObj();
  va_list args;

  ms_error->code = code;

  if(!routine)
    strcpy(ms_error->routine, "");
  else
    strncpy(ms_error->routine, routine, ROUTINELENGTH);

  if(!message_fmt)
    strcpy(ms_error->message, "");
  else
  {
    va_start(args, routine);
    vsprintf( ms_error->message, message_fmt, args );
    va_end(args);
  }
}

void msDebug( const char * pszFormat, ... )
{
	  (void)pszFormat;
#ifdef ENABLE_STDERR_DEBUG
    va_list args;
    struct timeval tv;

#ifdef NEED_NONBLOCKING_STDERR
    static char nonblocking_set = 0;
    if (!nonblocking_set)
    {
        fcntl(fileno(stderr), F_SETFL, O_NONBLOCK);
        nonblocking_set = 1;
    }
#endif

    gettimeofday(&tv, NULL);
    fprintf(stderr, "[%s].%ld ", chop(ctime(&(tv.tv_sec))), tv.tv_usec);

    va_start(args, pszFormat);
    vfprintf(stderr, pszFormat, args);
    va_end(args);
#endif
}

