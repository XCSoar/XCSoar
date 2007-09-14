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


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "ts_string.h"
#include "redir.h"	// initStdHandles


int			num_env_vars;
char**		_environ;
wchar_t**	_wenviron;


char* getenv(const char* name)
{
	initStdHandles();	// get environment variables from ChildData
	if (_environ == NULL)
		return NULL;
//	char** env = _environ;
//	while (*env != NULL)
	for (int i=0; i<num_env_vars; i++)
	{
		char** env = &_environ[i];
		char* equals = strchr(*env, '=');
		if (equals != NULL)
		{
			if (_strnicmp(name, *env, equals-*env) == 0)
			{
				return equals+1;
			}
		}
	}
	return NULL;
}


int _putenv(const char* str)
{
	_environ = (char**)realloc(_environ, (num_env_vars+1)*sizeof(char*));
	if (_environ == NULL)
		return -1;
	_wenviron = (wchar_t**)realloc(_wenviron, (num_env_vars+1)*sizeof(wchar_t*));
	if (_wenviron == NULL)
		return -1;
	num_env_vars++;
	_environ[num_env_vars-1] = ts_strdup(str);
	_wenviron[num_env_vars-1] = ts_strdup_ascii_to_unicode(str);
	return 0;
}


int _wputenv(const wchar_t* str)
{
	_environ = (char**)realloc(_environ, (num_env_vars+1)*sizeof(char*));
	if (_environ == NULL)
		return -1;
	_wenviron = (wchar_t**)realloc(_wenviron, (num_env_vars+1)*sizeof(wchar_t*));
	if (_wenviron == NULL)
		return -1;
	num_env_vars++;
	_environ[num_env_vars-1] = ts_strdup_unicode_to_ascii(str);
	_wenviron[num_env_vars-1] = ts_strdup(str);
	return 0;
}
