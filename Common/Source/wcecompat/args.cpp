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


#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include "ts_string.h"
#include "args.h"


typedef enum {
	TOK_ERROR = -1,
	TOK_NULL = 0,
	TOK_REDIR,
	TOK_STRING
} ArgType;

// redirection symbols allowed:
//	<	0<		redirect file to stdin
//	>	1>		redirect stdout to file
//		2>		redirect stderr to file
//		&>		redirect stdout and stderr to file
//	>>	1>>		append stdout to file
//		2>>		append stderr to file
//		&>>		append stdout and stderr to file
//	>&2	1>&2	redirect stdout to stderr
//		2>&1	redirect stderr to stdout


// return the command-line argument.
// This handles quotes and I/O redirection.
// ppCmdLine is updated to point to start of next arg.
// Below are the results of successive calls for the command-line: arg1 arg2<in>>out 2>err
//   "arg1"
//   "arg2"
//   "<"
//   "in"
//   ">>"
//   "out"
//   "2>"
//   "err"
// Note that redirection symbols are seperated from other args.
// Returns:
//   redirArg.redirType - not all RedirType values are returned, only RT_HANDLE and RT_FILE
//   redirArg.filename  - not all RedirType values are returned, only RT_HANDLE and RT_FILE
//   stderrAlso         - true if stderr is also being redirected with descriptor (only for "&>foo")
static ArgType getCmdLineArg(LPTSTR* ppCmdLine, TCHAR* arg, int maxArgChars, RedirArg* redirArg, bool* stderrAlso)
{
	ArgType		result = TOK_STRING;
	LPTSTR		p;
	BOOL		in_quotes = FALSE;
	int			argLen = 0;

	if (ppCmdLine == NULL || *ppCmdLine == NULL || *ppCmdLine[0] == 0 || arg == NULL || maxArgChars <= 0)
		return TOK_ERROR;

	// start at the beginning
	p = *ppCmdLine;

	// skip whitespace
	while (isspace(*p) && *p != 0)
		p++;

	// process each character
	while (*p != 0 && argLen < maxArgChars-1)
	{
		// break out if we hit a space (which seperates args)
		if (isspace(*p) && !in_quotes)
		{
			p++;
			break;
		}

		if (*p == '"')
		{
			if (!in_quotes)
				in_quotes = TRUE;
			else
				in_quotes = FALSE;
			p++;
		}
		else if (*p == '\\' && *(p+1) == '"')
		{
			arg[argLen++] = '"';
			p += 2;
		}
		else if (*p == '<' && !in_quotes)
		{	// redirect stdin
			if (argLen == 0 || (argLen == 1 && arg[0] == '0'))
			{
				arg[argLen++] = *p;
				result = TOK_REDIR;
				if (redirArg != NULL)
				{
					redirArg->redirType = RT_FILE;
					redirArg->fd = 0;
					redirArg->fd2 = -1;
					redirArg->filename = NULL;
					redirArg->append = false;
					redirArg->openForRead = true;
					redirArg->openForWrite = false;
				}
				if (stderrAlso)
					*stderrAlso = false;
			}
			else
			{	// redirection is the next arg (delimits the current arg)
			}
			break;
		}
		else if (*p == '>' && !in_quotes)
		{	// redirect stdout/stderr
			if (argLen == 0 || (argLen == 1 && (arg[0] == '1' || arg[0] == '2' || arg[0] == '&')))
			{
				RedirType	redirType = RT_FILE;
				bool		append = false;
				int			fd = -1;
				bool		_stderrAlso = false;
				int			fd2 = -1;
				bool		openForRead = false;
				bool		openForWrite = true;
				if (argLen == 0)
					fd = 1;
				else if (argLen == 1)
				{
					if (arg[0] == '1')
						fd = 1;
					else if (arg[0] == '2')
						fd = 2;
					else if (arg[0] == '&')
					{
						fd = 1;
						_stderrAlso = true;
					}
				}
				arg[argLen++] = *p++;			// copy '>'
				if (*p == '>')
				{
					arg[argLen++] = *p++;		// copy second '>'
					append = true;
				}
				else if (*p == '&')
				{
					arg[argLen++] = *p++;		// copy '&'
					if (*p == '1')
					{
						arg[argLen++] = *p++;	// copy '1'
						fd2 = 1;
					}
					else if (*p == '2')
					{
						arg[argLen++] = *p++;	// copy '2'
						fd2 = 2;
					}
					redirType = RT_HANDLE;
				}
				result = TOK_REDIR;
				if (redirArg != NULL)
				{
					redirArg->redirType = redirType;
					redirArg->fd = fd;
					redirArg->fd2 = fd2;
					redirArg->filename = NULL;
					redirArg->append = append;
					redirArg->openForRead = openForRead;
					redirArg->openForWrite = openForWrite;
				}
				if (stderrAlso)
					*stderrAlso = _stderrAlso;
			}
			else
			{	// redirection is the next arg (delimits the current arg)
			}
			break;
		}
		else
		{
			arg[argLen++] = *p++;
		}
	}

	// ensure argument is terminated
	arg[argLen] = 0;

	// return FALSE if no args were found
	if (argLen == 0)
		result = TOK_NULL;

	// update ppCmdLine to point to next arg
	*ppCmdLine = p;

	return result;
}


