/*
 * Copyright (C) 2010 Max Kellermann <max@duempel.org>
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

#ifndef XCSOAR_STATIC_STRING_HPP
#define XCSOAR_STATIC_STRING_HPP

#include "Util/StringUtil.hpp"

#include <algorithm>

#include <assert.h>
#include <stddef.h>
#include <tchar.h>

/**
 * A string with a maximum size known at compile time.
 */
template<typename T, size_t max>
class StaticStringBase
{
public:
  typedef size_t size_type;

  static constexpr size_type MAX_SIZE = max;
  static constexpr T SENTINEL = '\0';

protected:
  T data[max];

public:
  StaticStringBase() = default;
  explicit StaticStringBase(const T *value) {
    set(value);
  }

  size_type length() const {
    return StringLength(data);
  }

  bool empty() const {
    return data[0] == SENTINEL;
  }

  bool full() const {
    return length() >= MAX_SIZE - 1;
  }

  void clear() {
    data[0] = SENTINEL;
  }

  /**
   * Truncate the string to the specified length.
   *
   * @param new_length the new length; must be equal or smaller than
   * the current length
   */
  void Truncate(size_type new_length) {
    assert(new_length <= length());

    data[new_length] = SENTINEL;
  }

  void SetASCII(const char *src, const char *src_end) {
    T *end = ::CopyASCII(data, MAX_SIZE - 1, src, src_end);
    *end = SENTINEL;
  }

  void SetASCII(const char *src) {
    SetASCII(src, src + StringLength(src));
  }

#ifdef _UNICODE
  void SetASCII(const TCHAR *src, const TCHAR *src_end) {
    T *end = ::CopyASCII(data, MAX_SIZE - 1, src, src_end);
    *end = SENTINEL;
  }

  void SetASCII(const TCHAR *src) {
    SetASCII(src, src + StringLength(src));
  }
#endif

  /**
   * Eliminate all non-ASCII characters.
   */
  void CleanASCII() {
    CopyASCII(data, data);
  }

  bool equals(const T *other) const {
    assert(other != nullptr);

    return StringIsEqual(data, other);
  }

  gcc_pure
  bool StartsWith(const T *prefix) const {
    return StringStartsWith(data, prefix);
  }

  gcc_pure
  bool Contains(const T *needle) const {
    return StringFind(data, needle) != nullptr;
  }

  /**
   * Returns a writable buffer.
   */
  T *buffer() {
    return data;
  }

  /**
   * Returns one character.  No bounds checking.
   */
  T operator[](size_type i) const {
    assert(i <= length());

    return data[i];
  }

  /**
   * Returns one writable character.  No bounds checking.
   */
  T &operator[](size_type i) {
    assert(i <= length());

    return data[i];
  }

  const T *begin() const {
    return data;
  }

  const T *end() const {
    return data + length();
  }

  T last() const {
    assert(length() > 0);

    return data[length() - 1];
  }

  const T *get() const {
    return data;
  }

  void set(const T *new_value) {
    assert(new_value != nullptr);

    CopyString(data, new_value, MAX_SIZE);
  }

  void set(const T *new_value, size_type length) {
    assert(new_value != nullptr);

    size_type max_length = (MAX_SIZE < length + 1) ? MAX_SIZE : length + 1;
    CopyString(data, new_value, max_length);
  }

  void append(const T *new_value) {
    assert(new_value != nullptr);

    size_type len = length();
    CopyString(data + len, new_value, MAX_SIZE - len);
  }

  void append(const T *new_value, size_type _length) {
    assert(new_value != nullptr);

    size_type len = length();
    size_type max_length = (MAX_SIZE - len < _length + 1) ?
                           MAX_SIZE - len : _length + 1;
    CopyString(data + len, new_value, max_length);
  }

  bool Append(T ch) {
    size_t l = length();
    if (l >= MAX_SIZE - 1)
      return false;

    data[l] = ch;
    data[l + 1] = SENTINEL;
    return true;
  }

  /**
   * Append ASCII characters from the specified string without buffer
   * boundary checks.
   */
  void UnsafeAppendASCII(const char *p) {
    CopyASCII(data + length(), p);
  }

  const T *c_str() const {
    return get();
  }

  operator const T *() const {
    return get();
  }

  bool operator ==(const T *value) const {
    return equals(value);
  }

  bool operator !=(const T *value) const {
    return !equals(value);
  }

  StaticStringBase<T, max> &operator =(const T *new_value) {
    set(new_value);
    return *this;
  }

  StaticStringBase<T, max> &operator +=(const T *new_value) {
    append(new_value);
    return *this;
  }

  StaticStringBase<T, max> &operator +=(T ch) {
    Append(ch);
    return *this;
  }

  /**
   * Don't use - not thread safe.
   */
  T *first_token(const T *delim) {
    return StringToken(data, delim);
  }

  /**
   * Don't use - not thread safe.
   */
  T *next_token(const T *delim) {
    return StringToken(nullptr, delim);
  }

  /**
   * Use snprintf() to set the value of this string.  The value is
   * truncated if it is too long for the buffer.
   */
  template<typename... Args>
  void Format(const T *fmt, Args&&... args) {
    StringFormat(data, MAX_SIZE, fmt, args...);
  }

  /**
   * Use snprintf() to append to this string.  The value is truncated
   * if it would become too long for the buffer.
   */
  template<typename... Args>
  void AppendFormat(const T *fmt, Args&&... args) {
    size_t l = length();
    StringFormat(data + l, MAX_SIZE - l, fmt, args...);
  }

  /**
   * Use sprintf() to set the value of this string.  WARNING: this
   * does not check if the new value fits into the buffer, and might
   * overflow.  Use only when you are sure that the buffer is big
   * enough!
   */
  template<typename... Args>
  void UnsafeFormat(const T *fmt, Args&&... args) {
    StringFormatUnsafe(data, fmt, args...);
  }
};

/**
 * A string with a maximum size known at compile time.
 * This is the TCHAR-based sister of the NarrowString class.
 */
template<size_t max>
class StaticString: public StaticStringBase<TCHAR, max>
{
public:
  StaticString() = default;
  explicit StaticString(const TCHAR *value):StaticStringBase<TCHAR, max>(value) {}

  StaticString<max> &operator =(const TCHAR *new_value) {
    return (StaticString<max> &)StaticStringBase<TCHAR, max>::operator =(new_value);
  }

  StaticString<max> &operator +=(const TCHAR *new_value) {
    return (StaticString<max> &)StaticStringBase<TCHAR, max>::operator +=(new_value);
  }

  StaticString<max> &operator +=(TCHAR ch) {
    return (StaticString<max> &)StaticStringBase<TCHAR, max>::operator +=(ch);
  }
};

/**
 * A string with a maximum size known at compile time.
 * This is the char-based sister of the StaticString class.
 */
template<size_t max>
class NarrowString: public StaticStringBase<char, max>
{
public:
  NarrowString() = default;
  explicit NarrowString(const char *value):StaticStringBase<char, max>(value) {}

  NarrowString<max> &operator =(const char *new_value) {
    return (NarrowString<max> &)StaticStringBase<char, max>::operator =(new_value);
  }

  NarrowString<max> &operator +=(const char *new_value) {
    return (NarrowString<max> &)StaticStringBase<char, max>::operator +=(new_value);
  }

  NarrowString<max> &operator +=(char ch) {
    return (NarrowString<max> &)StaticStringBase<char, max>::operator +=(ch);
  }
};

#endif
