/*
 * Copyright (c) 1999-2000 Image Power, Inc. and the University of
 *   British Columbia.
 * Copyright (c) 2001-2002 Michael David Adams.
 * All rights reserved.
 */

/* __START_OF_JASPER_LICENSE__
 * 
 * JasPer License Version 2.0
 * 
 * Copyright (c) 2001-2006 Michael David Adams
 * Copyright (c) 1999-2000 Image Power, Inc.
 * Copyright (c) 1999-2000 The University of British Columbia
 * 
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person (the
 * "User") obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 * 
 * 1.  The above copyright notices and this permission notice (which
 * includes the disclaimer below) shall be included in all copies or
 * substantial portions of the Software.
 * 
 * 2.  The name of a copyright holder shall not be used to endorse or
 * promote products derived from the Software without specific prior
 * written permission.
 * 
 * THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL PART OF THIS
 * LICENSE.  NO USE OF THE SOFTWARE IS AUTHORIZED HEREUNDER EXCEPT UNDER
 * THIS DISCLAIMER.  THE SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS
 * "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
 * INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.  NO ASSURANCES ARE
 * PROVIDED BY THE COPYRIGHT HOLDERS THAT THE SOFTWARE DOES NOT INFRINGE
 * THE PATENT OR OTHER INTELLECTUAL PROPERTY RIGHTS OF ANY OTHER ENTITY.
 * EACH COPYRIGHT HOLDER DISCLAIMS ANY LIABILITY TO THE USER FOR CLAIMS
 * BROUGHT BY ANY OTHER ENTITY BASED ON INFRINGEMENT OF INTELLECTUAL
 * PROPERTY RIGHTS OR OTHERWISE.  AS A CONDITION TO EXERCISING THE RIGHTS
 * GRANTED HEREUNDER, EACH USER HEREBY ASSUMES SOLE RESPONSIBILITY TO SECURE
 * ANY OTHER INTELLECTUAL PROPERTY RIGHTS NEEDED, IF ANY.  THE SOFTWARE
 * IS NOT FAULT-TOLERANT AND IS NOT INTENDED FOR USE IN MISSION-CRITICAL
 * SYSTEMS, SUCH AS THOSE USED IN THE OPERATION OF NUCLEAR FACILITIES,
 * AIRCRAFT NAVIGATION OR COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL
 * SYSTEMS, DIRECT LIFE SUPPORT MACHINES, OR WEAPONS SYSTEMS, IN WHICH
 * THE FAILURE OF THE SOFTWARE OR SYSTEM COULD LEAD DIRECTLY TO DEATH,
 * PERSONAL INJURY, OR SEVERE PHYSICAL OR ENVIRONMENTAL DAMAGE ("HIGH
 * RISK ACTIVITIES").  THE COPYRIGHT HOLDERS SPECIFICALLY DISCLAIM ANY
 * EXPRESS OR IMPLIED WARRANTY OF FITNESS FOR HIGH RISK ACTIVITIES.
 * 
 * __END_OF_JASPER_LICENSE__
 */

/*!
 * @file jas_math.h
 * @brief Math-Related Code
 */

#ifndef	JAS_MATH_H
#define	JAS_MATH_H

/******************************************************************************\
* Includes
\******************************************************************************/

/* The configuration header file should be included first. */
#include <jasper/jas_config.h>

#include <jasper/jas_types.h>

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
* Macros
\******************************************************************************/

#define JAS_KIBI	JAS_CAST(size_t, 1024)
#define JAS_MEBI	(JAS_KIBI * JAS_KIBI)

/* Compute the absolute value. */
#define	JAS_ABS(x) \
	(((x) >= 0) ? (x) : (-(x)))

/* Compute the minimum of two values. */
#define	JAS_MIN(x, y) \
	(((x) < (y)) ? (x) : (y))

/* Compute the maximum of two values. */
#define	JAS_MAX(x, y) \
	(((x) > (y)) ? (x) : (y))

/* Compute the remainder from division (where division is defined such
  that the remainder is always nonnegative). */