static BOOL wildcardFindFirst(const char* arg, HANDLE* findHandle, char** expandedArg)
{
	WIN32_FIND_DATA	findData;
	WCHAR			argW[1024];
	int				filenameLen;
	ascii2unicode(arg, argW, 1024);
	*findHandle = FindFirstFile(argW, &findData);
	if (*findHandle == (HANDLE)0xffffffff)
		return FALSE;
	filenameLen = wcslen(findData.cFileName);
	*expandedArg = (char*)malloc(filenameLen+1);
	if (*expandedArg == NULL)
		return FALSE;
	unicode2ascii(findData.cFileName, *expandedArg, filenameLen+1);
	return TRUE;
}


static BOOL wildcardFindNext(HANDLE findHandle, char** expandedArg)
{
	WIN32_FIND_DATA	findData;
	int				filenameLen;
	if (!FindNextFile(findHandle, &findData))
	{
		FindClose(findHandle);
		return FALSE;
	}
	filenameLen = wcslen(findData.cFileName);
	*expandedArg = (char*)malloc(filenameLen+1);
	if (*expandedArg == NULL)
	{
		FindClose(findHandle);
		return FALSE;
	}
	unicode2ascii(findData.cFileName, *expandedArg, filenameLen+1);
	return TRUE;
}


static int qsort_stricmp(const void* a, const void* b)
{
	return _stricmp(*(const char**)a, *(const char**)b);
}


