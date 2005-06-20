/*++

Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

    uniqueid.h

Abstract:

    Definitions for the unique identifier returned by
    IOCTL_HAL_GET_DEVICEID

Revision History:

--*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <winioctl.h>

typedef struct _DEVICE_ID {
	DWORD	dwSize;
	DWORD	dwPresetIDOffset;
	DWORD	dwPresetIDBytes;
	DWORD	dwPlatformIDOffset;
	DWORD	dwPlatformIDBytes;
} DEVICE_ID, *PDEVICE_ID;


#ifndef IOCTL_HAL_GET_DEVICEID
#define IOCTL_HAL_GET_DEVICEID CTL_CODE(FILE_DEVICE_HAL, 21, METHOD_BUFFERED, FILE_ANY_ACCESS)
BOOL KernelIoControl(DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned);
#endif

#ifdef __cplusplus
}
#endif
