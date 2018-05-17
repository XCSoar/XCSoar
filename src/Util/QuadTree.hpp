/*
 * Copyright (C) 2011 Max Kellermann <max.kellermann@gmail.com>
 *               2012 Tobias Bieniek <tobias.bieniek@gmx.de>
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

#ifndef QUAD_TREE_HPP
#define QUAD_TREE_HPP

#include "Compiler.h"

#include <utility>
#include <limits>
#include <memory>

#include <assert.h>

/**
 * A quad tree implementation.  It provides quick searches on a two
 * dimensional space.  It is used to store objects on earth's surface.
 *
 * The QuadTree needs to know its own bounds.  Initially, the bounds
 * are undefined.  During that initial phase, values may be added with
 * AddQuick() or AddScan() to the root bucket, which is not going to
 * be splitted.  Once the bounds are known and valid, you can add new
 * element with AddDeep(), but only if the new element fits into the
 * existing bounds.  If not, you have to "flatten" the QuadTree and
 * rescan the bounds.
 *
 * Elements may be removed at any time, but the bounds are not
 * modified then.
 *
 * @see http://en.wikipedia.org/wiki/Quadtree
 */
template<typename T, typename Accessor,
         typename Alloc = std::allocator<T> >
class QuadTree {
  struct AlwaysTrue {
    constexpr
    bool operator()(const T &) const {
      return true;
    }
  };

  /**
   * Split a bucket when it has this number of leaves.
   */
  static constexpr unsigned SPLIT_THRESHOLD = 16;

public:
  typedef int position_type;
  typedef unsigned distance_type;

  /**
   * Returns the maximum value for the numeric type #distance_type.
   */
  constexpr
  static inline distance_type max_distance(){
    return std::numeric_limits<distance_type>::max();
  }

  class iterator;
  class const_iterator;

  /**
   * Calculate the square of this number.
   */
  constexpr
  static distance_type Square(distance_type x) {
    return x * x;
  }

  /**
   * Overload of Square(distance_type) which takes a signed integer.
   * Without this overload, the square of a negative number cannot be
   * calculated.
   */
  constexpr
  static distance_type Square(position_type x) {
    return x * x;
  }

  /**
   * A location on the plane.
   */
  struct Point {
    position_type x, y;

    constexpr
    Point(position_type _x, position_type _y):x(_x), y(_y) {}

    /**
     * Calculate the square distance to another point.
     */
    constexpr
    distance_type SquareDistanceTo(const Point &other) const {
      return Square(other.x - x) + Square(other.y - y);
    }

    constexpr
    bool operator==(const Point &other) const {
      return x == other.x && y == other.y;
    }

    constexpr
    bool operator!=(const Point &other) const {
      return !(*this == other);
    }
  };

  /**
   * A rectangle on the plane that is parallel to the X and Y axes.
   */
  struct Rectangle {
    position_type left, top, right, bottom;

    /**
     * Non-initialising contructor.
     */
    Rectangle() = default;

    constexpr
    Rectangle(position_type _left, position_type _top,
              position_type _right, position_type _bottom)
      :left(_left), top(_top), right(_right), bottom(_bottom) {}

    constexpr
    bool IsEmpty() const {
      return left == right || top == bottom;
    }

    /**
     * Initialise all attributes with zero, making this object
     * "empty".
     */
    void Clear() {
      left = top = right = bottom = 0;
    }

    /**
     * Is this rectangle big enough so it can be splitted?
     */
    constexpr
    bool CanSplit() const {
      return left + 2 <= right && top + 2 <= bottom;
    }

    constexpr
    Point GetMiddle() const {
      return Point((left + right) / 2, (top + bottom) / 2);
    }

    constexpr
    bool IsInside(const Point &point) const {
      return point.x >= left && point.x <= right &&
        point.y >= top && point.y <= bottom;
    }

    /**
     * Make this rectangle empty, at the specified position.
     */
    void Set(const Point &point) {
      left = right = point.x;
      top = bottom = point.y;
    }

    /**
     * Extend the bounds of this rectangle so the specified point is
     * inside.
     */
    void Scan(const Point &point) {
      if (point.x < left)
        left = point.x;

      if (point.x > right)
        right = point.x;

      if (point.y < top)
        top = point.y;

      if (point.y > bottom)
        bottom = point.y;
    }

    constexpr
    distance_type HorizontalDistanceTo(const Point &other) const {
      return other.x < left
        ? left - other.x
        : (other.x > right
           ? other.x - right
           : 0);
    }

