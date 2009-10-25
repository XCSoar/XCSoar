#ifndef FLATBOUND_HPP
#define FLATBOUND_HPP

#include <algorithm>

struct FlatBound {
  FlatBound():min(0),max(0) {};
  FlatBound(int _val):min(_val),max(_val) {};
  FlatBound(int _min, int _max):min(_min),max(_max) {};
  int min;
  int max;

  // used by kdtree
  FlatBound operator+(const FlatBound &rhs) const {
    return FlatBound(min, max+rhs.max);
//    return FlatBound(min+rhs.max, max+rhs.max);
  }
  FlatBound operator-(const FlatBound &rhs) const {
    return FlatBound(min-rhs.min, max);
//    return FlatBound(min-rhs.min, max-rhs.min);
  }

  int dist(const FlatBound& b) const {
    if (min>b.max) {
      return min-b.max;
    }
    if (b.min>max) {
      return b.min-max;
    }
    // overlapping
    return 0;
  }

  struct kd_distance {
    typedef int distance_type;
    int operator() (const FlatBound &a, const FlatBound &b) const {
      return a.dist(b);
    }
  };
  struct kd_less {
    bool operator() (const FlatBound &a, const FlatBound &b) const {
      if (a.max<b.min) {
        return true;
      } else {
        return false;
      }
    }
  };
};

#endif
