// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "net/http/Features.hpp"
#include "Tracking/SkyLines/Features.hpp"

/* live tracking requires networking */
#ifdef HAVE_HTTP
#define HAVE_LIVETRACK24
#endif

#if defined(HAVE_SKYLINES_TRACKING) || defined(HAVE_LIVETRACK24)
#define HAVE_TRACKING
#endif