    constexpr
    distance_type VerticalDistanceTo(const Point &other) const {
      return other.y < top
        ? top - other.y
        : (other.y > bottom
           ? other.y - bottom
           : 0);
    }

    /**
     * Calculate the minimum square distance of this rectangle to the
     * specified point.  Returns 0 when the point is inside.
     */
    constexpr
    distance_type SquareDistanceTo(const Point &other) const {
      return Square(HorizontalDistanceTo(other)) + Square(VerticalDistanceTo(other));
    }

    constexpr
    bool IsWithinSquareRange(const Point &other, distance_type square_range) const {
      return SquareDistanceTo(other) <= square_range;
    }

    /**
     * Split this rectangle so the specified point is inside the new
     * rectangle.  Returns a pair which specifies the position of the
     * new rectangle inside the old bigger one.
     */
    std::pair<bool,bool> Split(const Point &point) {
      const Point middle = GetMiddle();
      bool x = point.x >= middle.x;
      bool y = point.y >= middle.y;

      if (x)
        left = middle.x;
      else
        right = middle.x;

      if (y)
        top = middle.y;
      else
        bottom = middle.y;

      return std::make_pair(x, y);
    }
  };

  /**
   * Function wrapper for the Accessor.
   */
  constexpr
  static Point GetPosition(const T &value) {
    return Point(Accessor().GetX(value),
                 Accessor().GetY(value));
  }

protected:
  /**
   * An envelope for a value object.
   */
  struct Leaf {
    /** linked list */
    Leaf *next;

    /** the actual value */
    T value;

    template<typename U>
    explicit Leaf(U &&_value):value(std::forward<U>(_value)) {}

    constexpr
    Point GetPosition() const {
      return QuadTree<T,Accessor,Alloc>::GetPosition(value);
    }

    constexpr
    distance_type SquareDistanceTo(const Point &other) const {
      return GetPosition().SquareDistanceTo(other);
    }

    constexpr
    bool InSquareRange(const Point &ref, distance_type square_range) const {
      return GetPosition().SquareDistanceTo(ref) <= square_range;
    }
  };

  typedef typename Alloc::template rebind<Leaf>::other LeafAllocator;

  struct LeafList {
    /* a linked list of values, or nullptr if this is a splitted bucket */
    Leaf *head;
    unsigned size;

    LeafList():head(nullptr), size(0) {}

    ~LeafList() {
      assert(head == nullptr);
      assert(size == 0);
    }

    constexpr
    bool IsEmpty() const {
      return head == nullptr;
    }

    constexpr
    unsigned GetSize() const {
      return size;
    }

    /**
     * Add the specified leaf to the list.
     */
    void Add(Leaf *leaf) {
      leaf->next = head;
      head = leaf;
      ++size;
    }

    /**
     * Remove the specified Leaf from the list, and return a non-const
     * pointer to it.  It is not freed.
     */
    Leaf *Remove(const Leaf *leaf) {
      assert(size > 0);

      Leaf **p = &head;
      while (*p != leaf) {
        assert(*p != nullptr);
        p = &(*p)->next;
      }

      Leaf *deconst_leaf = *p;
      *p = leaf->next;

      assert(size > 0);
      --size;
      return deconst_leaf;
    }

    /**
     * Remove the topmost Leaf from the list and returns it.  It is
     * not freed.
     */
    Leaf *Pop() {
      assert(!IsEmpty());

      Leaf *leaf = head;
      head = head->next;
      --size;
      return leaf;
    }

    /**
     * Remove and free all Leaf objects in the list.
     */
    void Clear(LeafAllocator &allocator) {
      Leaf *leaf = head;
      while (leaf != nullptr) {
        Leaf *next = leaf->next;
        allocator.destroy(leaf);
        allocator.deallocate(leaf, 1);
        leaf = next;
      }

      head = nullptr;
      size = 0;
    }

    template<class P>
    void EraseIf(const P &predicate, LeafAllocator &leaf_allocator) {
      Leaf **p = &head;
      while (true) {
        Leaf *leaf = *p;
        if (leaf == nullptr)
          break;

        if (predicate(leaf->value)) {
          assert(size > 0);
          --size;

          *p = leaf->next;
          leaf_allocator.destroy(leaf);
          leaf_allocator.deallocate(leaf, 1);
        } else
          p = &leaf->next;
      }
    }