// generate argc/argv from program name and lpCmdLine
// returns 0 on failure, or argc on success
int processCmdLine(LPTSTR lpCmdLine, char*** pArgv)
{
	BOOL		success = FALSE;
	int			argc = 0;
	char**		argv = NULL;
	TCHAR		programName[1024];
	const int	maxArgChars = 1024;
	TCHAR		argW[maxArgChars];
	char		arg[maxArgChars];
	ArgType		argType;

	// get program name
	if (GetModuleFileName(NULL, programName, 1024) <= 0)
		goto cleanup;
	programName[1023] = 0;	// make sure it's terminated when result is truncated

	// add program name to args
	argc++;
	argv = (char**)realloc(argv, argc*sizeof(char*));
	if (argv == NULL)
		goto cleanup;
	argv[argc-1] = (char*)malloc((wcslen(programName)+1)*2);
	if (argv[argc-1] == NULL)
		goto cleanup;
	unicode2ascii(programName, argv[argc-1], wcslen(programName)+1);

	while ((argType = getCmdLineArg(&lpCmdLine, argW, maxArgChars, NULL, NULL)) > 0)
	{
		ts_strcpy(arg, argW);	// convert to unicode (if we're in unicode)
		if (argType == TOK_STRING)
		{
			HANDLE	findHandle;
			char*	expandedArg;
			if ((strchr(arg,'*')==NULL && strchr(arg,'?')==NULL) || !wildcardFindFirst(arg, &findHandle, &expandedArg))
			{	// not a wildcard, or is but doesn't match anything, so just add arg to argv
				argc++;
				argv = (char**)realloc(argv, argc*sizeof(char*));
				if (argv == NULL)
					goto cleanup;
				argv[argc-1] = (char*)malloc(strlen(arg)+1);
				if (argv[argc-1] == NULL)
					goto cleanup;
				strcpy(argv[argc-1], arg);
			}
			else
			{	// handle wildcards
				int		firstExpandedArgIndex = argc;
				int		lastExpandedArgIndex = argc;
				int		numPathCharsToCopyFromArg = 0;
				char*	lastSlash = max(strrchr(arg,'\\'), strrchr(arg,'/'));
				if (lastSlash != NULL)
					numPathCharsToCopyFromArg = lastSlash - arg + 1;
				do
				{
					argc++;
					argv = (char**)realloc(argv, argc*sizeof(char*));
					if (argv == NULL)
					{
						free(expandedArg);
						goto cleanup;
					}
					if (numPathCharsToCopyFromArg == 0)
						argv[argc-1] = expandedArg;
					else
					{
						argv[argc-1] = (char*)malloc(numPathCharsToCopyFromArg + strlen(expandedArg) + 1);
						if (argv[argc-1] == NULL)
							goto cleanup;
						strncpy(argv[argc-1], arg, numPathCharsToCopyFromArg);
						strcpy(argv[argc-1]+numPathCharsToCopyFromArg, expandedArg);
					}
					lastExpandedArgIndex = argc-1;
				} while (wildcardFindNext(findHandle, &expandedArg));
				qsort(&argv[firstExpandedArgIndex], lastExpandedArgIndex-firstExpandedArgIndex+1, sizeof(char*), qsort_stricmp);
			}
		}
	}

	// success
	success = TRUE;

cleanup:

	if (!success)
	{
		if (argv != NULL)
		{
			int	i;
			for (i=0; i<argc; i++)
				free(argv[i]);
			free(argv);
		}
		argc = 0;
	}
	else
		*pArgv = argv;

	return argc;
}


// remove redir args from cmdline, does this to the original buffer
bool removeRedirArgs(LPTSTR lpCmdLine)
{
	bool		success = false;
	const int	maxArgChars = 1024;
	TCHAR		arg[maxArgChars];
	ArgType		argType;
	RedirArg	redirArg;
	LPTSTR		p;	// pointer to position in lpCmdLine before previous getCmdLineArg call, ie. start of arg

	if (lpCmdLine == NULL)
		return false;

	p = lpCmdLine;
	while ((argType = getCmdLineArg(&lpCmdLine, arg, maxArgChars, &redirArg, NULL)) > 0)
	{
		if (argType == TOK_REDIR)
		{
			if (redirArg.redirType == RT_FILE)
			{
				TCHAR	filename[1024];
				if (getCmdLineArg(&lpCmdLine, filename, sizeof(filename), NULL, NULL) != TOK_STRING)
					goto cleanup;
			}
			// remove redir arg
			if (*lpCmdLine == 0)
				*p = 0;
			else
				memmove(p, lpCmdLine, ts_strsizez(lpCmdLine));
			lpCmdLine = p;
		}
		p = lpCmdLine;
	}

	// success
	success = true;

cleanup:

	return success;
}


