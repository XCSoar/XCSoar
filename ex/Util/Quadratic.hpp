#ifndef QUADRATIC_HPP
#define QUADRATIC_HPP

class Quadratic {
public:
  Quadratic(double _a, double _b, double _c):
    a(_a),
    b(_b),
    c(_c)
    {};
  bool check() {
    if (b*b-4*a*c<0) {
      return false;
    }
    if (a==0.0) {
      return false;
    }
    return true;
  }
  double solution_max() {
    return (a>0? solution(true):solution(false));
  }
  double solution_min() {
    return (a>0? solution(false):solution(true));
  }
private:
  double solution(bool positive) {
    return (-b+(positive?1:-1)*sqrt(b*b-4*a*c))/(2.0*a);
  }
  const double a;
  const double b;
  const double c;
};


#endif