    /**
     * Move all Leaf objects from the specified list to this one.
     */
    void MoveAllFrom(LeafList &other) {
      Leaf *last = other.head;
      if (last == nullptr)
        return;

      while (last->next != nullptr)
        last = last->next;

      last->next = head;
      head = other.head;
      size += other.size;
      other.head = nullptr;
      other.size = 0;
    }

    gcc_pure
    const Leaf &FindPointer(const T *value) const {
      assert(value != nullptr);

      const Leaf *i = head;
      while (true) {
        if (&i->value == value)
          return *i;

        i = i->next;
        assert(i != nullptr);
      }
    }

    template<class P>
    gcc_pure
    std::pair<const Leaf *, distance_type>
    FindNearestIf(const Point location, distance_type square_range,
                  const P &predicate) const {
      const Leaf *nearest = nullptr;
      distance_type nearest_square_distance = square_range;

      for (const Leaf *i = head; i != nullptr; i = i->next) {
        if (!predicate(i->value))
          continue;

        distance_type square_distance = i->SquareDistanceTo(location);
        if (square_distance <= nearest_square_distance) {
          nearest_square_distance = square_distance;
          nearest = i;
        }
      }

      return std::make_pair(nearest, nearest_square_distance);
    }

    template<class V>
    void VisitWithinRange(const Point location, distance_type square_range,
                          V &visitor) const {
      for (Leaf *leaf = head; leaf != nullptr; leaf = leaf->next)
        if (leaf->InSquareRange(location, square_range))
          visitor((const T &)leaf->value);
    }
  };

  struct QuadBucket;
  typedef typename Alloc::template rebind<QuadBucket>::other BucketAllocator;

  /**
   * A rectangular partition of the plane.  A bucket can be "splitted"
   * and contains more buckets.  A "leaf" bucket (one that is not
   * splitted) may contain a linked list of Leaf objects (values).
   * Only the root bucket may be completely empty (when the whole
   * QuadTree is empty).
   */
  struct Bucket {
    /* the parent bucket */
    Bucket *parent;

    /* exactly 4 child buckets, or nullptr if this is a leaf bucket */
    QuadBucket *children;

    /* a list of values; must be empty in a "splitted" bucket */
    LeafList leaves;

    Bucket()
      :parent(nullptr), children(nullptr) {}

    ~Bucket() {
      assert(IsEmpty());
    }

    constexpr
    bool IsEmpty() const {
      return children == nullptr && leaves.IsEmpty();
    }

    constexpr
    bool IsSplitted() const {
      return children != nullptr;
    }

    /**
     * Returns the (deep) number of leaves in this bucket.
     */
    gcc_pure
    unsigned GetSize() const {
      return IsSplitted()
        ? children->GetSize()
        : leaves.GetSize();
    }

    const Bucket *GetRoot() const {
      const Bucket *bucket = this;
      while (bucket->parent != nullptr)
        bucket = bucket->parent;
      return bucket;
    }

    /**
     * Dispose all child buckets and values.
     */
    void Clear(BucketAllocator &bucket_allocator,
               LeafAllocator &leaf_allocator) {
      assert(children == nullptr || leaves.IsEmpty());

      if (IsSplitted()) {
        children->Clear(bucket_allocator, leaf_allocator);
        bucket_allocator.destroy(children);
        bucket_allocator.deallocate(children, 1);
        children = nullptr;
      } else
        leaves.Clear(leaf_allocator);
    }

    /**
     * Add the specified Leaf directly into this bucket, without
     * further checks.  Not legal on splitted buckets.
     */
    void AddHere(Leaf *leaf) {
      assert(!IsSplitted());

      leaves.Add(leaf);
    }

    /**
     * Split this bucket into four children, and distribute the leaves
     * to them.
     */
    void Split(const Point middle, BucketAllocator &bucket_allocator) {
      assert(!IsSplitted());

      children = bucket_allocator.allocate(1);
      bucket_allocator.construct(children, QuadBucket(this));

      while (!leaves.IsEmpty()) {
        Leaf *leaf = leaves.Pop();

        const Point position = leaf->GetPosition();
        Bucket &bucket = children->Get(position.x >= middle.x,
                                       position.y >= middle.y);
        assert(!bucket.IsSplitted());
        bucket.AddHere(leaf);
      }
    }

    Bucket &GetChildAt(Rectangle &bounds, Point location) const {
      assert(IsSplitted());
      assert(bounds.IsInside(location));

      const auto xy = bounds.Split(location);
      return children->Get(xy.first, xy.second);
    }

