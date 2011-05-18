// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams
#include "fixed.hpp"
#include "Compiler.h"

#ifndef FIXED_MATH

void sin_cos(const double&theta, double*s, double*c)
{
#if defined(__GLIBC__) && defined(_GNU_SOURCE)
  sincos(theta, s, c);
#else
  *s = sin(theta);
  *c = cos(theta);
#endif
}

#else

const unsigned fixed::resolution_shift;
const fixed::value_t fixed::resolution;

fixed::value_t const internal_pi=0x3243f6a8;
fixed::value_t const internal_two_pi=0x6487ed51;
fixed::value_t const internal_half_pi=0x1921fb54;
fixed::value_t const internal_quarter_pi=0xc90fdaa;

fixed& fixed::operator%=(fixed const& other)
{
  m_nVal = m_nVal%other.m_nVal;
  return *this;
}

fixed& fixed::operator*=(fixed const& val)
{
  bool const val_negative=val.m_nVal<0;
  bool const this_negative=m_nVal<0;
  bool const negate=val_negative ^ this_negative;
  uvalue_t const other=val_negative?-val.m_nVal:val.m_nVal;
  uvalue_t const self=this_negative?-m_nVal:m_nVal;

  if (uvalue_t const self_upper=(self>>32))
    m_nVal = (self_upper * other) << (32 - resolution_shift);
  else
    m_nVal = 0;

  if (uvalue_t const self_lower = (self&0xffffffff)) {
    unsigned long const other_upper=static_cast<unsigned long>(other>>32);
    unsigned long const other_lower=static_cast<unsigned long>(other&0xffffffff);
    uvalue_t const lower_self_upper_other_res=self_lower*other_upper;
    uvalue_t const lower_self_lower_other_res=self_lower*other_lower;
    m_nVal+=(lower_self_upper_other_res<<(32-resolution_shift))
      + (lower_self_lower_other_res>>resolution_shift);
  }

  if (negate)
    m_nVal = -m_nVal;

  return *this;
}

fixed& fixed::operator/=(fixed const divisor)
{
  /* This is an approximate fixed point division.  Problem was: for
     doing fixed point division, we have to shift the numerator left
     by "resolution_shift" bits, and divide by then denominator then;
     the first shift would however overflow.  Solution: shift as many
     bytes left as long as the highest-order bit doesn't get lost, and
     apply the remaining bits as a "right shift" to the denominator.
     The result is approximately the same, and for XCSoar, we can
     neglect the error. */

  enum {
    /** number of bits in a value_f */
    bits = sizeof(value_t) * 8,
  };

  unsigned shift = resolution_shift;
  value_t numerator = m_nVal, denominator = divisor.m_nVal;

  /* shift the numerator left by as many multiple of 7 bits as possible */
  while (shift >= 7 &&
         /* check the most significant 8 bits; we can shift by at
            least 7 bits if there is either 0xff or 0x00 */
         (((numerator >> (bits - 8)) + 1) & 0xfe) == 0) {
    shift -= 7;
    numerator <<= 7;
  }

  bool small_denominator = denominator >= -resolution * 1024 &&
    denominator <= resolution * 1024;

  /* apply the remaining bits to the denominator */
  if (!small_denominator)
    denominator >>= shift;

  /* now do the real division */
  m_nVal = gcc_likely(denominator != 0)
    ? numerator / denominator
    : fixed_max.m_nVal;

  if (small_denominator)
    m_nVal <<= shift;

  return *this;
}

