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
#include <memory>

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

  TextCacheKey(TextCacheKey &&other) noexcept
    :font(other.font),
     text(other.text), allocated(other.allocated) {
    other.allocated = nullptr;
  }

  TextCacheKey(const Font &_font, StringView _text) noexcept
    :font(&_font), text(_text), allocated(nullptr) {}

  ~TextCacheKey() noexcept {
    free(allocated);
  }

  /**
   * Copy the "text" attribute.  This must be called before inserting
   * this key into the #Cache.
   */
  void Allocate() noexcept {
    assert(allocated == nullptr);

    text.data = allocated = strndup(text.data, text.size);
  }

  TextCacheKey &operator=(const TextCacheKey &other) = delete;

  TextCacheKey &operator=(TextCacheKey &&other) noexcept {
    font = other.font;
    text = other.text;
    std::swap(allocated, other.allocated);
    return *this;
  }

  [[gnu::pure]]
  bool operator==(const TextCacheKey &other) const noexcept {
    return font == other.font &&
      text.Equals(other.text);
  }

  struct StringHash {
    [[gnu::pure]]
    size_t operator()(StringView s) const noexcept {
      /* code copied from libstdc++ backward/hash_fun.h */
      unsigned long __h = 0;
      for (const auto ch : s)
        __h = 5 * __h + ch;
      return size_t(__h);
    }
  };

  struct Hash {
    StringHash string_hash;

    [[gnu::pure]]
    size_t operator()(const TextCacheKey &key) const noexcept {
      return (size_t)(const void *)key.font
        ^ string_hash(key.text);
    }
  };
};

struct RenderedText {
#ifdef ENABLE_OPENGL
  std::unique_ptr<GLTexture> texture;
#else
  std::unique_ptr<uint8_t[]> data;
  PixelSize size;
#endif

  RenderedText(const RenderedText &other) = delete;

#ifdef ENABLE_OPENGL
#if defined(USE_FREETYPE) || defined(USE_APPKIT) || defined(USE_UIKIT)
  RenderedText(PixelSize size, const uint8_t *buffer) noexcept
    :texture(new GLTexture(GL_ALPHA, size,
                           GL_ALPHA, GL_UNSIGNED_BYTE,
                           buffer))
  {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  }
#elif defined(ANDROID)
  RenderedText(int id, PixelSize size, PixelSize allocated_size) noexcept
    :texture(new GLTexture(id, size, allocated_size)) {}
#endif
#else
  RenderedText(PixelSize _size, std::unique_ptr<uint8_t[]> &&_data) noexcept
    :data(std::move(_data)), size(_size) {}
#endif

  RenderedText &operator=(const RenderedText &other) = delete;

  RenderedText(RenderedText &&other) noexcept = default;
  RenderedText &operator=(RenderedText &&other) noexcept = default;

  operator TextCache::Result() const noexcept {
#ifdef ENABLE_OPENGL
    return texture.get();
#else
    return { data.get(), size.width, size };
#endif
  }

  [[gnu::pure]]
  PixelSize GetSize() const noexcept {
#ifdef ENABLE_OPENGL
    return texture->GetSize();
#else
    return size;
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
  size_cache.Put(std::move(key), size);
  return size;
}

PixelSize
TextCache::LookupSize(const Font &font, StringView text) noexcept
{
#ifndef ENABLE_OPENGL
  const std::lock_guard<Mutex> lock(text_cache_mutex);
#endif

  if (text.empty())
    return {};

  TextCacheKey key(font, text);
  const RenderedText *cached = text_cache.Get(key);
  if (cached == nullptr)
    return {};

  return cached->GetSize();
}

TextCache::Result
TextCache::Get(const Font &font, StringView text) noexcept
{
#ifdef ENABLE_OPENGL
  assert(pthread_equal(pthread_self(), OpenGL::thread));
#endif
  assert(font.IsDefined());

  if (text.empty())
    return nullptr;

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
  if (buffer_size == 0)
    return nullptr;

  std::unique_ptr<uint8_t[]> buffer{new uint8_t[buffer_size]};

  font.Render(text2, size, buffer.get());
#ifdef ENABLE_OPENGL
  RenderedText rt(size, buffer.get());
#else
  RenderedText rt(size, std::move(buffer));
#endif

#elif defined(ANDROID)
  PixelSize size, allocated_size;
  int texture_id = font.TextTextureGL(text, size, allocated_size);
  if (texture_id == 0)
    return nullptr;

  RenderedText rt(texture_id, size, allocated_size);
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
TextCache::Flush() noexcept
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