    /**
     * Add the specified Leaf object to this bucket or a child bucket;
     * splits the bucket automatically when it grows too large.
     */
    void Add(Rectangle &bounds, Leaf *leaf,
             BucketAllocator &bucket_allocator) {
      if (IsSplitted()) {
        Bucket &child = GetChildAt(bounds, GetPosition(leaf->value));
        return child.Add(bounds, leaf, bucket_allocator);
      } else {
        AddHere(leaf);

        if (leaves.GetSize() > SPLIT_THRESHOLD && bounds.CanSplit())
          Split(bounds.GetMiddle(), bucket_allocator);
      }
    }

    Leaf *Remove(const Leaf *leaf) {
      assert(leaf != nullptr);

      return leaves.Remove(leaf);
    }

    void Erase(const Leaf *cleaf, LeafAllocator &leaf_allocator) {
      Leaf *leaf = Remove(cleaf);
      leaf_allocator.destroy(leaf);
      leaf_allocator.deallocate(leaf, 1);
    }

    template<class P>
    void EraseIf(const P &predicate, LeafAllocator &leaf_allocator) {
      leaves.EraseIf(predicate, leaf_allocator);

      if (children != nullptr)
        children->EraseIf(predicate, leaf_allocator);
    }

    /**
     * Scan the bounds of all leaves.  Only legal in a "flat"
     * (unsplitted) root bucket.
     */
    void Scan(Rectangle &bounds) {
      assert(parent == nullptr);
      assert(!IsSplitted());

      Leaf *leaf = leaves.head;
      if (leaf == nullptr) {
        bounds.Clear();
        return;
      }

      bounds.Set(GetPosition(leaf->value));
      for (leaf = leaf->next; leaf != nullptr; leaf = leaf->next)
        bounds.Scan(GetPosition(leaf->value));
    }

    void Flatten(BucketAllocator &bucket_allocator) {
      if (!IsSplitted())
        return;

      for (unsigned i = 0; i < children->N; ++i) {
        Bucket &bucket = children->buckets[i];
        bucket.Flatten(bucket_allocator);
        leaves.MoveAllFrom(bucket.leaves);
      }

      bucket_allocator.destroy(children);
      bucket_allocator.deallocate(children, 1);
      children = nullptr;
    }

    void Optimise(const Rectangle &bounds,
                  BucketAllocator &bucket_allocator) {
      if (leaves.GetSize() < SPLIT_THRESHOLD || !bounds.CanSplit())
        return;

      Split(bounds.GetMiddle(), bucket_allocator);
      children->Optimise(bounds, bucket_allocator);
    }

    /**
     * Find the first Bucket in the tree that has at least one Leaf.
     */
    gcc_pure
    const Bucket *FindFirstLeafBucket() const {
      if (!leaves.IsEmpty())
        return this;

      if (children == nullptr)
        return nullptr;

      for (unsigned i = 0; i < QuadBucket::N; ++i) {
        const Bucket *bucket = children->buckets[i].FindFirstLeafBucket();
        if (bucket != nullptr)
          return bucket;
      }

      return nullptr;
    }

    gcc_pure
    const Bucket *FindNextLeafBucket() const {
      assert(children == nullptr);

      const Bucket *current = this, *parent;
      while ((parent = current->parent) != nullptr) {
        const Bucket *sibling = current;
        while ((sibling = parent->children->GetNext(sibling)) != nullptr) {
          assert(sibling->parent == parent);
          const Bucket *bucket = sibling->FindFirstLeafBucket();
          if (bucket != nullptr)
            return bucket;
        }

        current = parent;
      }

      return nullptr;
    }

    gcc_pure
    Bucket *FindFirstLeafBucket() {
      const Bucket *cthis = this;
      return const_cast<Bucket *>(cthis->FindFirstLeafBucket());
    }

    gcc_pure
    Bucket *FindNextLeafBucket() {
      const Bucket *cthis = this;
      return const_cast<Bucket *>(cthis->FindNextLeafBucket());
    }

    gcc_pure
    const_iterator FindPointer(Rectangle &bounds, const T *value) const {
      assert(value != nullptr);
      assert(bounds.IsEmpty() || bounds.IsInside(GetPosition(*value)));
      assert(!bounds.IsEmpty() || parent == nullptr);

      if (IsSplitted()) {
        Bucket &child = GetChildAt(bounds, GetPosition(*value));
        return child.FindPointer(bounds, value);
      } else {
        const Leaf &leaf = leaves.FindPointer(value);
        return const_iterator(this, &leaf);
      }
    }

