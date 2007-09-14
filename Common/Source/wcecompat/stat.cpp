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


#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include "ts_string.h"
#include "internal.h"


int stat(const char* filename, struct stat* st)
{
	if (filename == NULL || st == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	TCHAR	filenameT[1000];
	ts_strcpy(filenameT, filename);
	WIN32_FILE_ATTRIBUTE_DATA	fad;
	if (!GetFileAttributesEx(filenameT, GetFileExInfoStandard, (LPVOID)&fad))
	{
		errno = ENOENT;
		return -1;
	}

	st->st_dev = 0;
	st->st_ino = 0;
	st->st_mode = 0;
	if (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		st->st_mode |= _S_IFDIR;
		st->st_mode |= _S_IEXEC;	// search permission
	}
	else
	{
		st->st_mode |= _S_IFREG;
		if (strlen(filename) >= 4 && _stricmp(filename+strlen(filename)-4, ".exe") == 0)
			st->st_mode |= _S_IEXEC;	// execute permission
	}
	st->st_mode |= _S_IREAD;	// TODO: assuming readable, but this may not be the case
	if (!(fad.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
		st->st_mode |= _S_IWRITE;
	st->st_nlink = 1;	// TODO: NTFS can have links, so get the correct value
	st->st_uid = 0;
	st->st_gid = 0;
	st->st_rdev = 0;
	st->st_size = fad.nFileSizeLow;
        /* JMW don't need this?  TODO
	st->st_atime = w32_filetime_to_time_t(&fad.ftLastAccessTime);
	st->st_mtime = w32_filetime_to_time_t(&fad.ftLastWriteTime);
	st->st_ctime = w32_filetime_to_time_t(&fad.ftCreationTime);
        */

	return 0;
}
