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


#include "ChildData.h"
#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "ts_string.h"


extern char** _environ;

// Encoded format:
//   DWORD		total size (in bytes)
//   DWORD		reserved, possibly for version number and flags (for now, MUST BE ZERO)
//   UNICODEZ	Current directory
//   DWORD		Number of redirections
//   Array of:
//     DWORD	flags (bits: 0&1={0=pipe,1=file,2=handle}, 2=filename_present, 3=append, 4=read, 5=write)
//     DWORD	fd
//     DWORD	fd2
//     UNICODEZ	name of pipe or file (eg. "wcecompat.12345678.stdin.pipe" or "\in.txt")
//   DWORD		size (in bytes) of environment variables (including NULL termination)
//   NULL-terminated list of environment variables (each entry of the form "name=value")

ChildData::ChildData() :
	currentDirectory(NULL),
	redirArgs(NULL), numRedirArgs(0),
	environment(NULL), environmentLength(0)
{
}

ChildData::~ChildData()
{
	free(currentDirectory);
	free(redirArgs);
	resetEnvironment();
}

const TCHAR* ChildData::getCurrentDirectory() const
{
	return currentDirectory;
}

void ChildData::setCurrentDirectory(const TCHAR* directory)
{
	if (currentDirectory != NULL)
		free(currentDirectory);
	currentDirectory = ts_strdup(directory);
}

void ChildData::resetCurrentDirectory()
{
	if (currentDirectory != NULL)
	{
		free(currentDirectory);
		currentDirectory = NULL;
	}
}

void ChildData::resetRedirArgs()
{
	if (redirArgs != NULL)
	{
		free(redirArgs);
		redirArgs = NULL;
	}
	numRedirArgs = 0;
}

RedirArg* ChildData::getRedirArg(int fd)
{
	if (redirArgs == NULL)
		return NULL;
	for (int i=0; i<numRedirArgs; i++)
		if (redirArgs[i].fd == fd)
			return &redirArgs[i];
	return NULL;
}

bool ChildData::setRedirArg(RedirArg* redirArg)
{
	// make sure array is allocated
	if (redirArgs == NULL)
	{
		redirArgs = (RedirArg*)malloc(sizeof(RedirArg));
		if (redirArgs == NULL)
			return false;
	}
	// check for existing entry (and overwrite if found)
	for (int i=0; i<numRedirArgs; i++)
		if (redirArgs[i].fd == redirArg->fd)
		{
			redirArgs[i] = *redirArg;
			return true;
		}
	// add entry
	redirArgs = (RedirArg*)realloc(redirArgs, sizeof(RedirArg)*(numRedirArgs+1));
	if (redirArgs == NULL)
		return false;
	redirArgs[numRedirArgs++] = *redirArg;
	return true;
}

bool ChildData::setRedirArgs(RedirArg* redirArgs, int numRedirArgs)
{
	for (int i=0; i<numRedirArgs; i++)
		if (!setRedirArg(&redirArgs[i]))
			return false;
	return true;
}

void ChildData::resetEnvironment()
{
	if (environment != NULL)
	{
		for (int i=0; i<environmentLength; i++)
		{
			free(environment[i].name);
			free(environment[i].value);
		}
		free(environment);
		environment = NULL;
	}
	environmentLength = 0;
}

bool ChildData::addEnvironmentVar(WCHAR* env)
{
	bool	result = false;
	WCHAR*	equals;
	int		nameLen;
	TCHAR*	name = NULL;
	TCHAR*	value = NULL;

	equals = wcschr(env, L'=');
	if (equals == NULL)
		goto cleanup;
	nameLen = equals - env;
	if (nameLen == 0)
		goto cleanup;
	name = (TCHAR*)malloc((nameLen+1)*sizeof(TCHAR));
	if (name == NULL)
		goto cleanup;
	ts_strncpy(name, env, nameLen);
	name[nameLen] = '\0';
	value = (TCHAR*)malloc((ts_strlen(equals+1)+1)*sizeof(TCHAR));
	if (value == NULL)
		goto cleanup;
	ts_strcpy(value, equals+1);
	environment = (NameValuePair*)realloc(environment, (environmentLength+1)*sizeof(NameValuePair));
	if (environment == NULL)
		goto cleanup;
	environment[environmentLength].name = name;
	environment[environmentLength].value = value;

	environmentLength++;

	result = true;

cleanup:

	if (result == false)
	{
		if (name != NULL)
			free(name);
		if (value != NULL)
			free(value);
	}

	return result;
}