    template<class P>
    gcc_pure
    std::pair<const_iterator, distance_type>
    FindNearestIf(const Rectangle &bounds,
                  const Point location, distance_type square_range,
                  const P &predicate) const {
      if (!bounds.IsWithinSquareRange(location, square_range))
        return std::make_pair(const_iterator(), max_distance());

      if (IsSplitted()) {
        return children->FindNearestIf(bounds, location, square_range,
                                       predicate);
      } else {
        const auto result =
          leaves.FindNearestIf(location, square_range, predicate);
        if (result.first == nullptr)
          return std::make_pair(const_iterator(), max_distance());

        return std::make_pair(const_iterator(this, result.first),
                              result.second);
      }
    }

    template<class V>
    void VisitWithinRange(const Rectangle &bounds,
                          const Point location, distance_type square_range,
                          V &visitor) const {
      if (!bounds.IsWithinSquareRange(location, square_range))
        return;

      if (IsSplitted())
        children->VisitWithinRange(bounds, location, square_range, visitor);
      else
        leaves.VisitWithinRange(location, square_range, visitor);
    }
  };

  struct QuadBucket {
    static constexpr unsigned N = 4;

    Bucket buckets[N];

    QuadBucket(Bucket *parent) {
      for (unsigned i = 0; i < N; ++i)
        buckets[i].parent = parent;
    }

    gcc_const
    Bucket &Get(bool right, bool bottom) {
      return buckets[(bottom << 1) | right];
    }

    gcc_pure
    const Bucket *GetNext(const Bucket *bucket) {
      assert(bucket != nullptr);
      assert(bucket->parent != nullptr);
      assert(this == bucket->parent->children);
      unsigned i = bucket - buckets;
      assert(i < N);

      return ++i < N ? &buckets[i] : nullptr;
    }

    constexpr
    unsigned GetSize() const {
      return buckets[0].GetSize() + buckets[1].GetSize() +
        buckets[2].GetSize() + buckets[3].GetSize();
    }

    void Clear(BucketAllocator &bucket_allocator,
               LeafAllocator &leaf_allocator) {
      for (unsigned i = 0; i < N; ++i)
        buckets[i].Clear(bucket_allocator, leaf_allocator);
    }

    template<class P>
    void EraseIf(const P &predicate, LeafAllocator &leaf_allocator) {
      for (unsigned i = 0; i < N; ++i)
        buckets[i].EraseIf(predicate, leaf_allocator);
    }

    constexpr
    static Rectangle GetTopLeft(const Rectangle r, const Point middle) {
      return Rectangle(r.left, r.top, middle.x, middle.y);
    }

    constexpr
    static Rectangle GetTopRight(const Rectangle r, const Point middle) {
      return Rectangle(middle.x, r.top, r.right, middle.y);
    }

    constexpr
    static Rectangle GetBottomLeft(const Rectangle r, const Point middle) {
      return Rectangle(r.left, middle.y, middle.x, r.bottom);
    }

    constexpr
    static Rectangle GetBottomRight(const Rectangle r, const Point middle) {
      return Rectangle(middle.x, middle.y, r.right, r.bottom);
    }

    void Optimise(const Rectangle &bounds, BucketAllocator &bucket_allocator) {
      const Point middle = bounds.GetMiddle();

      buckets[0].Optimise(GetTopLeft(bounds, middle), bucket_allocator);
      buckets[1].Optimise(GetTopRight(bounds, middle), bucket_allocator);
      buckets[2].Optimise(GetBottomLeft(bounds, middle), bucket_allocator);
      buckets[3].Optimise(GetBottomRight(bounds, middle), bucket_allocator);
    }

    template<class P>
    gcc_pure
    std::pair<const_iterator, distance_type>
    FindNearestIf(const Rectangle &bounds,
                  const Point location, distance_type square_range,
                  const P &predicate) const {
      const Point middle = bounds.GetMiddle();

      auto result =
        buckets[0].FindNearestIf(GetTopLeft(bounds, middle),
                                 location, square_range,
                                 predicate);

      if (result.first != const_iterator()) {
        assert(result.second <= square_range);
        square_range = result.second;
      }

      auto tmp =
        buckets[1].FindNearestIf(GetTopRight(bounds, middle),
                                 location, square_range,
                                 predicate);
      if (tmp.second < result.second)
        result = tmp;

      if (result.first != const_iterator()) {
        assert(result.second <= square_range);
        square_range = result.second;
      }

      tmp = buckets[2].FindNearestIf(GetBottomLeft(bounds, middle),
                                     location, square_range,
                                     predicate);
      if (tmp.second < result.second)
        result = tmp;

      if (result.first != const_iterator()) {
        assert(result.second <= square_range);
        square_range = result.second;
      }

      tmp = buckets[3].FindNearestIf(GetBottomRight(bounds, middle),
                                     location, square_range,
                                     predicate);
      if (tmp.second < result.second)
        result = tmp;

      return result;
    }