#define	JAS_MOD(x, y) \
	(((x) < 0) ? (((-x) % (y)) ? ((y) - ((-(x)) % (y))) : (0)) : ((x) % (y)))

/* Compute the integer with the specified number of least significant bits
  set to one. */
#define	JAS_ONES(n) \
  ((1 << (n)) - 1)

/******************************************************************************\
*
\******************************************************************************/

#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ > 6)
/* suppress clang warning "shifting a negative signed value is
   undefined" in the assertions below */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-negative-value"
#endif

JAS_ATTRIBUTE_CONST
JAS_ATTRIBUTE_DISABLE_USAN
inline static int jas_int_asr(int x, unsigned n)
{
	// Ensure that the shift of a negative value appears to behave as a
	// signed arithmetic shift.
	assert(((-1) >> 1) == -1);
	// The behavior is undefined when x is negative. */
	// We tacitly assume the behavior is equivalent to a signed
	// arithmetic right shift.
	return x >> n;
}

JAS_ATTRIBUTE_CONST
JAS_ATTRIBUTE_DISABLE_USAN
inline static int jas_int_asl(int x, unsigned n)
{
	// Ensure that the shift of a negative value appears to behave as a
	// signed arithmetic shift.
	assert(((-1) << 1) == -2);
	// The behavior is undefined when x is negative. */
	// We tacitly assume the behavior is equivalent to a signed
	// arithmetic left shift.
	return x << n;
}

JAS_ATTRIBUTE_CONST
JAS_ATTRIBUTE_DISABLE_USAN
inline static int_least32_t jas_least32_asr(int_least32_t x, unsigned n)
{
	// Ensure that the shift of a negative value appears to behave as a
	// signed arithmetic shift.
	assert(((JAS_CAST(int_least32_t, -1)) >> 1) == JAS_CAST(int_least32_t, -1));
	// The behavior is undefined when x is negative. */
	// We tacitly assume the behavior is equivalent to a signed
	// arithmetic right shift.
	return x >> n;
}

JAS_ATTRIBUTE_CONST
JAS_ATTRIBUTE_DISABLE_USAN
inline static int_least32_t jas_least32_asl(int_least32_t x, unsigned n)
{
	// Ensure that the shift of a negative value appears to behave as a
	// signed arithmetic shift.
	assert(((JAS_CAST(int_least32_t, -1)) << 1) == JAS_CAST(int_least32_t, -2));
	// The behavior is undefined when x is negative. */
	// We tacitly assume the behavior is equivalent to a signed
	// arithmetic left shift.
	return x << n;
}

JAS_ATTRIBUTE_CONST
JAS_ATTRIBUTE_DISABLE_USAN
inline static int_fast32_t jas_fast32_asr(int_fast32_t x, unsigned n)
{
	// Ensure that the shift of a negative value appears to behave as a
	// signed arithmetic shift.
	assert(((JAS_CAST(int_fast32_t, -1)) >> 1) == JAS_CAST(int_fast32_t, -1));
	// The behavior is undefined when x is negative. */
	// We tacitly assume the behavior is equivalent to a signed
	// arithmetic right shift.
	return x >> n;
}

JAS_ATTRIBUTE_CONST
JAS_ATTRIBUTE_DISABLE_USAN
inline static int_fast32_t jas_fast32_asl(int_fast32_t x, unsigned n)
{
	// Ensure that the shift of a negative value appears to behave as a
	// signed arithmetic shift.
	assert(((JAS_CAST(int_fast32_t, -1)) << 1) == JAS_CAST(int_fast32_t, -2));
	// The behavior is undefined when x is negative. */
	// We tacitly assume the behavior is equivalent to a signed
	// arithmetic left shift.
	return x << n;
}

#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ > 6)
#pragma GCC diagnostic pop
#endif

/******************************************************************************\
* Safe integer arithmetic (i.e., with overflow checking).
\******************************************************************************/

/* Compute the product of two size_t integers with overflow checking. */
inline static bool jas_safe_size_mul(size_t x, size_t y, size_t *result)
{
#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ > 5)
	size_t result_buffer;
	if (!result)
		result = &result_buffer;
	return !__builtin_mul_overflow(x, y, result);
#else
	/* Check if overflow would occur */
	if (x && y > SIZE_MAX / x) {
		/* Overflow would occur. */
		return false;
	}
	if (result) {
		*result = x * y;
	}
	return true;
#endif
}