bool ChildData::addEnvironmentList(WCHAR* env)
{
	if (env == NULL)
		return false;

	while (*(char*)env != '\0' || *(((char*)env)+1) != '\0')
	{
		addEnvironmentVar(env);
		env += ts_strlen(env)+1;
	}
	return true;
}

bool ChildData::saveEnvironment()
{
#if 0
	if (_environ != NULL)
	{
		for (char** env = _environ; *env != NULL; env++)
		{
			strcpy(p, *env);
			p += strlen(*env) + 1;
		}
	}
#endif
	return true;
}

bool ChildData::restoreEnvironment()
{
	TCHAR	buf[1024];
	for (int i=0; i<environmentLength; i++)
	{
		ts_strcpy(buf, environment[i].name);
		ts_strcat(buf, "=");
		ts_strcat(buf, environment[i].value);
//JMW		_tputenv(buf);
	}
	return true;
}

// if childData is NULL, the size will be returned in childDataLen
bool ChildData::encode(void* childData, int* childDataSize)
{
	int	currentDirectorySize = (currentDirectory == NULL) ? sizeof(WCHAR) : ts_strsizez(currentDirectory);
	int	redirSize = 0;
	for (int i=0; i<numRedirArgs; i++)
	{
		redirSize += sizeof(DWORD) + sizeof(DWORD) + sizeof(DWORD);
		if (redirArgs[i].redirType == RT_PIPE_UNSPEC ||
			redirArgs[i].redirType == RT_PIPE_STDIN ||
			redirArgs[i].redirType == RT_PIPE_STDOUT ||
			redirArgs[i].redirType == RT_PIPE_STDERR ||
			redirArgs[i].redirType == RT_FILE)
		{
			if (redirArgs[i].filename != NULL)
				redirSize += ts_strsizez(redirArgs[i].filename);
		}
	}
	DWORD	envSize = 0;
	if (environment != NULL)
	{
		for (int i=0; i<environmentLength; i++)
			envSize += (_tcslen(environment[i].name) + 1 + _tcslen(environment[i].value) + 1) * sizeof(WCHAR);
		envSize += sizeof(WCHAR);	// list termination
	}
	int	totalSize = 0;
	totalSize += sizeof(DWORD);				// length
	totalSize += sizeof(DWORD);				// reserved
	totalSize += currentDirectorySize;		// current directory
	totalSize += sizeof(DWORD);				// number of redirs
	totalSize += redirSize;					// redirs
	totalSize += sizeof(DWORD);				// environment length
	totalSize += envSize;					// environment variable list

	if (childDataSize != NULL)
		*childDataSize = totalSize;

	if (childData != NULL)
	{
		char* p = (char*)childData;
		// total size
		memcpy(p, &totalSize, sizeof(DWORD));	//*(DWORD*)p = totalSize;
		p += sizeof(DWORD);
		// reserved
		{
		DWORD	reserved = 0;
		memcpy(p, &reserved, sizeof(DWORD));	//*(DWORD*)p = 0;
		p += sizeof(DWORD);
		}
		// current directory
		if (currentDirectory == NULL)
		{
			WCHAR	zero = '\0';
			*p = '\0';
			memcpy(p, &zero, sizeof(WCHAR));	//*p = '\0';
			p += sizeof(WCHAR);
		}
		else
		{
			ts_strcpy((WCHAR*)p, currentDirectory);
			p += (ts_strlen((WCHAR*)p) + 1) * 2;
		}
		// num redirs
		memcpy(p, &numRedirArgs, sizeof(DWORD));	//*(DWORD*)p = numRedirArgs;
		p += sizeof(DWORD);
		// redirs
		for (int i=0; i<numRedirArgs; i++)
		{
			// determine if filename is present
			bool filenamePresent = false;
			if (redirArgs[i].redirType == RT_PIPE_UNSPEC ||
				redirArgs[i].redirType == RT_PIPE_STDIN ||
				redirArgs[i].redirType == RT_PIPE_STDOUT ||
				redirArgs[i].redirType == RT_PIPE_STDERR ||
				redirArgs[i].redirType == RT_FILE)
			{
				if (redirArgs[i].filename != NULL)
					filenamePresent = true;
			}
			// flags
			DWORD	flags = 0;
			if (redirArgs[i].redirType == RT_FILE)
				flags |= 0x00000001;
			else if (redirArgs[i].redirType == RT_HANDLE)
				flags |= 0x00000002;
			if (filenamePresent)
				flags |= 0x00000004;
			if (redirArgs[i].append)
				flags |= 0x00000008;
			if (redirArgs[i].openForRead)
				flags |= 0x00000010;
			if (redirArgs[i].openForWrite)
				flags |= 0x00000020;
			memcpy(p, &flags, sizeof(DWORD));	//*(DWORD*)p = flags;
			p += sizeof(DWORD);
			// fd
			memcpy(p, &redirArgs[i].fd, sizeof(DWORD));	//*(DWORD*)p = redirArgs[i].fd;
			p += sizeof(DWORD);
			// fd2
			memcpy(p, &redirArgs[i].fd2, sizeof(DWORD));	//*(DWORD*)p = redirArgs[i].fd2;
			p += sizeof(DWORD);
			// pipe/file name
			if (filenamePresent)
			{
				ts_strcpy((WCHAR*)p, redirArgs[i].filename);
				p += (ts_strlen((WCHAR*)p) + 1) * 2;
			}
		}
		// environment size
		memcpy(p, &envSize, sizeof(DWORD));	//*(DWORD*)p = ENVIRONMENTLENGTH * 2;
		p += sizeof(DWORD);
		// environment
		if (envSize != 0)
		{
			for (int i=0; i<environmentLength; i++)
			{
				ts_strcpy((WCHAR*)p, environment[i].name);
				p += ts_strlen((WCHAR*)p) * sizeof(WCHAR);
				ts_strcpy((WCHAR*)p, L"=");
				p += sizeof(WCHAR);
				ts_strcpy((WCHAR*)p, environment[i].value);
				p += (ts_strlen((WCHAR*)p) + 1) * sizeof(WCHAR);
			}
			// terminate the environment list
			WCHAR	zero = '\0';
			memcpy(p, &zero, sizeof(WCHAR));	//*p = '\0';
			p += sizeof(WCHAR);
		}
	}

	return true;
}