    template<class V>
    void VisitWithinRange(const Rectangle &bounds,
                          const Point location, distance_type square_range,
                          V &visitor) const {
      const Point middle = bounds.GetMiddle();

      buckets[0].VisitWithinRange(GetTopLeft(bounds, middle),
                                  location, square_range, visitor);
      buckets[1].VisitWithinRange(GetTopRight(bounds, middle),
                                  location, square_range, visitor);
      buckets[2].VisitWithinRange(GetBottomLeft(bounds, middle),
                                  location, square_range, visitor);
      buckets[3].VisitWithinRange(GetBottomRight(bounds, middle),
                                  location, square_range, visitor);
    }
  };

  /**
   * Safely deconstify a bucket pointer.  This is a hack.
   */
  gcc_const
  Bucket *DeconstifyBucket(const Bucket *bucket) {
    assert(bucket != nullptr);
    assert(bucket->GetRoot() == &root);

    return const_cast<Bucket *>(bucket);
  }

  /**
   * Safely deconstify a leaf pointer.  This is a hack.
   */
  gcc_const
  Leaf *DeconstifyLeaf(const Leaf *leaf) {
    assert(leaf != nullptr);

    return const_cast<Leaf *>(leaf);
  }

  LeafAllocator leaf_allocator;
  BucketAllocator bucket_allocator;

  Rectangle bounds;

  Bucket root;

public:
  QuadTree() {
    bounds.Clear();
  }

  QuadTree(const QuadTree &) = delete;

  ~QuadTree() {
    /* this needs to be called manually, because we can't pass the
       allocator references to the (Quad)Bucket / Leaf destructors */
    Clear();
  }

  constexpr
  bool HaveBounds() const {
    return !bounds.IsEmpty();
  }

  /**
   * Manually set the bounds.
   *
   * @param _bounds the new bounds, which must not be empty
   */
  void SetBounds(const Rectangle &_bounds) {
    assert(bounds.IsEmpty());
    assert(root.IsEmpty());
    assert(!_bounds.IsEmpty());

    bounds = _bounds;
  }

  /**
   * Clear the bounds.  May only be called on a "flat" QuadTree,
   * e.g. after Flatten() has been called.
   */
  void ClearBounds() {
    assert(IsFlat());

    bounds.Clear();
  }

  /**
   * Is the specified point within the current bounds?  If yes, then
   * it may be added without a rescan.
   */
  constexpr
  bool IsWithinBounds(Point position) const {
    return bounds.IsInside(position);
  }

  /**
   * Is the specified value within the current bounds?  If yes, then
   * it may be added without a rescan.
   */
  constexpr
  bool IsWithinBounds(const T &value) const {
    return bounds.IsInside(GetPosition(value));
  }

  constexpr
  bool IsWithinKnownBounds(const T &value) const {
    return bounds.IsEmpty() || bounds.IsInside(GetPosition(value));
  }

  /**
   * Scan the bounds of all leaves.  May only be caled on a "flat"
   * QuadTree, e.g. after Flatten() has been called.
   *
   * @return true if the QuadTree has non-empty bounds now
   */
  bool ScanBounds() {
    ClearBounds();
    root.Scan(bounds);

    return !bounds.IsEmpty();
  }

  constexpr
  bool IsFlat() const {
    return !root.IsSplitted();
  }

  /**
   * Flatten the tree, i.e. put all values into the root bucket and
   * delete all other buckets.
   */
  void Flatten() {
    root.Flatten(bucket_allocator);
  }

  /**
   * Rescan the bounds and rebuild the tree.
   */
  void Optimise() {
    Flatten();
    if (ScanBounds())
      root.Optimise(bounds, bucket_allocator);
  }

  /**
   * Remove all values.
   */
  void Clear() {
    root.Clear(bucket_allocator, leaf_allocator);
    bounds.Clear();
  }