// get redirection arguments, ie. filenames that relate to redirection.
// NULL returned for redirections that aren't specified.
// Same filename may be returned for multiple redirections, such as for: >foo 2>&1
// The filename buffers must be free()'d.
bool getRedirArgs(LPTSTR lpCmdLine, RedirArg** redirArgs, int* numRedirArgs)
{
	bool		success = false;
	const int	maxArgChars = 1024;
	TCHAR		arg[maxArgChars];
	ArgType		argType;
	RedirArg	redirArg;
	bool		stderrAlso;

	if (lpCmdLine == NULL || redirArgs == NULL || numRedirArgs == NULL)
		return false;

	// for now, we only handle the first 3 descriptors (stdin, stdout, stderr), so pre-create the array
	*redirArgs = (RedirArg*)malloc(3*sizeof(RedirArg));
	if (*redirArgs == NULL)
		return false;
	*numRedirArgs = 3;
	// setup default stdin
	(*redirArgs)[0].redirType = RT_PIPE_STDIN;
	(*redirArgs)[0].fd = 0;
	(*redirArgs)[0].fd2 = -1;
	(*redirArgs)[0].filename = NULL;
	(*redirArgs)[0].append = false;
	(*redirArgs)[0].openForRead = true;
	(*redirArgs)[0].openForWrite = false;
	// setup default stdout
	(*redirArgs)[1].redirType = RT_PIPE_STDOUT;
	(*redirArgs)[1].fd = 1;
	(*redirArgs)[1].fd2 = -1;
	(*redirArgs)[1].filename = NULL;
	(*redirArgs)[1].append = false;
	(*redirArgs)[1].openForRead = false;
	(*redirArgs)[1].openForWrite = true;
	// setup default stderr
	(*redirArgs)[2].redirType = RT_PIPE_STDERR;
	(*redirArgs)[2].fd = 2;
	(*redirArgs)[2].fd2 = -1;
	(*redirArgs)[2].filename = NULL;
	(*redirArgs)[2].append = false;
	(*redirArgs)[2].openForRead = false;
	(*redirArgs)[2].openForWrite = true;

	while ((argType = getCmdLineArg(&lpCmdLine, arg, maxArgChars, &redirArg, &stderrAlso)) > 0)
	{
		if (argType == TOK_REDIR)
		{
			int	fd = redirArg.fd;
			if (fd > 2)
				goto cleanup;	// don't handle other file descriptors

			// if any other descriptors redirect via this descriptor, copy current redirArg to them
			int fdCopiedTo = -1;	// first descriptor copied to
			for (int i=0; i<*numRedirArgs; i++)
			{
				if (i == fd)
					continue;	// don't compare against self
				if ((*redirArgs)[i].redirType == RT_HANDLE && (*redirArgs)[i].fd2 == fd)
				{
					if (fdCopiedTo == -1)
					{	// copy redirArg to first referencing descriptor
						fdCopiedTo = i;
						(*redirArgs)[i] = (*redirArgs)[fd];
					}
					else
					{	// make other referencing descriptors reference the first referencing descriptor
						(*redirArgs)[i].fd2 = fdCopiedTo;
					}
				}
			}

			(*redirArgs)[fd] = redirArg;
			if (redirArg.redirType == RT_FILE)
			{
				TCHAR	filename[1024];
				if (getCmdLineArg(&lpCmdLine, filename, sizeof(filename), NULL, NULL) != TOK_STRING)
					goto cleanup;
				(*redirArgs)[fd].filename = _tcsdup(filename);
			}

			if (stderrAlso)
			{
				int	fd = 2;	// stderr

				// if any other descriptors redirect via this descriptor, copy current redirArg to them
				int fdCopiedTo = -1;	// first descriptor copied to
				for (int i=0; i<*numRedirArgs; i++)
				{
					if (i == fd)
						continue;	// don't compare against self
					if ((*redirArgs)[i].redirType == RT_HANDLE && (*redirArgs)[i].fd2 == fd)
					{
						if (fdCopiedTo == -1)
						{	// copy redirArg to first referencing descriptor
							fdCopiedTo = i;
							(*redirArgs)[i] = (*redirArgs)[fd];
						}
						else
						{	// make other referencing descriptors reference the first referencing descriptor
							(*redirArgs)[i].fd2 = fdCopiedTo;
						}
					}
				}

				(*redirArgs)[fd] = redirArg;
				(*redirArgs)[fd].redirType = RT_HANDLE;
				(*redirArgs)[fd].fd = fd;
				(*redirArgs)[fd].fd2 = redirArg.fd;
			}
		}
	}

	// success
	success = true;

cleanup:

	if (!success)
	{
		free(*redirArgs);
		*redirArgs = NULL;
		*numRedirArgs = 0;
	}

	return success;
}
