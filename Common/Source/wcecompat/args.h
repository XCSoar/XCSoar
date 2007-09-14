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


#ifndef __wcecompat__ARGS_H__
#define __wcecompat__ARGS_H__


#include <windows.h>


// generate argc/argv from program name and lpCmdLine
// returns 0 on failure, or argc on success
int processCmdLine(LPTSTR lpCmdLine, char*** pArgv);


enum RedirType {
	RT_NONE,
	RT_PIPE_UNSPEC,		// unspecified pipe (one of stdin, stdout, stderr, but unknown which it is)
	RT_PIPE_STDIN,
	RT_PIPE_STDOUT,		// 2>&1 >file (stderr is redirected to original stdout, stdout to file)
	RT_PIPE_STDERR,		// >&2 2>file (stdout is redirected to original stderr, stderr to file)
	RT_HANDLE,			// >file 2>&1 (stdout is redirected to file, stderr to same file via stdout)
	RT_FILE,			// >file 2>file <file
};

typedef struct
{
	RedirType	redirType;
	int			fd;				// file descriptor being redirected
	int			fd2;			// file descriptor being redirected to/from (only valid if redirType==RT_HANDLE)
	TCHAR*		filename;		// filename associated with redirection (only valid if redirType==RT_FILE or PIPE_UNSP*)
	bool		append;			// true if the file being redirected to should be appending to
	bool		openForRead;	// true if the file being redirected to/from should be opened for reading
	bool		openForWrite;	// true if the file being redirected to/from should be opened for writing
} RedirArg;

// get redirection arguments, ie. filenames that relate to redirection.
// NULL returned for redirections that aren't specified.
// Same filename may be returned for multiple redirections, such as for: >foo 2>&1
// The filename buffers must be free()'d.
bool getRedirArgs(LPTSTR lpCmdLine, RedirArg** redirArgs, int* numRedirArgs);

bool removeRedirArgs(LPTSTR lpCmdLine);


#endif // __wcecompat__ARGS_H__