  /**
   * Add a new value to the root bucket in a "flat" tree.  Do not scan
   * bounds.
   *
   * @return a reference to the new value in the QuadTree; it remains
   * valid until the value is removed
   */
  template<typename U>
  const T &AddQuick(U &&value) {
    assert(IsFlat());
    assert(bounds.IsEmpty());

    Leaf *leaf = leaf_allocator.allocate(1);
    leaf_allocator.construct(leaf, Leaf(std::forward<U>(value)));

    root.AddHere(leaf);

    return leaf->value;
  }

  /**
   * Add a new value to the root bucket in a "flat" tree.  Scan
   * bounds.
   *
   * @return a reference to the new value in the QuadTree; it remains
   * valid until the value is removed
   */
  template<typename U>
  const T &AddScan(U &&value) {
    assert(IsFlat());

    Leaf *leaf = leaf_allocator.allocate(1);
    leaf_allocator.construct(leaf, Leaf(std::forward<U>(value)));

    bounds.Scan(GetPosition(leaf->value));
    root.AddHere(leaf);

    return leaf->value;
  }

  /**
   * Add a new value to the tree.  It must fit inside the bounds.
   *
   * @return a reference to the new value in the QuadTree; it
   * remains valid until the value is removed
   */
  template<typename U>
  const T &AddDeep(U &&value) {
    assert(IsWithinBounds(value));

    Leaf *leaf = leaf_allocator.allocate(1);
    leaf_allocator.construct(leaf, Leaf(std::forward<U>(value)));

    Rectangle bounds = this->bounds;
    root.Add(bounds, leaf, bucket_allocator);

    return leaf->value;
  }

  /**
   * Add a new value.
   *
   * @return a reference to the new value in the QuadTree; it remains
   * valid until the value is removed
   */
  template<typename U>
  const T &Add(U &&value) {
    if (bounds.IsEmpty())
      /* quick-add mode: add all values to the root bucket, optimise
         later */
      return AddQuick(std::forward<U>(value));
    else if (IsWithinBounds(value))
      /* it's within the current bounds, we can insert it properly
         now */
      return AddDeep(std::forward<U>(value));
    else {
      /* outside of the current bounds: we need to rescan the
         bounds, prepare for that now */
      Flatten();
      ClearBounds();
      return AddQuick(std::forward<U>(value));
    }
  }

  /**
   * Does this QuadTree contain at least one value?
   */
  constexpr
  bool IsEmpty() const {
    return root.IsEmpty();
  }

  class iterator {
    friend class QuadTree;

    Bucket *bucket;
    Leaf *leaf;

    constexpr
    iterator()
      :bucket(nullptr), leaf(nullptr) {}

    constexpr
    iterator(Bucket *_bucket)
      :bucket(_bucket), leaf(bucket->leaves.head) {}

  public:
    typedef std::forward_iterator_tag iterator_category;
    typedef ptrdiff_t difference_type;
    typedef T value_type;
    typedef T *pointer;
    typedef T &reference;

    constexpr
    bool operator==(const iterator &other) const {
      return bucket == other.bucket && leaf == other.leaf;
    }

    constexpr
    bool operator!=(const iterator &other) const {
      return !(*this == other);
    }

    iterator &operator++() {
      assert(leaf != nullptr);

      leaf = leaf->next;
      if (leaf == nullptr) {
        bucket = bucket->FindNextLeafBucket();
        leaf = bucket != nullptr
          ? bucket->leaves.head
          : nullptr;
      }

      return *this;
    }

    reference operator*() const {
      assert(leaf != nullptr);

      return leaf->value;
    }

    pointer operator->() const {
      assert(leaf != nullptr);

      return &leaf->value;
    }
  };

  gcc_pure
  iterator begin() {
    Bucket *bucket = root.FindFirstLeafBucket();
    return bucket != nullptr
      ? iterator(bucket)
      : iterator();
  }

  gcc_pure
  iterator end() {
    return iterator();
  }

  class const_iterator {
    friend class QuadTree;

    const Bucket *bucket;
    const Leaf *leaf;

    constexpr
    const_iterator()
      :bucket(nullptr), leaf(nullptr) {}

    constexpr
    const_iterator(const Bucket *_bucket)
      :bucket(_bucket), leaf(bucket->leaves.head) {}

    constexpr
    const_iterator(const Bucket *_bucket, const Leaf *_leaf)
      :bucket(_bucket), leaf(_leaf) {}

  public:
    typedef std::forward_iterator_tag iterator_category;
    typedef ptrdiff_t difference_type;
    typedef const T value_type;
    typedef const T *pointer;
    typedef const T &reference;