fixed fixed::sqrt() const
{
#ifdef FAST_BUT_IMPRECISE_SQRT
  /* TODO: this algorithm is too imprecise, with inaccuracy of about
     2.1%; disabling it for now, until we have a better one */

  unsigned const max_shift=62;
  uvalue_t a_squared=1LL<<max_shift;
  unsigned b_shift=(max_shift+resolution_shift)/2;
  uvalue_t a=1LL<<b_shift;

  uvalue_t x=m_nVal;

  while(b_shift && a_squared>x)
    {
      a>>=1;
      a_squared>>=2;
      --b_shift;
    }

  uvalue_t remainder=x-a_squared;
  --b_shift;

  while(remainder && b_shift) {
    uvalue_t b_squared=1LL<<(2*b_shift-resolution_shift);
    int const two_a_b_shift=b_shift+1-resolution_shift;
    uvalue_t two_a_b=(two_a_b_shift>0)?(a<<two_a_b_shift):(a>>-two_a_b_shift);

    while (b_shift && remainder<(b_squared+two_a_b)) {
      b_squared>>=2;
      two_a_b>>=1;
      --b_shift;
    }

    uvalue_t const delta=b_squared+two_a_b;
    if ((2 * remainder) > delta) {
      a += 1LL << b_shift;
      remainder -= delta;
      if (b_shift)
        --b_shift;
    }
  }

  return fixed(internal(), a);
#else
  /* slow fallback */
  return fixed(::sqrt(as_double()));
#endif
}

namespace
{
  int const max_power=63-fixed::resolution_shift;
  fixed::value_t const log_two_power_n_reversed[]={
    0x18429946ELL,0x1791272EFLL,0x16DFB516FLL,0x162E42FF0LL,0x157CD0E70LL,0x14CB5ECF1LL,0x1419ECB71LL,0x13687A9F2LL,
    0x12B708872LL,0x1205966F3LL,0x115424573LL,0x10A2B23F4LL,0xFF140274LL,0xF3FCE0F5LL,0xE8E5BF75LL,0xDDCE9DF6LL,
    0xD2B77C76LL,0xC7A05AF7LL,0xBC893977LL,0xB17217F8LL,0xA65AF679LL,0x9B43D4F9LL,0x902CB379LL,0x851591FaLL,
    0x79FE707bLL,0x6EE74EFbLL,0x63D02D7BLL,0x58B90BFcLL,0x4DA1EA7CLL,0x428AC8FdLL,0x3773A77DLL,0x2C5C85FeLL,
    0x2145647ELL,0x162E42FfLL,0xB17217FLL
  };

  fixed::value_t const log_one_plus_two_power_minus_n[]={
    0x67CC8FBLL,0x391FEF9LL,0x1E27077LL,0xF85186LL,
    0x7E0A6CLL,0x3F8151LL,0x1FE02ALL,0xFF805LL,0x7FE01LL,0x3FF80LL,0x1FFE0LL,0xFFF8LL,
    0x7FFELL,0x4000LL,0x2000LL,0x1000LL,0x800LL,0x400LL,0x200LL,0x100LL,
    0x80LL,0x40LL,0x20LL,0x10LL,0x8LL,0x4LL,0x2LL,0x1LL
  };

  fixed::value_t const log_one_over_one_minus_two_power_minus_n[]={
    0xB172180LL,0x49A5884LL,0x222F1D0LL,0x108598BLL,
    0x820AECLL,0x408159LL,0x20202BLL,0x100805LL,0x80201LL,0x40080LL,0x20020LL,0x10008LL,
    0x8002LL,0x4001LL,0x2000LL,0x1000LL,0x800LL,0x400LL,0x200LL,0x100LL,
    0x80LL,0x40LL,0x20LL,0x10LL,0x8LL,0x4LL,0x2LL,0x1LL
  };
}


fixed fixed::exp() const
{
    if(m_nVal>=log_two_power_n_reversed[0])
        return fixed_max;

    if(m_nVal<-log_two_power_n_reversed[63-2*resolution_shift])
        return fixed(internal(),0);

    if(!m_nVal)
        return fixed(internal(),resolution);

    value_t res=resolution;

    if(m_nVal>0) {
      int power=max_power;
      value_t const* log_entry=log_two_power_n_reversed;
      value_t temp=m_nVal;
      while (temp && power>(-(int)resolution_shift)) {
        while (!power || (temp<*log_entry)) {
          if(!power)
            log_entry=log_one_plus_two_power_minus_n;
          else
            ++log_entry;
          --power;
        }
        temp -= *log_entry;
        if (power < 0)
          res+=(res>>(-power));
        else
          res<<=power;
      }
    } else {
      int power=resolution_shift;
      value_t const* log_entry=log_two_power_n_reversed+(max_power-power);
      value_t temp=m_nVal;

      while (temp && power>(-(int)resolution_shift)) {
        while (!power || (temp>(-*log_entry))) {
          if (!power)
            log_entry=log_one_over_one_minus_two_power_minus_n;
          else
            ++log_entry;

          --power;
        }
        temp+=*log_entry;
        if (power < 0)
          res-=(res>>(-power));
        else
          res>>=power;
      }
    }

    return fixed(internal(), res);
}

