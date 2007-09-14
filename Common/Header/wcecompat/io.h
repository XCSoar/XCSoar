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


#ifndef __wcecompat__IO_H__
#define __wcecompat__IO_H__


#ifdef __cplusplus
extern "C" {
#endif


int __cdecl _access(const char *pathname, int mode);
int __cdecl _open(const char* filename, int flags, int mode);
int __cdecl _wopen(const unsigned short* filename, int flags, int mode);
int __cdecl _close(int fd);
long __cdecl _lseek(int fd, long offset, int whence);
int __cdecl _read(int fd, void *buffer, unsigned int count);
int __cdecl _write(int fd, const void *buffer, unsigned int count);
int __cdecl _unlink(const char *pathname);
#define access _access
#define open _open
#define wopen _wopen
#define close _close
#define lseek _lseek
#define read _read
#define write _write
#define unlink _unlink


#ifdef __cplusplus
}
#endif


#endif // __wcecompat__IO_H__
