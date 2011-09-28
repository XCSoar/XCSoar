#ifndef FIXED_HPP
#define FIXED_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams
//
// Extensions and bug/compilation fixes by John Wharington 2009

#include "Compiler.h"

#include <algorithm>
#include "Constants.h"
#include <assert.h>
using std::max;
using std::min;

#ifdef FIXED_MATH
#define fixed_constant(d, f) fixed(fixed::internal(), (f))
#else
#define fixed_constant(d, f) (d)
#endif

#define fixed_int_constant(i) fixed_constant((double)(i), ((fixed::value_t)(i)) << fixed::resolution_shift)

#define fixed_zero fixed_int_constant(0)
#define fixed_one fixed_int_constant(1)
#define fixed_minus_one fixed_int_constant(-1)
#define fixed_two fixed_int_constant(2)
#define fixed_four fixed_int_constant(4)
#define fixed_ten fixed_int_constant(10)

#define fixed_half fixed_constant(0.5, 1 << (fixed::resolution_shift - 1))
#define fixed_third fixed_constant(1./3., (1 << fixed::resolution_shift) / 3)
#define fixed_two_thirds fixed_constant(2./3., (2 << (fixed::resolution_shift)) / 3)

#define fixed_deg_to_rad fixed_constant(0.0174532925199432958, 0x477d1bLL)
#define fixed_rad_to_deg fixed_constant(57.2957795131, 0x394bb834cLL)
#define fixed_pi fixed_constant(M_PI, 0x3243f6a8LL)
#define fixed_two_pi fixed_constant(M_2PI, 0x6487ed51LL)
#define fixed_half_pi fixed_constant(M_HALFPI, 0x1921fb54LL)
#define fixed_quarter_pi fixed_constant(M_HALFPI/2, 0xc90fdaaLL)

#define fixed_90 fixed_int_constant(90)
#define fixed_180 fixed_int_constant(180)
#define fixed_270 fixed_int_constant(270)
#define fixed_360 fixed_int_constant(360)

#define fixed_sqrt_two fixed_constant(1.4142135623730951, 0x16a09e66LL)
#define fixed_sqrt_half fixed_constant(0.70710678118654757, 0xb504f33LL)

#ifndef FIXED_MATH
#include <math.h>
#define FIXED_DOUBLE(x) (x)
#define FIXED_INT(x) ((int)x)
typedef double fixed;

void sin_cos(const double&theta, double*s, double*c);
#define positive(x) (x > 0)
#define negative(x) (x < 0)
#define sigmoid(x) (2.0 / (1.0 + exp(-x)) - 1.0)

gcc_const
static inline fixed
half(fixed a)
{
  return a * 0.5;
}

gcc_const
inline fixed rsqrt(fixed a) {
  // not fast
  return 1.0/sqrt(a);
}

gcc_const
inline fixed fast_sqrt(fixed a) {
  // not fast
  return sqrt(a);
}

gcc_const
inline fixed sqr(fixed a) {
  return a*a;
}

gcc_const
inline fixed fast_mult(fixed a, int a_bits, fixed b, int b_bits)
{
  return a * b;
}

gcc_const
inline fixed fast_mult(fixed a, fixed b, int b_bits)
{
  return a * b;
}

gcc_const
inline fixed accurate_half_sin(fixed a) {
  return sin(a/2);
}

#else
#define FIXED_DOUBLE(x) x.as_double()
#define FIXED_INT(x) x.as_int()

#include <complex>
#include <climits>

#ifdef HAVE_BOOST
#include <boost/cstdint.hpp>
#else
#include <stdint.h>
#endif

class fixed
{
#ifdef HAVE_BOOST
  typedef boost::int64_t int64_t;
  typedef boost::uint64_t uint64_t;
#endif
  typedef uint64_t uvalue_t;

public:
  typedef int64_t value_t;

  static const unsigned resolution_shift = 28;
  static const value_t resolution = 1 << resolution_shift;
  static const unsigned accurate_cordic_shift = 11;

private:
    value_t m_nVal;

public:

  struct internal {};

  fixed() {}

  fixed(internal, value_t nVal)
    :m_nVal(nVal) {}

//    fixed(value_t nVal):
//        m_nVal(nVal<<resolution_shift)
//    {}

  explicit fixed(long nVal)
    :m_nVal((value_t)(nVal)<<resolution_shift) {}

  explicit fixed(int nVal)
    :m_nVal((value_t)(nVal)<<resolution_shift) {}

