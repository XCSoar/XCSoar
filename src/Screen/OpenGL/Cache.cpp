/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Screen/OpenGL/Cache.hpp"
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Debug.hpp"
#include "Screen/OpenGL/Point.hpp"
#include "Screen/Font.hpp"
#include "Screen/Color.hpp"
#include "Util/ListHead.hpp"
#include "Util/Cache.hpp"

#include <unordered_map>
#include <string>
#include <assert.h>

struct TextCacheKey {
  const Font *font;
  UPixelScalar height;
  std::string text;

  TextCacheKey(const Font &_font, const char *_text)
    :font(&_font), height(_font.GetHeight()), text(_text) {}

  gcc_pure
  bool operator==(const TextCacheKey &other) const {
    return font == other.font && height == other.height && text == other.text;
  }

  struct Hash {
    std::hash<std::string> string_hash;

    gcc_pure
    size_t operator()(const TextCacheKey &key) const {
      return (size_t)(const void *)key.font ^ key.height
        ^ string_hash(key.text);
    }
  };
};

struct RenderedText : public ListHead {
  TextCacheKey key;
  GLTexture texture;

#ifdef ANDROID
  RenderedText(TextCacheKey &_key, int id, unsigned width, unsigned height)
    :key(_key), texture(id, width, height) {}
#else
  RenderedText(TextCacheKey &_key, SDL_Surface *surface)
    :key(_key), texture(surface) {}
#endif
};

static Cache<TextCacheKey, PixelSize, 1024u, TextCacheKey::Hash> size_cache;
static std::unordered_map<TextCacheKey, RenderedText *, TextCacheKey::Hash> text_cache_map;
static ListHead text_cache_head = ListHead(ListHead::empty());
static unsigned text_cache_size = 0;

PixelSize
TextCache::GetSize(const Font &font, const char *text)
{
  TextCacheKey key(font, text);
  const PixelSize *cached = size_cache.Get(key);
  if (cached != NULL)
    return *cached;

  PixelSize size = font.TextSize(text);
  size_cache.Put(key, size);
  return size;
}

PixelSize
TextCache::LookupSize(const Font &font, const char *text)
{
  PixelSize size = { 0, 0 };

  if (*text == 0)
    return size;

  TextCacheKey key(font, text);
  auto i = text_cache_map.find(key);
  if (i == text_cache_map.end())
    return size;

  const RenderedText &rendered = *i->second;
  size.cx = rendered.texture.get_width();
  size.cy = rendered.texture.get_height();
  return size;
}

GLTexture *
TextCache::get(const Font *font, const char *text)
{
  assert(pthread_equal(pthread_self(), OpenGL::thread));
  assert(font != NULL);
  assert(text != NULL);

  if (*text == 0)
    return NULL;

  TextCacheKey key(*font, text);

  /* look it up */

  auto i = text_cache_map.find(key);
  if (i != text_cache_map.end()) {
    /* found in the cache */
    RenderedText *rt = i->second;

    /* move to the front, so it gets flushed last */
    rt->MoveAfter(text_cache_head);

    return &rt->texture;
  }

  /* remove old entries from cache */

  if (text_cache_size >= 256) {
    RenderedText *rt = (RenderedText *)text_cache_head.GetPrevious();
    text_cache_map.erase(rt->key);
    rt->Remove();
    delete rt;
    --text_cache_size;
  }

  /* render the text into a OpenGL texture */

  const Color background_color = COLOR_BLACK;
  const Color text_color = COLOR_WHITE;

#ifdef ANDROID
  PixelSize size;
  int texture_id = font->TextTextureGL(text, size, text_color, background_color);
  if (texture_id == 0)
    return NULL;

  RenderedText *rt = new RenderedText(key, texture_id, size.cx, size.cy);
#else
  SDL_Surface *surface = ::TTF_RenderUTF8_Solid(font->Native(), text,
                                                COLOR_BLACK);
  if (surface == NULL)
    return NULL;

  surface->flags &= ~SDL_SRCCOLORKEY;
  if (surface->format->palette != NULL &&
      surface->format->palette->ncolors >= 2) {
    surface->format->palette->colors[0] = background_color;
    surface->format->palette->colors[1] = text_color;
  }

  /* insert into cache */

  RenderedText *rt = new RenderedText(key, surface);
  SDL_FreeSurface(surface);
#endif

  rt->InsertAfter(text_cache_head);

  text_cache_map.insert(std::make_pair(key, rt));

  ++text_cache_size;

  /* done */

  return &rt->texture;
}

void
TextCache::flush()
{
  assert(pthread_equal(pthread_self(), OpenGL::thread));

  size_cache.Clear();
  text_cache_map.clear();

  for (RenderedText *rt = (RenderedText *)text_cache_head.GetNext();
       rt != &text_cache_head;) {
    RenderedText *next = (RenderedText *)rt->GetNext();
    delete rt;
    rt = next;
  }

  text_cache_head.Clear();
  text_cache_size = 0;
}
