/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_THREAD_LOCAL_HXX
#define XCSOAR_THREAD_LOCAL_HXX

#ifdef HAVE_POSIX
#include <pthread.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

/**
 * This class provides an interface to Thread Local Storage (TLS).
 */
class ThreadLocal {
#ifdef HAVE_POSIX
private:
  pthread_key_t key;

public:
  ThreadLocal() {
    ::pthread_key_create(&key, NULL);
  }

  ~ThreadLocal() {
    ::pthread_key_delete(key);
  }

  void *get() const {
    return ::pthread_getspecific(key);
  }

  void set(void *value) {
    ::pthread_setspecific(key, value);
  }
#else /* !HAVE_POSIX */
private:
  DWORD tls_index;

public:
  ThreadLocal():tls_index(::TlsAlloc()) {}
  ~ThreadLocal() {
    ::TlsFree(tls_index);
  }

  void *get() const {
    return ::TlsGetValue(tls_index);
  }

  void set(void *value) {
    ::TlsSetValue(tls_index, value);
  }
#endif /* !HAVE_POSIX */

  operator void*() const {
    return get();
  }

  void *operator =(void *value) {
    set(value);
    return value;
  }
};

/**
 * A wrapper for #ThreadLocal which manages a user specified type.
 */
template<class T>
class ThreadLocalObject : private ThreadLocal {
public:
  const T get() const {
    return (T)(long)ThreadLocal::get();
  }

  void set(const T value) {
    ThreadLocal::set((void *)(long)value);
  }

  operator T() const {
    return get();
  }

  const T operator =(const T value) {
    set(value);
    return value;
  }
};

/**
 * A wrapper for #ThreadLocal which manages a user specified type.
 */
class ThreadLocalInteger : public ThreadLocalObject<int> {
public:
  int operator ++() {
    int value = get();
    set(++value);
    return value;
  }

  int operator --() {
    int value = get();
    set(--value);
    return value;
  }
};

#endif