  explicit fixed(short nVal)
    :m_nVal((value_t)(nVal)<<resolution_shift) {}

/*    
    fixed(unsigned value_t nVal):
        m_nVal(nVal<<resolution_shift)
    {}
*/  
  explicit fixed(unsigned long nVal)
    :m_nVal((value_t)(nVal)<<resolution_shift) {}
  explicit fixed(unsigned int nVal)
    :m_nVal((value_t)(nVal)<<resolution_shift) {}
  explicit fixed(unsigned short nVal)
    :m_nVal((value_t)(nVal)<<resolution_shift) {}
  explicit fixed(double nVal)
    :m_nVal(static_cast<value_t>(nVal*static_cast<double>(resolution))) {}
  explicit fixed(float nVal)
    :m_nVal(static_cast<value_t>(nVal*static_cast<float>(resolution))) {}

  gcc_pure
  friend bool operator==(fixed const& lhs,fixed const& rhs) {
    return lhs.m_nVal==rhs.m_nVal;
  }

  gcc_pure
  friend bool operator!=(fixed const& lhs,fixed const& rhs) {
    return lhs.m_nVal!=rhs.m_nVal;
  }

  gcc_pure
  friend bool operator<(fixed const& lhs,fixed const& rhs) {
    return lhs.m_nVal<rhs.m_nVal;
  }

  gcc_pure
  friend bool operator>(fixed const& lhs,fixed const& rhs) {
    return lhs.m_nVal>rhs.m_nVal;
  }

  gcc_pure
  friend bool operator<=(fixed const& lhs,fixed const& rhs) {
    return lhs.m_nVal<=rhs.m_nVal;
  }

  gcc_pure
  friend bool operator>=(fixed const& lhs,fixed const& rhs) {
    return lhs.m_nVal>=rhs.m_nVal;
  }

  gcc_pure
  operator bool() const {
    return m_nVal != 0;
  }

  gcc_pure
  inline operator double() const {
    return as_double();
  }

  gcc_pure
  inline operator float() const {
    return as_float();
  }

  gcc_pure
  inline operator short() const {
    return as_short();
  }

  gcc_pure
  inline operator int() const {
    return as_int();
  }

  gcc_pure
  inline operator unsigned() const {
    return (unsigned)as_int();
  }

  gcc_pure
  inline operator unsigned short() const {
    return as_unsigned_short();
  }

  gcc_pure
  inline operator long() const {
    return as_long();
  }

  gcc_pure
  float as_float() const {
    return m_nVal/(float)resolution;
  }

  gcc_pure
  double as_double() const {
    return m_nVal/(double)resolution;
  }

  gcc_pure
  long as_long() const {
    return (long)(m_nVal >> resolution_shift);
  }

  gcc_pure
  int64_t as_int64() const {
    return m_nVal >> resolution_shift;
  }

  gcc_pure
  int as_int() const {
    return (int)(m_nVal >> resolution_shift);
  }

  gcc_pure
  unsigned long as_unsigned_long() const {
    return (unsigned long)(m_nVal >> resolution_shift);
  }

  /*
  uint64_t as_unsigned_int64() const {
    return (uint64_t)(m_nVal >> resolution_shift);
  }
  */

  gcc_pure
  unsigned int as_unsigned_int() const {
    return (unsigned int)(m_nVal >> resolution_shift);
  }

  gcc_pure
  short as_short() const {
    return (short)(m_nVal >> resolution_shift);
  }

  gcc_pure
  unsigned short as_unsigned_short() const {
    return (unsigned short)(m_nVal >> resolution_shift);
  }

  // TODO: be more generic
  gcc_pure
  long as_glfixed() const {
    //assert(resolution_shift >= 16);
    return m_nVal >> (resolution_shift - 16);
  }

  // TODO: be more generic
  gcc_pure
  long as_glfixed_scale() const {
    //assert(resolution_shift <= 32);
    return m_nVal << (32 - resolution_shift);
  }

  fixed operator++() {
    m_nVal += resolution;
    return *this;
  }

  fixed operator--() {
    m_nVal -= resolution;
    return *this;
  }

  gcc_pure
  bool positive() const;

  gcc_pure
  bool negative() const;

  gcc_pure
  fixed Half() const {
    return fixed(internal(), m_nVal >> 1);
  }

  gcc_pure
  fixed trunc() const {
    value_t x = m_nVal;
    if (x < 0)
      x += resolution - 1;
    return fixed(fixed::internal(),
                 x & ((int64_t)-1 << resolution_shift));
  }

