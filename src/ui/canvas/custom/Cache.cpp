/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#include "Cache.hpp"
#include "ui/dim/Size.hpp"
#include "ui/canvas/Font.hpp"
#include "util/Cache.hxx"
#include "util/StringCompare.hxx"
#include "util/StringAPI.hxx"
#include "util/StringView.hxx"
#include "util/TStringView.hxx"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Texture.hpp"
#include "ui/canvas/opengl/Debug.hpp"
#else
#include "thread/Mutex.hxx"
#endif

#ifdef UNICODE
#include "util/ConvertString.hpp"
#endif

#include <cassert>

/**
 * A key type for the Cache template class.  It can be operated in two
 * modes: the zero-allocation mode is used by default; it stores a
 * pointer to the original string.  A key that is used to actually
 * store the element must contain a copy of the string, just in case
 * the original string goes out of scope.  The method Allocate()
 * creates that copy.
 */
struct TextCacheKey {
  const Font *font;
  StringView text;
  char *allocated;

  TextCacheKey(const TextCacheKey &other) = delete;

  TextCacheKey(TextCacheKey &&other)
    :font(other.font),
     text(other.text), allocated(other.allocated) {
    other.allocated = nullptr;
  }

  TextCacheKey(const Font &_font, StringView _text) noexcept
    :font(&_font), text(_text), allocated(nullptr) {}

  ~TextCacheKey() {
    free(allocated);
  }

  /**
   * Copy the "text" attribute.  This must be called before inserting
   * this key into the #Cache.
   */
  void Allocate() {
    assert(allocated == nullptr);

    text.data = allocated = strndup(text.data, text.size);
  }

  TextCacheKey &operator=(const TextCacheKey &other) = delete;

  TextCacheKey &operator=(TextCacheKey &&other) {
    font = other.font;
    text = other.text;
    std::swap(allocated, other.allocated);
    return *this;
  }

  gcc_pure
  bool operator==(const TextCacheKey &other) const {
    return font == other.font &&
      text.Equals(other.text);
  }

  struct StringHash {
    gcc_pure
    size_t operator()(StringView s) const {
      /* code copied from libstdc++ backward/hash_fun.h */
      unsigned long __h = 0;
      for (const auto ch : s)
        __h = 5 * __h + ch;
      return size_t(__h);
    }
  };

  struct Hash {
    StringHash string_hash;

    gcc_pure
    size_t operator()(const TextCacheKey &key) const {
      return (size_t)(const void *)key.font
        ^ string_hash(key.text);
    }
  };
};

struct RenderedText {
#ifdef ENABLE_OPENGL
  GLTexture *texture;
#else
  uint8_t *data;
  unsigned width, height;
#endif

  RenderedText(const RenderedText &other) = delete;

#ifdef ENABLE_OPENGL
  RenderedText(RenderedText &&other)
    :texture(other.texture) {
    other.texture = nullptr;
  }

#if defined(USE_FREETYPE) || defined(USE_APPKIT) || defined(USE_UIKIT)
  RenderedText(unsigned width, unsigned height, const uint8_t *buffer) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    texture = new GLTexture(GL_ALPHA, PixelSize(width, height),
                            GL_ALPHA, GL_UNSIGNED_BYTE,
                            buffer);
  }
#elif defined(ANDROID)
  RenderedText(int id, unsigned width, unsigned height,
               unsigned allocated_width, unsigned allocated_height)
    :texture(new GLTexture(id, PixelSize(width, height),
                           PixelSize(allocated_width, allocated_height))) {}
#endif
#else
  RenderedText(RenderedText &&other)
    :data(other.data), width(other.width), height(other.height) {
    other.data = nullptr;
  }

  RenderedText(unsigned _width, unsigned _height, uint8_t *_data)
    :data(_data), width(_width), height(_height) {}
#endif

  ~RenderedText() {
#ifdef ENABLE_OPENGL
    delete texture;
#else
    delete[] data;
#endif
  }

  RenderedText &operator=(const RenderedText &other) = delete;

  RenderedText &operator=(RenderedText &&other) {
#ifdef ENABLE_OPENGL
    std::swap(texture, other.texture);
#else
    std::swap(data, other.data);
    width = other.width;
    height = other.height;
#endif
    return *this;
  }

  operator TextCache::Result() const {
#ifdef ENABLE_OPENGL
    return texture;
#else
    return { data, width, width, height };
#endif
  }
};

