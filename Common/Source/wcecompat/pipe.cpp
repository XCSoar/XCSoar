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
#include <stdio.h>
#include <tchar.h>
#include "pipe.h"
#include "ts_string.h"


// dwCreationDisposition can be one of the following values from CreateFile:
//   CREATE_NEW		Fail if the pipe already exists
//   OPEN_EXISTING	Fail if the pipe does not exist
Pipe* createPipe(const TCHAR* name, DWORD dwCreationDisposition)
{
	if (dwCreationDisposition != CREATE_NEW && dwCreationDisposition != OPEN_EXISTING)
		return NULL;

	BOOL	result = FALSE;
	Pipe*	pipe = new Pipe;
	if (pipe == NULL)
		return NULL;
	pipe->hReadableEvent = NULL;
	pipe->hWriteableEvent = NULL;
	pipe->hMutex = NULL;
	pipe->hFileMapping = NULL;
	pipe->pBuffer = NULL;
	pipe->bufferLength = 16384;

	TCHAR mutexName[100];
	_stprintf(mutexName, TEXT("%s.mutex"), name);
	pipe->hMutex = CreateMutex(NULL, FALSE, mutexName);
	if (pipe->hMutex == NULL)
		goto cleanup;
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		if (dwCreationDisposition == CREATE_NEW)
			goto cleanup;
	}
	else
	{
		if (dwCreationDisposition == OPEN_EXISTING)
			goto cleanup;
	}

	TCHAR readableEventName[100];
	_stprintf(readableEventName, TEXT("%s.readable"), name);
	pipe->hReadableEvent = CreateEvent(NULL, TRUE, FALSE, readableEventName);
	if (pipe->hReadableEvent == NULL)
		goto cleanup;
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		if (dwCreationDisposition == CREATE_NEW)
			goto cleanup;
	}
	else
	{
		if (dwCreationDisposition == OPEN_EXISTING)
			goto cleanup;
	}

	TCHAR writeableEventName[100];
	_stprintf(writeableEventName, TEXT("%s.writeable"), name);
	pipe->hWriteableEvent = CreateEvent(NULL, TRUE, TRUE, writeableEventName);
	if (pipe->hWriteableEvent == NULL)
		goto cleanup;
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		if (dwCreationDisposition == CREATE_NEW)
			goto cleanup;
	}
	else
	{
		if (dwCreationDisposition == OPEN_EXISTING)
			goto cleanup;
	}

	TCHAR memName[100];
	_stprintf(memName, TEXT("%s.mem"), name);
	pipe->hFileMapping = CreateFileMapping((HANDLE)INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, pipe->bufferLength, memName);
	if (pipe->hFileMapping == NULL)
		goto cleanup;
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		if (dwCreationDisposition == CREATE_NEW)
			goto cleanup;
	}
	else
	{
		if (dwCreationDisposition == OPEN_EXISTING)
			goto cleanup;
	}

	pipe->pBuffer = (char*)MapViewOfFile(pipe->hFileMapping, FILE_MAP_WRITE, 0, 0, 0);
	if (pipe->pBuffer == NULL)
		goto cleanup;
	memset(pipe->pBuffer, 0, pipe->bufferLength);

	result = TRUE;

cleanup:

	if (result == FALSE)
	{
		closePipe(pipe);
		pipe = NULL;
	}
	return pipe;
}


void closePipe(Pipe* pipe)
{
	if (pipe != NULL)
	{
		if (pipe->hReadableEvent != NULL)
			CloseHandle(pipe->hReadableEvent);
		if (pipe->hWriteableEvent != NULL)
			CloseHandle(pipe->hWriteableEvent);
		if (pipe->hMutex != NULL)
			CloseHandle(pipe->hMutex);
		if (pipe->pBuffer != NULL)
			UnmapViewOfFile(pipe->pBuffer);
		if (pipe->hFileMapping != NULL)
			CloseHandle(pipe->hFileMapping);
		delete pipe;
	}
}