// if childData is NULL, the size will be returned in childDataLen
bool ChildData::decode(void* childData)//, int childDataSize)
{
	bool	result = false;
	int		strLen;

	if (childData == NULL)// || childDataSize < sizeof(DWORD))
		return false;

	char* p = (char*)childData;

	// total size
	DWORD size;
	memcpy(&size, p, sizeof(DWORD));
	p += sizeof(DWORD);
//	// check that size encoded in buffer isn't bigger that passed-in size
//	if (size > childDataSize)
//		return false;
	size -= sizeof(DWORD);

	// reserved (ignore)
	p += sizeof(DWORD);
	size -= sizeof(DWORD);

	// get current directory
	strLen = ts_strlen((WCHAR*)p);
	if ((strLen+1)*2 > (int)size)
		goto cleanup;
	if (strLen == 0)
		resetCurrentDirectory();
	else
	{
#ifdef UNICODE
		setCurrentDirectory((WCHAR*)p);
#else
		char buf[256];
		unicode2ascii((WCHAR*)p, buf);
		setCurrentDirectory(buf);
#endif
	}
	p += (strLen + 1) * 2;
	size -= (strLen + 1)*2;

	// num redirs
	memcpy(&numRedirArgs, p, sizeof(DWORD));
	p += sizeof(DWORD);
	size -= sizeof(DWORD);

	// allocate redirs array
	if (redirArgs != NULL)
		free(redirArgs);
	redirArgs = (RedirArg*)malloc(sizeof(RedirArg)*numRedirArgs);
	if (redirArgs == NULL)
		goto cleanup;

	// redirs
	{
	for (int i=0; i<numRedirArgs; i++)
	{
		// flags
		DWORD flags;
		memcpy(&flags, p, sizeof(DWORD));
		p += sizeof(DWORD);
		size -= sizeof(DWORD);
		switch (flags&0x00000003)
		{
			case 0:
				redirArgs[i].redirType = RT_PIPE_UNSPEC;
				break;
			case 1:
				redirArgs[i].redirType = RT_FILE;
				break;
			case 2:
				redirArgs[i].redirType = RT_HANDLE;
				break;
		}
		bool filenamePresent = false;
		if (flags & 0x00000004)
			filenamePresent = true;
		redirArgs[i].append			= ((flags & 0x00000008) != 0);
		redirArgs[i].openForRead	= ((flags & 0x00000010) != 0);
		redirArgs[i].openForWrite	= ((flags & 0x00000020) != 0);
		// fd
		memcpy(&redirArgs[i].fd, p, sizeof(DWORD));
		p += sizeof(DWORD);
		size -= sizeof(DWORD);
		// fd2
		memcpy(&redirArgs[i].fd2, p, sizeof(DWORD));
		p += sizeof(DWORD);
		size -= sizeof(DWORD);
		// pipe/file name
		if (filenamePresent)
		{
			int strLen = ts_strlen((WCHAR*)p);
			if ((strLen+1)*2 > (int)size)
				goto cleanup;
			redirArgs[i].filename = _tcsdup((WCHAR*)p);
			p += (strLen + 1)*2;
			size -= (strLen + 1)*2;
		}
		else
			redirArgs[i].filename = NULL;
	}
	}

	// get environment
	if (size <= 0)
		return false;
	p += sizeof(DWORD);	// skip env size
	resetEnvironment();
	addEnvironmentList((WCHAR*)p);

	// success
	result = true;

cleanup:

	return result;
}

