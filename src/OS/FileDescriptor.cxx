/*
 * Copyright (C) 2012-2016 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FileDescriptor.hxx"

#include <sys/stat.h>
#include <fcntl.h>

#ifdef __BIONIC__
#include <sys/syscall.h>
#endif

#ifdef HAVE_POSIX
#include <poll.h>
#endif

#if defined(HAVE_EVENTFD) && !defined(__BIONIC__)
#include <sys/eventfd.h>
#endif

#if defined(HAVE_SIGNALFD) && !defined(__BIONIC__)
#include <sys/signalfd.h>
#endif

#ifdef HAVE_INOTIFY
#include <sys/inotify.h>
#endif

#ifndef O_NOCTTY
#define O_NOCTTY 0
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

bool
FileDescriptor::Open(const char *pathname, int flags, mode_t mode)
{
	fd = ::open(pathname, flags | O_NOCTTY | O_CLOEXEC, mode);
	return IsDefined();
}

bool
FileDescriptor::OpenReadOnly(const char *pathname)
{
	return Open(pathname, O_RDONLY);
}

#ifdef HAVE_POSIX

bool
FileDescriptor::OpenNonBlocking(const char *pathname)
{
	return Open(pathname, O_RDWR | O_NONBLOCK);
}

bool
FileDescriptor::CreatePipe(FileDescriptor &r, FileDescriptor &w)
{
	int fds[2];

#ifdef __linux__
	const int flags = O_CLOEXEC;
#ifdef __BIONIC__
	/* Bionic provides the pipe2() function only since Android 2.3,
	   therefore we must roll our own system call here */
	const int result = syscall(__NR_pipe2, fds, flags);
#else
	const int result = pipe2(fds, flags);
#endif
#else
	const int result = pipe(fds);
#endif

	if (result < 0)
		return false;

	r = FileDescriptor(fds[0]);
	w = FileDescriptor(fds[1]);
	return true;
}

void
FileDescriptor::SetNonBlocking()
{
	assert(IsDefined());

	int flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void
FileDescriptor::SetBlocking()
{
	assert(IsDefined());

	int flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

void
FileDescriptor::EnableCloseOnExec()
{
	assert(IsDefined());

	const int old_flags = fcntl(fd, F_GETFD, 0);
	fcntl(fd, F_SETFD, old_flags | FD_CLOEXEC);
}

void
FileDescriptor::DisableCloseOnExec()
{
	assert(IsDefined());

	const int old_flags = fcntl(fd, F_GETFD, 0);
	fcntl(fd, F_SETFD, old_flags & ~FD_CLOEXEC);
}

bool
FileDescriptor::CheckDuplicate(int new_fd)
{
	if (fd == new_fd) {
		DisableCloseOnExec();
		return true;
	} else
		return Duplicate(new_fd);
}

#endif

#ifdef HAVE_EVENTFD

bool
FileDescriptor::CreateEventFD(unsigned initval)
{
#ifdef __BIONIC__
	/* Bionic provides the eventfd() function only since Android 2.3,
	   therefore we must roll our own system call here */
	fd = syscall(__NR_eventfd2, initval, O_NONBLOCK|O_CLOEXEC);
#else
	fd = ::eventfd(initval, EFD_NONBLOCK|EFD_CLOEXEC);
#endif
	return fd >= 0;
}

#endif

#ifdef HAVE_SIGNALFD

bool
FileDescriptor::CreateSignalFD(const sigset_t *mask)
{
#ifdef __BIONIC__
	int new_fd = syscall(__NR_signalfd4, fd, mask, sizeof(*mask),
			     O_NONBLOCK|O_CLOEXEC);
#else
	int new_fd = ::signalfd(fd, mask, SFD_NONBLOCK|SFD_CLOEXEC);
#endif
	if (new_fd < 0)
		return false;

	fd = new_fd;
	return true;
}

#endif

#ifdef HAVE_INOTIFY

bool
FileDescriptor::CreateInotify()
{
#ifdef __BIONIC__
	/* Bionic doesn't have inotify_init1() */
	int new_fd = inotify_init();
#else
	int new_fd = inotify_init1(IN_CLOEXEC|IN_NONBLOCK);
#endif
	if (new_fd < 0)
		return false;

#ifdef __BIONIC__
	SetNonBlocking();
#endif

	fd = new_fd;
	return true;
}

#endif

bool
FileDescriptor::Rewind()
{
	assert(IsDefined());

	return lseek(fd, 0, SEEK_SET) == 0;
}

off_t
FileDescriptor::GetSize() const
{
	struct stat st;
	return ::fstat(fd, &st) >= 0
		? (long)st.st_size
		: -1;
}

#ifdef HAVE_POSIX

int
FileDescriptor::Poll(short events, int timeout) const
{
	assert(IsDefined());

	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = events;
	int result = poll(&pfd, 1, timeout);
	return result > 0
		? pfd.revents
		: result;
}

int
FileDescriptor::WaitReadable(int timeout) const
{
	return Poll(POLLIN, timeout);
}

int
FileDescriptor::WaitWritable(int timeout) const
{
	return Poll(POLLOUT, timeout);
}

#endif
