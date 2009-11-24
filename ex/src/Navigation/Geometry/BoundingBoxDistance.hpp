#ifndef BOUNDINGBOXDISTANCE_HPP
#define BOUNDINGBOXDISTANCE_HPP

/**
 * Class used for 2-d integer bounding box distance calculations by kd-tree
 */
class BBDist {
public:
  BBDist(const size_t _dim, const int _val)
    {
      set_vals(-1);
      val[_dim%2] = _val;
      calc_d();
    }
  BBDist(const double _val) {
    set_vals(-1);
    d = (int)_val;
  }
  BBDist(const int _val) {
    set_vals(-1);
    d = _val;
  }
  BBDist& operator+=(const BBDist &rhs) {
    set_max(0, rhs);
    set_max(1, rhs);
    calc_d();
    return *this;
  }
  operator double () const {
    return d;
  }
private:
  void set_max(const size_t _dim, const BBDist &rhs) {
    val[_dim] = std::max(val[_dim],rhs.val[_dim]);
  }
  void calc_d() {
    d=0;
    for (unsigned i=0; i<2; i++) {
      if (val[i]>0) {
        d+= val[i]*val[i];
      }
    }
  }
  void set_vals(const int _val) {
    val[0] = _val;
    val[1] = _val;
  }
  int val[2];
  int d;  
};

#endif
