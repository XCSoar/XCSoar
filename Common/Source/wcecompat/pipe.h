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


#ifndef __wcecompat__pipe_h__
#define __wcecompat__pipe_h__


#include <windows.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct Pipe
{
//	bool	hasChannels;	// true if channelNum and data length should preceed each transmission
//	BYTE	channelNum;		// number to preceed each transmission
	HANDLE	hReadableEvent;	// manual-reset event to signal other end of pipe that data is available for reading
	HANDLE	hWriteableEvent;// manual-reset event to signal other end of pipe that buffer has room for writing
	HANDLE	hMutex;			// mutex for synchronising access to shared memory buffer
	HANDLE	hFileMapping;	// handle of shared memory mapping
	char*	pBuffer;		// pointer to shared memory buffer
	DWORD	bufferLength;	// length of buffer
} Pipe;


// dwCreationDisposition can be one of the following values from CreateFile:
//   CREATE_NEW		Fail if the pipe already exists
//   OPEN_EXISTING	Fail if the pipe does not exist
Pipe* createPipe(const TCHAR* name, DWORD dwCreationDisposition);

void closePipe(Pipe* pipe);

//void setPipeChannelNum(Pipe* pipe, unsigned char channelNum);
//Pipe* createPipeChannel(Pipe* pipe, unsigned char channelNum);

bool pipeReadable(Pipe* pipe);

int pipeRead(Pipe* pipe, unsigned char* data, int max_len);
//int pipeReadChannel(Pipe* pipe, unsigned char* data, int max_len, unsigned char* channelNum);
int pipeWrite(Pipe* pipe, unsigned char* data, int length);

#ifdef __cplusplus
}
#endif


#endif /* __wcecompat__pipe_h__ */
