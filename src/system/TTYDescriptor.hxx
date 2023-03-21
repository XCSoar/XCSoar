// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max@duempel.org>

#pragma once

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
	[[gnu::pure]]
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