  gcc_pure
  fixed floor() const;

  gcc_pure
  fixed ceil() const;

  gcc_pure
  fixed sqrt() const;

  gcc_pure
  fixed fast_sqrt() const;

  gcc_pure
  fixed sqr() const;

  gcc_pure
  fixed rsqrt() const;

  gcc_pure
  fixed exp() const;

  gcc_pure
  fixed log() const;

  fixed& operator%=(fixed const& other);
  fixed& operator*=(fixed const& val);
  fixed& operator/=(fixed const divisor);
  fixed& operator-=(fixed const& val) {
    m_nVal -= val.m_nVal;
    return *this;
  }

  fixed& operator+=(fixed const& val) {
    m_nVal += val.m_nVal;
    return *this;
  }

/*
    fixed& operator*=(value_t val)
    {
        m_nVal*=val;
        return *this;
    }
*/

  fixed& operator*=(long val) {
    m_nVal*=val;
    return *this;
  }

  fixed& operator*=(int val) {
    m_nVal*=val;
    return *this;
  }

  fixed& operator*=(short val) {
    m_nVal*=val;
    return *this;
  }

  fixed& operator*=(char val) {
    m_nVal*=val;
    return *this;
  }

/*
    fixed& operator*=(unsigned value_t val)
    {
        m_nVal*=val;
        return *this;
    }
*/

  fixed& operator*=(unsigned long val) {
    m_nVal*=val;
    return *this;
  }

  fixed& operator*=(unsigned int val) {
    m_nVal*=val;
    return *this;
  }

  fixed& operator*=(unsigned short val) {
    m_nVal*=val;
    return *this;
  }

  fixed& operator*=(unsigned char val) {
    m_nVal*=val;
    return *this;
  }

  gcc_const
  static fixed fast_mult(fixed a, int a_shift, fixed b, int b_shift) {
    return fixed(internal(),
                 ((a.m_nVal >> a_shift) * (b.m_nVal >> b_shift))
                 >> (resolution_shift - a_shift - b_shift));
  }

/*
    fixed& operator/=(value_t val)
    {
        m_nVal/=val;
        return *this;
    }
*/

  fixed& operator/=(long val) {
    m_nVal/=val;
    return *this;
  }

  fixed& operator/=(int val) {
    m_nVal/=val;
    return *this;
  }

  fixed& operator/=(short val) {
    m_nVal/=val;
    return *this;
  }

  fixed& operator/=(char val) {
    m_nVal/=val;
    return *this;
  }

/*
    fixed& operator/=(unsigned value_t val)
    {
        m_nVal/=val;
        return *this;
    }
*/

  fixed& operator/=(unsigned long val) {
    m_nVal/=val;
    return *this;
  }

  fixed& operator/=(unsigned int val) {
    m_nVal/=val;
    return *this;
  }

  fixed& operator/=(unsigned short val) {
    m_nVal/=val;
    return *this;
  }

  fixed& operator/=(unsigned char val) {
    m_nVal/=val;
    return *this;
  }

  bool operator!() const {
    return m_nVal==0;
  }

  gcc_pure
  fixed modf(fixed* integral_part) const;

  gcc_pure
  fixed atan() const;

  static void sin_cos(fixed const& theta,fixed* s,fixed*c);
  static void to_polar(fixed const& x,fixed const& y,fixed* r,fixed*theta);

  gcc_pure
  static fixed atan2(fixed const& y,fixed const& x);

  gcc_pure
  static fixed sigmoid(fixed const& x);

  gcc_pure
  fixed sin() const;

  gcc_pure
  fixed cos() const;

  gcc_pure
  fixed tan() const;

  gcc_pure
  fixed accurate_half_sin() const;

  gcc_pure
  fixed operator-() const;

  gcc_pure
  fixed abs() const;
};

inline bool fixed::positive() const
{
  return (m_nVal>0);
}

inline bool fixed::negative() const
{
  return (m_nVal<0);
}

gcc_pure
inline fixed operator-(fixed const& a,fixed const& b)
{
  fixed temp(a);
  return temp-=b;
}

gcc_pure
inline fixed operator%(fixed const& a,fixed const& b)
{
  fixed temp(a);
  return temp%=b;
}

gcc_pure
inline fixed operator+(fixed const& a,fixed const& b)
{
  fixed temp(a);
  return temp+=b;
}

gcc_pure
inline fixed operator*(unsigned long a, fixed const& b)
{
  fixed temp(b);
  return temp*=a;
}