void ChildData::dump()
{
	printf("========================================================================\n");
	printf("ChildData:\n");
	printf("========================================================================\n");
	_tprintf(TEXT("currentDirectory=\"%s\"\n"), currentDirectory ? currentDirectory : TEXT("<null>"));
	for (int i=0; i<numRedirArgs; i++)
	{
		if (redirArgs[i].redirType == RT_PIPE_UNSPEC)
		{
			_tprintf(TEXT("redirType=RT_PIPE_UNSPEC, fd#%d = \"%s\", append=%s, read=%s, write=%s\n"),
					redirArgs[i].fd,
					redirArgs[i].filename ? redirArgs[i].filename : TEXT("<null>"),
					redirArgs[i].append ? TEXT("true") : TEXT("false"),
					redirArgs[i].openForRead ? TEXT("true") : TEXT("false"),
					redirArgs[i].openForWrite ? TEXT("true") : TEXT("false"));
		}
		else if (redirArgs[i].redirType == RT_FILE)
		{
			_tprintf(TEXT("redirType=RT_FILE, fd#%d = \"%s\", append=%s, read=%s, write=%s\n"),
					redirArgs[i].fd,
					redirArgs[i].filename ? redirArgs[i].filename : TEXT("<null>"),
					redirArgs[i].append ? TEXT("true") : TEXT("false"),
					redirArgs[i].openForRead ? TEXT("true") : TEXT("false"),
					redirArgs[i].openForWrite ? TEXT("true") : TEXT("false"));
		}
		else if (redirArgs[i].redirType == RT_HANDLE)
		{
			_tprintf(TEXT("redirType=RT_HANDLE, fd#%d = fd#%d, append=%s, read=%s, write=%s\n"),
					redirArgs[i].fd, redirArgs[i].fd2,
					redirArgs[i].append ? TEXT("true") : TEXT("false"),
					redirArgs[i].openForRead ? TEXT("true") : TEXT("false"),
					redirArgs[i].openForWrite ? TEXT("true") : TEXT("false"));
		}
		else
		{
			_tprintf(TEXT("redirType=UNKNOWN(%d), fd#%d = fd#%d, append=%s, read=%s, write=%s\n"),
					(int)redirArgs[i].redirType,
					redirArgs[i].fd, redirArgs[i].fd2,
					redirArgs[i].append ? TEXT("true") : TEXT("false"),
					redirArgs[i].openForRead ? TEXT("true") : TEXT("false"),
					redirArgs[i].openForWrite ? TEXT("true") : TEXT("false"));
		}
	}
	printf("environmentLength=%d\n", environmentLength);
	if (environment == NULL)
		printf("environment=<null>\n");
	else
	{
		for (int i=0; i<environmentLength; i++)
			_tprintf(TEXT("env[%2d]: %s=%s"), i, environment[i].name, environment[i].value);
	}
	printf("========================================================================\n");
}