#ifndef ENABLE_OPENGL
/**
 * Without OpenGL, this library is accessed from DrawThread and UI
 * thread, therefore we need to protect it.
 */
static Mutex text_cache_mutex;
#endif

static Cache<TextCacheKey, PixelSize, 1024u, 701u, TextCacheKey::Hash> size_cache;
static Cache<TextCacheKey, RenderedText, 256u, 211u, TextCacheKey::Hash> text_cache;

PixelSize
TextCache::GetSize(const Font &font, StringView text) noexcept
{
#ifndef ENABLE_OPENGL
  const std::lock_guard<Mutex> lock(text_cache_mutex);
#endif

  TextCacheKey key(font, text);
  const PixelSize *cached = size_cache.Get(key);
  if (cached != nullptr)
    return *cached;

#ifdef UNICODE
  PixelSize size = font.TextSize(UTF8ToWideConverter(text));
#else
  PixelSize size = font.TextSize(text);
#endif

  key.Allocate();
  size_cache.Put(std::move(key), std::move(size));
  return size;
}

PixelSize
TextCache::LookupSize(const Font &font, StringView text) noexcept
{
#ifndef ENABLE_OPENGL
  const std::lock_guard<Mutex> lock(text_cache_mutex);
#endif

  PixelSize size = { 0, 0 };

  if (text.empty())
    return size;

  TextCacheKey key(font, text);
  const RenderedText *cached = text_cache.Get(key);
  if (cached == nullptr)
    return size;

#ifdef ENABLE_OPENGL
  return cached->texture->GetSize();
#else
  return { cached->width, cached->height };
#endif
}

TextCache::Result
TextCache::Get(const Font &font, StringView text) noexcept
{
#ifdef ENABLE_OPENGL
  assert(pthread_equal(pthread_self(), OpenGL::thread));
#endif
  assert(font.IsDefined());

  if (text.empty()) {
#ifdef ENABLE_OPENGL
    return nullptr;
#else
    return Result::Null();
#endif
  }

  TextCacheKey key(font, text);

  /* look it up */

#ifndef ENABLE_OPENGL
  const std::lock_guard<Mutex> lock(text_cache_mutex);
#endif

  const RenderedText *cached = text_cache.Get(key);
  if (cached != nullptr)
    return *cached;

  /* render the text into a OpenGL texture */

#if defined(USE_FREETYPE) || defined(USE_APPKIT) || defined(USE_UIKIT)
#ifdef UNICODE
  UTF8ToWideConverter text2(text);
#else
  TStringView text2 = text;
#endif
  PixelSize size = font.TextSize(text2);
  size_t buffer_size = font.BufferSize(size);
  if (buffer_size == 0) {
#ifdef ENABLE_OPENGL
    return nullptr;
#else
    return Result::Null();
#endif
  }

  uint8_t *buffer = new uint8_t[buffer_size];
  if (buffer == nullptr) {
#ifdef ENABLE_OPENGL
    return nullptr;
#else
    return Result::Null();
#endif
  }

  font.Render(text2, size, buffer);
  RenderedText rt(size.width, size.height, buffer);
#ifdef ENABLE_OPENGL
  delete[] buffer;
#endif
#elif defined(ANDROID)
  PixelSize size, allocated_size;
  int texture_id = font.TextTextureGL(text, size, allocated_size);
  if (texture_id == 0)
    return nullptr;

  RenderedText rt(texture_id, size.width, size.height,
                  allocated_size.width, allocated_size.height);
#else
#error No font renderer
#endif

  Result result = rt;

  key.Allocate();
  text_cache.Put(std::move(key), std::move(rt));

  /* done */

  return result;
}

void
TextCache::Flush()
{
#ifdef ENABLE_OPENGL
  assert(pthread_equal(pthread_self(), OpenGL::thread));
#endif

#ifndef ENABLE_OPENGL
  const std::lock_guard<Mutex> lock(text_cache_mutex);
#endif

  size_cache.Clear();
  text_cache.Clear();
}
