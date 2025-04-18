#pragma once

#ifdef __linux__
#define USE_EVENTFD
#define USE_SIGNALFD
#define USE_EPOLL
#endif

#define HAVE_THREADED_EVENT_LOOP