/* Compute the product of three size_t integers with overflow checking. */
inline static bool jas_safe_size_mul3(size_t a, size_t b, size_t c,
  size_t *result)
{
	size_t tmp;
	if (!jas_safe_size_mul(a, b, &tmp) ||
	  !jas_safe_size_mul(tmp, c, &tmp)) {
		return false;
	}
	if (result) {
		*result = tmp;
	}
	return true;
}

/* Compute the sum of two size_t integers with overflow checking. */
inline static bool jas_safe_size_add(size_t x, size_t y, size_t *result)
{
#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ > 5)
	size_t result_buffer;
	if (!result)
		result = &result_buffer;
	return !__builtin_add_overflow(x, y, result);
#else
	if (y > SIZE_MAX - x) {
		return false;
	}
	if (result) {
		*result = x + y;
	}
	return true;
#endif
}

/* Compute the difference of two size_t integers with overflow checking. */
inline static bool jas_safe_size_sub(size_t x, size_t y, size_t *result)
{
#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ > 5)
	size_t result_buffer;
	if (!result)
		result = &result_buffer;
	return !__builtin_sub_overflow(x, y, result);
#else
	if (y > x) {
		return false;
	}
	if (result) {
		*result = x - y;
	}
	return true;
#endif
}

/* Compute the product of two int_fast32_t integers with overflow checking. */
inline static bool jas_safe_intfast32_mul(int_fast32_t x, int_fast32_t y,
  int_fast32_t *result)
{
#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ > 5)
	int_fast32_t result_buffer;
	if (!result)
		result = &result_buffer;
	return !__builtin_mul_overflow(x, y, result);
#else
	if (x > 0) {
		/* x is positive */
		if (y > 0) {
			/* x and y are positive */
			if (x > INT_FAST32_MAX / y) {
				return false;
			}
		} else {
			/* x positive, y nonpositive */
			if (y < INT_FAST32_MIN / x) {
				return false;
			}
		}
	} else {
		/* x is nonpositive */
		if (y > 0) {
			/* x is nonpositive, y is positive */
			if (x < INT_FAST32_MIN / y) {
				return false;
			}
		} else { /* x and y are nonpositive */
			if (x != 0 && y < INT_FAST32_MAX / x) {
				return false;
			}
		}
	}

	if (result) {
		*result = x * y;
	}
	return true;
#endif
}

/* Compute the product of three int_fast32_t integers with overflow checking. */
inline static bool jas_safe_intfast32_mul3(int_fast32_t a, int_fast32_t b,
  int_fast32_t c, int_fast32_t *result)
{
	int_fast32_t tmp;
	if (!jas_safe_intfast32_mul(a, b, &tmp) ||
	  !jas_safe_intfast32_mul(tmp, c, &tmp)) {
		return false;
	}
	if (result) {
		*result = tmp;
	}
	return true;
}

/* Compute the sum of two int_fast32_t integers with overflow checking. */
inline static bool jas_safe_intfast32_add(int_fast32_t x, int_fast32_t y,
  int_fast32_t *result)
{
#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ > 5)
	int_fast32_t result_buffer;
	if (!result)
		result = &result_buffer;
	return !__builtin_add_overflow(x, y, result);
#else
	if ((y > 0 && x > INT_FAST32_MAX - y) ||
	  (y < 0 && x < INT_FAST32_MIN - y)) {
		return false;
	}
	if (result) {
		*result = x + y;
	}
	return true;
#endif
}

#if 0
/*
This function is potentially useful but not currently used.
So, it is commented out.
*/
inline static bool jas_safe_uint_mul(unsigned x, unsigned y, unsigned *result)
{
	/* Check if overflow would occur */
	if (x && y > UINT_MAX / x) {
		/* Overflow would occur. */
		return false;
	}
	if (result) {
		*result = x * y;
	}
	return true;
}
#endif

#ifdef __cplusplus
}
#endif

#endif
