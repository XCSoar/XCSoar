/*  wcecompat: Windows CE C Runtime Library "compatibility" library.
 *
 *  Copyright (C) 2001-2002 Essemer Pty Ltd.  All rights reserved.
 *  http://www.essemer.com.au/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <string.h>
#include <stdio.h>

char* messages[] = {
/*0           */    "No error",
/*EPERM       */    "Operation not permitted",
/*ENOENT      */    "No such file or directory",
/*ESRCH       */    "No such process",
/*EINTR       */    "Interrupted system call",
/*EIO         */    "I/O error",
/*ENXIO       */    "No such device or address",
/*E2BIG       */    "Arg list too long",
/*ENOEXEC     */    "Exec format error",
/*EBADF       */    "Bad file descriptor",
/*ECHILD      */    "No child processes",
/*EAGAIN      */    "Resource temporarily unavailable",
/*ENOMEM      */    "Not enough memory",
/*EACCES      */    "Permission denied",
/*EFAULT      */    "Bad address",
/*15          */    "Unknown error",						// ENOTBLK "Block device required"
/*EBUSY       */    "Device or resource busy",
/*EEXIST      */    "File exists",
/*EXDEV       */    "Improper link",						//         "Cross-device link"
/*ENODEV      */    "No such device",
/*ENOTDIR     */    "Not a directory",
/*EISDIR      */    "Is a directory",
/*EINVAL      */    "Invalid argument",
/*ENFILE      */    "Too many open files in system",
/*EMFILE      */    "Too many open files",
/*ENOTTY      */    "Inappropriate I/O control operation",	//         "Not a character device"
/*26          */    "Unknown error",						// ETXTBSY "Text file busy"
/*EFBIG       */    "File too large",
/*ENOSPC      */    "No space left on device",
/*ESPIPE      */    "Invalid seek",							//         "Illegal seek"
/*EROFS       */    "Read-only file system",
/*EMLINK      */    "Too many links",
/*EPIPE       */    "Broken pipe",
/*EDOM        */    "Domain error",							//         "Math arg out of domain of func"
/*ERANGE      */    "Result too large",						//         "Math result out of range"
/*35          */    "Unknown error",						// ENOMSG  "No message of desired type"
/*EDEADLK     */    "Resource deadlock avoided",			// EIDRM   "Identifier removed"
/*37          */    "Unknown error",						// ECHRNG  "Channel number out of range"
/*ENAMETOOLONG*/    "Filename too long",
/*ENOLCK      */    "No locks available",
/*ENOSYS      */    "Function not implemented",
/*ENOTEMPTY   */    "Directory not empty",
/*EILSEQ      */    "Illegal byte sequence"
};
const int NUM_MESSAGES = sizeof(messages)/sizeof(messages[0]);


char* strerror(int errnum)
{
	if (errnum < NUM_MESSAGES)
		return messages[errnum];
	return "Unknown error";
}