gcc_pure
inline fixed operator*(long a, fixed const& b)
{
  fixed temp(b);
  return temp*=a;
}

gcc_pure
inline fixed operator*(unsigned a, fixed const& b)
{
  fixed temp(b);
  return temp*=a;
}

gcc_pure
inline fixed operator*(int a, fixed const& b)
{
  fixed temp(b);
  return temp*=a;
}

gcc_pure
inline fixed operator*(unsigned short a, fixed const& b)
{
  fixed temp(b);
  return temp*=a;
}

gcc_pure
inline fixed operator*(short a, fixed const& b)
{
  fixed temp(b);
  return temp*=a;
}

gcc_pure
inline fixed operator*(unsigned char a, fixed const& b)
{
  fixed temp(b);
  return temp*=a;
}

gcc_pure
inline fixed operator*(char a, fixed const& b)
{
  fixed temp(b);
  return temp*=a;
}

gcc_pure
inline fixed operator*(fixed const& a,unsigned long b)
{
  fixed temp(a);
  return temp *= b;
}

gcc_pure
inline fixed operator*(fixed const& a,long b)
{
  fixed temp(a);
  return temp *= b;
}

gcc_pure
inline fixed operator*(fixed const& a,unsigned b)
{
  fixed temp(a);
  return temp *= b;
}

gcc_pure
inline fixed operator*(fixed const& a,int b)
{
  fixed temp(a);
  return temp *= b;
}

gcc_pure
inline fixed operator*(fixed const& a,unsigned short b)
{
  fixed temp(a);
  return temp *= b;
}

gcc_pure
inline fixed operator*(fixed const& a,short b)
{
  fixed temp(a);
  return temp *= b;
}

gcc_pure
inline fixed operator*(fixed const& a,unsigned char b)
{
  fixed temp(a);
  return temp *= b;
}

gcc_pure
inline fixed operator*(fixed const& a,char b)
{
  fixed temp(a);
  return temp *= b;
}

gcc_pure
inline fixed operator*(fixed const& a,fixed const& b)
{
  fixed temp(a);
  return temp*=b;
}

/**
 * Simplified and faster multiplication when you know the range of
 * the coefficients at compile time.
 *
 * @param a first coefficient
 * @param a_shift number of bits to discard from the first coefficient
 * @param b second coefficient
 * @param b_shift number of bits to discard from the second coefficient
 */
gcc_const
inline fixed
fast_mult(fixed a, int a_shift, fixed b, int b_shift)
{
  return fixed::fast_mult(a, a_shift, b, b_shift);
}

gcc_const
inline fixed fast_mult(fixed a, fixed b, int b_bits)
{
  return fixed::fast_mult(a, 0, b, b_bits);
}

gcc_pure
inline fixed operator/(fixed const& a,unsigned long b)
{
  fixed temp(a);
  return temp /= b;
}

gcc_pure
inline fixed operator/(fixed const& a,long b)
{
  fixed temp(a);
  return temp /= b;
}

gcc_pure
inline fixed operator/(fixed const& a,unsigned b)
{
  fixed temp(a);
  return temp /= b;
}

gcc_pure
inline fixed operator/(fixed const& a,int b)
{
  fixed temp(a);
  return temp /= b;
}

gcc_pure
inline fixed operator/(fixed const& a,unsigned short b)
{
  fixed temp(a);
  return temp /= b;
}

gcc_pure
inline fixed operator/(fixed const& a,short b)
{
  fixed temp(a);
  return temp /= b;
}

gcc_pure
inline fixed operator/(fixed const& a,unsigned char b)
{
  fixed temp(a);
  return temp /= b;
}

gcc_pure
inline fixed operator/(fixed const& a,char b)
{
  fixed temp(a);
  return temp /= b;
}

gcc_pure
inline fixed operator/(fixed const& a,fixed const& b)
{
  fixed temp(a);
  return temp/=b;
}

gcc_pure
static inline fixed pow(fixed x, fixed y)
{
  return fixed(pow((double)x, (double)y));
}

inline fixed sin(fixed const& x)
{
  return x.sin();
}
inline fixed cos(fixed const& x)
{
  return x.cos();
}
inline fixed tan(fixed const& x)
{
  return x.tan();
}
inline fixed atan(fixed const& x)
{
    return x.atan();
}
inline fixed accurate_half_sin(fixed const& x)
{
  return x.accurate_half_sin();
}

gcc_pure
inline fixed atan2(fixed const& y, fixed const& x)
{
  return fixed::atan2(y,x);
}