fixed fixed::log() const
{
  if (m_nVal <= 0)
      return -fixed_max;

  if (m_nVal == resolution)
      return fixed_zero;

  uvalue_t temp=m_nVal;
  int left_shift=0;
  uvalue_t const scale_position=0x8000000000000000LL;
  while (temp < scale_position) {
    ++left_shift;
    temp<<=1;
  }

  value_t res=(left_shift<max_power)?
    log_two_power_n_reversed[left_shift]:
    -log_two_power_n_reversed[2*max_power-left_shift];
  unsigned right_shift=1;
  uvalue_t shifted_temp=temp>>1;
  while (temp && (right_shift<resolution_shift)) {
    while (right_shift < resolution_shift &&
           temp < shifted_temp + scale_position) {
      shifted_temp>>=1;
      ++right_shift;
    }

    temp-=shifted_temp;
    shifted_temp=temp>>right_shift;
    res+=log_one_over_one_minus_two_power_minus_n[right_shift-1];
  }

  return fixed(internal(), res);
}

namespace
{
  const long arctantab[32] = {
    297197971, 210828714, 124459457, 65760959, 33381290, 16755422, 8385879,
    4193963, 2097109, 1048571, 524287, 262144, 131072, 65536, 32768, 16384,
    8192, 4096, 2048, 1024, 512, 256, 128, 64, 32, 16, 8, 4, 2, 1, 0, 0,
  };


  long scale_cordic_result(long a)
  {
    long const cordic_scale_factor=0x22C2DD1C; /* 0.271572 * 2^31*/
    return (long)((((fixed::value_t)a)*cordic_scale_factor)>>31);
  }

  fixed::value_t scale_cordic_result_accurate(long a)
  {
    fixed::value_t const cordic_scale_factor=0x22C2DD1C; /* 0.271572 * 2^31*/
    return (fixed::value_t)((((fixed::value_t)a)*cordic_scale_factor)>>
                            (31-fixed::accurate_cordic_shift));
  }

  long right_shift(long val,int shift)
  {
    return (shift<0)?(val<<-shift):(val>>shift);
  }

  void perform_cordic_rotation_unscaled(long&x, long&y, long theta)
  {
    long const *arctanptr = arctantab;

    for (int i = -1; i <= (int)fixed::resolution_shift; ++i) {
      long const yshift=right_shift(y,i);
      long const xshift=right_shift(x,i);

      if (theta < 0) {
        x += yshift;
        y -= xshift;
        theta += *arctanptr++;
      } else {
        x -= yshift;
        y += xshift;
        theta -= *arctanptr++;
      }
    }    
  }

  void perform_cordic_rotation(long&px, long&py, long theta)
  {
    perform_cordic_rotation_unscaled(px, py, theta);
    px = scale_cordic_result(px);
    py = scale_cordic_result(py);
  }

  fixed::value_t perform_cordic_rotation_accurate_sin(long theta)
  {
    long x_cos=1<< fixed::resolution_shift;
    long x_sin=0;
    perform_cordic_rotation_unscaled(x_cos, x_sin, theta);
    return scale_cordic_result_accurate(x_sin);
  }

  void perform_cordic_polarization(long& argx, long&argy)
  {
    long theta=0;
    long x = argx, y = argy;
    long const *arctanptr = arctantab;

    for (int i = -1; i <= (int)fixed::resolution_shift; ++i) {
      long const yshift=right_shift(y,i);
      long const xshift=right_shift(x,i);
      if (y < 0) {
        y += xshift;
        x -= yshift;
        theta -= *arctanptr++;
      } else {
        y -= xshift;
        x += yshift;
        theta += *arctanptr++;
      }
    }

    argx = scale_cordic_result(x);
    argy = theta;
  }
}

