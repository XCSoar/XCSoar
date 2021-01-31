/*
 * Copyright 2012-2021 Max Kellermann <max@duempel.org>
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

#ifndef TTY_DESCRIPTOR_HXX
#define TTY_DESCRIPTOR_HXX

#include "io/FileDescriptor.hxx"

#include <stdlib.h>
#include <termios.h>

/**
 * An OO wrapper for a terminal file descriptor.
 */
class TTYDescriptor : public FileDescriptor {
public:
	constexpr TTYDescriptor(FileDescriptor _fd) noexcept
		:FileDescriptor(_fd) {}

	bool Unlock() const noexcept {
		return unlockpt(Get()) >= 0;
	}

	/**
	 * Synchronously wait for all pending output to be transmitted.
	 */
	bool Drain() const noexcept {
#ifdef __BIONIC__
		/* bionic doesn't have tcdrain() */
		return true;
#else
		return tcdrain(Get()) == 0;
#endif
	}

	/**
	 * Returns the name of the slave pseudo-terminal.  Not thread-safe!
	 */
	gcc_pure
	const char *GetSlaveName() const noexcept {
		return ptsname(Get());
	}

	/**
	 * Discard all pending input.
	 */
	bool FlushInput() const noexcept {
		return tcflush(Get(), TCIFLUSH) >= 0;
	}

	/**
	 * Discard all pending output.
	 */
	bool FlushOutput() const noexcept {
		return tcflush(Get(), TCOFLUSH) >= 0;
	}

	/**
	 * Discard both pending input and output.
	 */
	bool FlushBoth() const noexcept {
		return tcflush(Get(), TCIOFLUSH) >= 0;
	}

	bool GetAttr(struct termios &attr) const noexcept {
		return tcgetattr(Get(), &attr) >= 0;
	}

	bool SetAttr(int optional_actions,
		     const struct termios &attr) const noexcept {
		return tcsetattr(Get(), optional_actions, &attr) >= 0;
	}
};

#endif
