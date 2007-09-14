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


#include <io.h>
#include <stdio.h>
#include <errno.h>


int access(const char* pathname, int mode)
{
//	if (fprintf(stderr, "NOT IMPLEMENTED: access(pathname=%s, mode=%d)\n", pathname, mode) <= 0)
//		printf("NOT IMPLEMENTED: access(pathname=%s, mode=%d)\n", pathname, mode);
	errno = -1;
	return -1;
}


int unlink(const char* pathname)
{
//	if (fprintf(stderr, "NOT IMPLEMENTED: unlink(pathname=%s)\n", pathname) <= 0)
//		printf("NOT IMPLEMENTED: unlink(pathname=%s)\n", pathname);
	errno = -1;
	return -1;
}