fixed
fixed::accurate_half_sin() const
{
  value_t x= (m_nVal>>1) % internal_two_pi;
  if( x < 0 )
    x += internal_two_pi;

  bool negate_sin=false;
  
  if( x > internal_pi )
  {
    x =internal_two_pi-x;
    negate_sin=true;
  }
  if(x>internal_half_pi)
  {
    x=internal_pi-x;
  }
  
  const value_t x_sin = perform_cordic_rotation_accurate_sin((long)x);
  return fixed(fixed::internal(), 
               (negate_sin? -x_sin:x_sin) );
}

void fixed::sin_cos(fixed const& theta,fixed* s,fixed*c)
{
    value_t x=theta.m_nVal%internal_two_pi;
    if( x < 0 )
        x += internal_two_pi;

    bool negate_cos=false;
    bool negate_sin=false;

    if( x > internal_pi )
    {
        x =internal_two_pi-x;
        negate_sin=true;
    }
    if(x>internal_half_pi)
    {
        x=internal_pi-x;
        negate_cos=true;
    }
    long x_cos=1<<resolution_shift,x_sin=0;

    perform_cordic_rotation(x_cos,x_sin,(long)x);

    if(s)
    {
        s->m_nVal=negate_sin?-x_sin:x_sin;
    }
    if(c)
    {
        c->m_nVal=negate_cos?-x_cos:x_cos;
    }
}

fixed fixed::atan() const
{
    fixed r,theta;
    to_polar(fixed_one, *this, &r, &theta);
    return theta;
}

fixed fixed::atan2(fixed const& y, fixed const& x)
{
    fixed r,theta;
    to_polar(x,y, &r, &theta);
    return theta;
}

void fixed::to_polar(fixed const& x,fixed const& y,fixed* r,fixed* theta)
{
    bool const negative_x=x.m_nVal<0;
    bool const negative_y=y.m_nVal<0;
    
    uvalue_t a=negative_x?-x.m_nVal:x.m_nVal;
    uvalue_t b=negative_y?-y.m_nVal:y.m_nVal;

    unsigned right_shift=0;
    unsigned const max_value=1U<<resolution_shift;

    while((a>=max_value) || (b>=max_value))
    {
        ++right_shift;
        a>>=1;
        b>>=1;
    }
    long xtemp=(long)a;
    long ytemp=(long)b;
    perform_cordic_polarization(xtemp,ytemp);
    r->m_nVal=(value_t)(xtemp)<<right_shift;
    theta->m_nVal=ytemp;

    if(negative_x && negative_y)
    {
        theta->m_nVal-=internal_pi;
    }
    else if(negative_x)
    {
        theta->m_nVal=internal_pi-theta->m_nVal;
    }
    else if(negative_y)
    {
        theta->m_nVal=-theta->m_nVal;
    }
}


fixed fixed::sqr() const
{
  uvalue_t const self=(m_nVal<0)?-m_nVal:m_nVal;
  
  fixed res;
  uvalue_t const self_upper=(self>>32);
  if (self_upper)
    res.m_nVal = (self_upper * self) << (32 - resolution_shift);
  else
    res.m_nVal = 0;

  if (uvalue_t const self_lower = (self&0xffffffff)) {
    uvalue_t const lower_upper=self_lower*self_upper;
    uvalue_t const lower_lower=self_lower*self_lower;
    res.m_nVal+=
      (lower_upper<<(32-resolution_shift))
      +(lower_lower>>resolution_shift);
  }
  return res;
}


static inline
fixed rsqrt_guess(fixed x) {
  union { double f; uint64_t u; } y = {x.as_float()};
  y.u = 0x5fe6ec85e7de30daLL - (y.u>>1);
  return fixed(y.f);
}


fixed
fixed::rsqrt() const
{
  static const fixed threehalfs = fixed(1.5);

  fixed y(rsqrt_guess(*this));
  if (y.m_nVal<2) return y;

  const fixed x2 = fixed(internal(), m_nVal>>1);
#define tolerance (1<<10)
  value_t v_last= y.m_nVal;
  while (1) {
    y *= threehalfs-x2*y.sqr();
    assert(y>= fixed_zero);
    const value_t err = y.m_nVal-v_last;
    if ((y.m_nVal<2) || ((err>0? err:-err) < tolerance))
      return y;
    v_last = y.m_nVal;
  }
}

#endif