    constexpr
    const_iterator(const iterator &other)
      :bucket(other.bucket), leaf(other.leaf) {}

    constexpr
    bool operator==(const const_iterator &other) const {
      return bucket == other.bucket && leaf == other.leaf;
    }

    constexpr
    bool operator!=(const const_iterator &other) const {
      return !(*this == other);
    }

    const_iterator &operator++() {
      assert(leaf != nullptr);

      leaf = leaf->next;
      if (leaf == nullptr) {
        bucket = bucket->FindNextLeafBucket();
        leaf = bucket != nullptr
          ? bucket->leaves.head
          : nullptr;
      }

      return *this;
    }

    reference operator*() const {
      assert(leaf != nullptr);

      return leaf->value;
    }

    pointer operator->() const {
      assert(leaf != nullptr);

      return &leaf->value;
    }
  };

  gcc_pure
  const_iterator begin() const {
    const Bucket *bucket = root.FindFirstLeafBucket();
    return bucket != nullptr
      ? const_iterator(bucket)
      : const_iterator();
  }

  gcc_pure
  const_iterator end() const {
    return const_iterator();
  }

  gcc_pure
  const_iterator FindPointer(const T *value) const {
    assert(value != nullptr);
    assert(IsWithinKnownBounds(*value));

    Rectangle bounds = this->bounds;
    return root.FindPointer(bounds, value);
  }

  gcc_pure
  unsigned size() const {
    return root.GetSize();
  }

  void clear() {
    Clear();
  }

  template<typename U>
  void insert(U &&value) {
    Add(std::forward<U>(value));
  }

  void erase(const_iterator it) {
    assert(it.bucket != nullptr);
    assert(it.bucket->GetRoot() == &root);
    assert(it.leaf != nullptr);

    Bucket *bucket = DeconstifyBucket(it.bucket);
    bucket->Erase(it.leaf, leaf_allocator);

    if (IsEmpty())
      ClearBounds();
  }

  /**
   * Erase all elements that match the specified predicate.
   */
  template<class P>
  void EraseIf(const P &predicate) {
    root.EraseIf(predicate, leaf_allocator);

    if (IsEmpty())
      ClearBounds();
  }

  /**
   * Replace the value, and reposition the item in the tree (if its
   * position has been modified).
   */
  template<typename U>
  void Replace(const_iterator it, U &&value) {
    assert(it.bucket != nullptr);
    assert(it.bucket->GetRoot() == &root);
    assert(it.leaf != nullptr);

    Leaf *leaf = DeconstifyLeaf(it.leaf);
    if (bounds.IsEmpty()) {
      /* bounds are not known yet, we can replace the value quickly */
      leaf->value = std::forward<U>(value);
      return;
    }

    const Point old_position = GetPosition(leaf->value);
    const Point new_position = GetPosition(value);

    leaf->value = std::forward<U>(value);

    if (new_position != old_position) {
      /* the position has changed, we have to reposition the Leaf
         object */
      Bucket *bucket = DeconstifyBucket(it.bucket);
      bucket->Remove(leaf);

      Rectangle bounds = this->bounds;
      root.Add(bounds, leaf, bucket_allocator);
    }
  }

  template<class P>
  gcc_pure
  std::pair<const_iterator, distance_type>
  FindNearestIf(const Point location, distance_type range,
                const P &predicate) const {
    return root.FindNearestIf(bounds, location, Square(range),
                              predicate);
  }

  template<class P>
  gcc_pure
  std::pair<const_iterator, distance_type>
  FindNearestIf(const T &value, distance_type range,
                const P &predicate) const {
    return FindNearestIf(GetPosition(value), range, predicate);
  }

  gcc_pure
  std::pair<const_iterator, distance_type>
  FindNearest(const Point location, distance_type range) const {
    return root.FindNearestIf(bounds, location, Square(range), AlwaysTrue());
  }

  gcc_pure
  std::pair<const_iterator, distance_type>
  FindNearest(const T &value, distance_type range) const {
    return FindNearest(GetPosition(value), range);
  }

  template<class V>
  void VisitWithinRange(const Point location, distance_type range,
                        V &visitor) const {
    root.VisitWithinRange(bounds, location, Square(range), visitor);
  }

  template<class V>
  void VisitWithinRange(const T &value, distance_type range,
                        V &visitor) const {
    VisitWithinRange(GetPosition(value), range, visitor);
  }
};

#endif