static inline fixed asin(fixed x)
{
  return atan2(x, (fixed_one-x*x).sqrt());
}

static inline fixed acos(fixed x)
{
  return atan2((fixed_one-x*x).sqrt(), x);
}

gcc_pure
inline fixed sqr(fixed const& x)
{
  return x.sqr();
}

gcc_pure
inline fixed sqrt(fixed const& x)
{
  return x.sqrt();
}

gcc_pure
inline fixed fast_sqrt(fixed const& x)
{
  assert(!x.negative());
  if (!x.positive())
    return fixed_zero;
  return x.rsqrt()*x;
}

gcc_pure
inline fixed rsqrt(fixed const& x)
{
  return x.rsqrt();
}

gcc_const
inline fixed hypot(fixed x, fixed y)
{
  return sqrt(sqr(x) + sqr(y));
}

gcc_pure
inline fixed exp(fixed const& x)
{
  return x.exp();
}

gcc_pure
inline fixed log(fixed const& x)
{
  return x.log();
}

inline fixed trunc(fixed x)
{
  return x.trunc();
}

inline fixed floor(fixed const& x)
{
  return x.floor();
}

gcc_pure
inline fixed ceil(fixed const& x)
{
  return x.ceil();
}

gcc_pure
inline fixed fabs(fixed const& x)
{
  return x.abs();
}

gcc_pure
inline fixed modf(fixed const& x,fixed*integral_part)
{
  return x.modf(integral_part);
}

gcc_pure
static inline fixed fmod(fixed x, fixed y)
{
  return fixed(fmod((double)x, (double)y));
}

inline fixed fixed::ceil() const
{
  if (m_nVal%resolution)
    return floor() + fixed(1);
  else
    return *this;
}

inline fixed fixed::floor() const
{
  fixed res(*this);
  value_t const remainder=m_nVal%resolution;
  if (remainder) {
    res.m_nVal -= remainder;
    if (m_nVal < 0)
      res -= fixed(1);
  }

  return res;
}

inline fixed fixed::sin() const
{
  fixed res;
  sin_cos(*this,&res,0);
  return res;
}

inline fixed fixed::cos() const
{
  fixed res;
  sin_cos(*this,0,&res);
  return res;
}

inline fixed fixed::tan() const
{
  fixed s,c;
  sin_cos(*this,&s,&c);
  return s/c;
}

inline fixed fixed::operator-() const
{
  return fixed(internal(),-m_nVal);
}

inline fixed fixed::abs() const
{
  return fixed(internal(),m_nVal<0?-m_nVal:m_nVal);
}

inline fixed fixed::modf(fixed*integral_part) const
{
  value_t fractional_part=m_nVal%resolution;
  if(m_nVal<0 && fractional_part>0)
    {
      fractional_part-=resolution;
    }
  integral_part->m_nVal=m_nVal-fractional_part;
  return fixed(internal(),fractional_part);
}

inline void sin_cos(fixed const& theta,fixed* s,fixed*c)
{
  ::fixed::sin_cos(theta, s, c);
}

gcc_pure
inline fixed sigmoid(fixed const& x)
{
  return ::fixed::sigmoid(x);
}

 namespace std
 {
   template<>
   inline ::fixed arg(const std::complex< ::fixed>& val)
   {
     ::fixed r,theta;
     ::fixed::to_polar(val.real(),val.imag(),&r,&theta);
     return theta;
   }

   template<>
   inline complex< ::fixed> polar(::fixed const& rho,::fixed const& theta)
   {
     ::fixed s,c;
     ::fixed::sin_cos(theta,&s,&c);
     return complex< ::fixed>(rho * c, rho * s);
   }
 }

#define fixed_max fixed(fixed::internal(), 0x7fffffffffffffffLL)

inline fixed fixed::sigmoid(const fixed&x) {
  return fixed_two/(fixed_one+(-x).exp())-fixed_one;
}

gcc_pure
inline bool positive(const fixed&f) {
  return f.positive();
}

gcc_pure
inline bool negative(const fixed&f) {
  return f.negative();
}

gcc_const
static inline fixed
half(fixed a)
{
  return a.Half();
}

#endif

inline void limit_tolerance(fixed& f, const fixed tol_act) {
  if (fabs(f)<tol_act) {
    f = positive(f)? tol_act:-tol_act;
  }
}

/**
 * Convert this number to an unsigned integer, with rounding.
 */
gcc_const static inline unsigned
uround(const fixed x)
{
  return (unsigned)(x + fixed_half);
}

#endif
