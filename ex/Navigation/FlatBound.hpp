#ifndef FLATBOUND_HPP
#define FLATBOUND_HPP

struct FlatBound {
  FlatBound():min(0),max(0) {};
  FlatBound(int _min):min(_min),max(_min) {};
  FlatBound(int _min, int _max):min(_min),max(_max) {};
  int min;
  int max;

  // used by kdtree
  FlatBound operator+(const FlatBound &rhs) const {
    return FlatBound(min,max+rhs.max);
  }
  FlatBound operator-(const FlatBound &rhs) const {
    return FlatBound(min-rhs.min,max);
  }

  int dist(const FlatBound& b) const {
    if (min>b.max) {
      return min-b.max;
    }
    if (max<b.min) {
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
