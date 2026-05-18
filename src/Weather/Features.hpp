// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "net/http/Features.hpp"

#ifdef HAVE_HTTP
#define HAVE_NOAA
#define HAVE_PCMET
#ifdef ENABLE_OPENGL
#define HAVE_EDL
#endif
#endif
