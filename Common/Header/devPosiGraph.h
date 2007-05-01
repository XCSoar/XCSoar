#ifndef DEVPG_H
#define DEVPG_H

#include <windows.h>

#include "sizes.h"

#include "MapWindow.h"

#include "device.h"

BOOL pgInstall(PDeviceDescriptor_t d);

BOOL pgRegister(void);

#endif