bool pipeReadable(Pipe* pipe)
{
	if (pipe == NULL)
		return false;

	// check if the pipe is readable
	return (WaitForSingleObject(pipe->hReadableEvent, 0) == WAIT_OBJECT_0);
}


int pipeRead(Pipe* pipe, unsigned char* data, int max_len)
{
	int bytes_read = 0;

	if (pipe == NULL)
		return 0;

	// wait for pipe to become readable
	// NOTE: what if the caller already did this, such as the "select" loop in cerunner???
	//       this is a waste and unnecessary penalty then
	WaitForSingleObject(pipe->hReadableEvent, INFINITE);

	// serialise access through mutex (timeout after 5 seconds)
	DWORD waitResult = WaitForSingleObject(pipe->hMutex, 5000);
	if (waitResult == WAIT_FAILED || waitResult == WAIT_TIMEOUT)
		return 0;

	// get length from shared memory
	int		length = ((DWORD*)pipe->pBuffer)[0];
	int		maxLength = pipe->bufferLength - sizeof(DWORD);
	bool	full = (length >= maxLength);

	// copy data
	if (max_len >= (int)length)
	{
		bytes_read = length;
		memcpy(data, &((DWORD*)pipe->pBuffer)[1], bytes_read);
		((DWORD*)pipe->pBuffer)[0] = 0;
		ResetEvent(pipe->hReadableEvent);		// no data left for reading
		if (full)
			SetEvent(pipe->hWriteableEvent);	// now writeable (only set if was full)
	}
	else
	{
		bytes_read = max_len;
		memcpy(data, &((DWORD*)pipe->pBuffer)[1], bytes_read);
		length -= bytes_read;
		((DWORD*)pipe->pBuffer)[0] = length;
		memmove(&((DWORD*)pipe->pBuffer)[1], ((unsigned char*)&((DWORD*)pipe->pBuffer)[1])+bytes_read, length);
		if (full && length < maxLength)
			SetEvent(pipe->hWriteableEvent);	// now writeable (only set if was full)
	}

	// release mutex
	ReleaseMutex(pipe->hMutex);

	return bytes_read;
}


int pipeWrite(Pipe* pipe, unsigned char* data, int length)
{
	int	bytes_written = 0;
	if (pipe == NULL)
		return EOF;

	while (length > 0)
	{
		// wait for pipe to become writeable
		// NOTE: what if the caller already did this, such as the "select" loop in cerunner???
		//       this is a waste and unnecessary penalty then
		WaitForSingleObject(pipe->hWriteableEvent, INFINITE);

		// serialise access through mutex (timeout after 5 seconds)
		DWORD waitResult = WaitForSingleObject(pipe->hMutex, 5000);
		if (waitResult == WAIT_FAILED || waitResult == WAIT_TIMEOUT)
			return EOF;

		DWORD originalLength = ((DWORD*)pipe->pBuffer)[0];
		DWORD currentLength = originalLength;
		DWORD maxLength = pipe->bufferLength - sizeof(DWORD);
		if (currentLength < maxLength)
		{
			DWORD lengthToWriteNow = length;
			if (currentLength+length > maxLength)
				lengthToWriteNow = maxLength - currentLength;
			// copy data into shared memory
			memcpy(((unsigned char*)&((DWORD*)pipe->pBuffer)[1])+currentLength, data, lengthToWriteNow);
			// write length into shared memory
			currentLength += lengthToWriteNow;
			((DWORD*)pipe->pBuffer)[0] = currentLength;
			if (currentLength >= (DWORD)maxLength)
				ResetEvent(pipe->hWriteableEvent);			// buffer full, no longer writeable
			// keep track of total bytes written during this writePipe() call
			bytes_written += lengthToWriteNow;
			// adjust data pointer and length for next write
			data += bytes_written;
			length -= bytes_written;
		}

		// signal data available for reading
		if (originalLength == 0)
		{	// only signal if there was no data in the buffer
			SetEvent(pipe->hReadableEvent);
		}

		// release mutex
		ReleaseMutex(pipe->hMutex);
	}

	return bytes_written;
}
