/**
 * Copyright (c) 2026 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "mint/memory/function_tools.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/reference.h"
#include <cmath>
#include <limits>

namespace {

mint::Reference mint_math_cos(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(cos(to_number(cursor, value)));
}

mint::Reference mint_math_sin(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(sin(to_number(cursor, value)));
}

mint::Reference mint_math_sin_cos(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_iterator_from(cursor, mint::create_number(sin(to_number(cursor, value))),
	    mint::create_number(cos(to_number(cursor, value))));
}

mint::Reference mint_math_tan(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(tan(to_number(cursor, value)));
}

mint::Reference mint_math_acos(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(acos(to_number(cursor, value)));
}

mint::Reference mint_math_asin(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(asin(to_number(cursor, value)));
}

mint::Reference mint_math_atan(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(atan(to_number(cursor, value)));
}

mint::Reference mint_math_atan(mint::Cursor& cursor, const mint::Reference& x_value, const mint::Reference& y_value) {
	return mint::create_number(atan2(to_number(cursor, x_value), to_number(cursor, y_value)));
}

mint::Reference mint_math_cosh(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(cosh(to_number(cursor, value)));
}

mint::Reference mint_math_sinh(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(sinh(to_number(cursor, value)));
}

mint::Reference mint_math_tanh(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(tanh(to_number(cursor, value)));
}

mint::Reference mint_math_acosh(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(acosh(to_number(cursor, value)));
}

mint::Reference mint_math_asinh(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(asinh(to_number(cursor, value)));
}

mint::Reference mint_math_atanh(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(atanh(to_number(cursor, value)));
}

mint::Reference mint_math_exp(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(exp(to_number(cursor, value)));
}

mint::Reference mint_math_frexp(mint::Cursor& cursor, const mint::Reference& value) {
	int exponent = 0;
	return mint::create_iterator_from(cursor, mint::create_number(frexp(to_number(cursor, value), &exponent)),
	    mint::create_signed_number(exponent));
}

mint::Reference mint_math_ldexp(mint::Cursor& cursor, const mint::Reference& value, const mint::Reference& exponent) {
	return mint::create_number(ldexp(to_number(cursor, value), to_integer<int>(cursor, exponent)));
}

mint::Reference mint_math_log(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(log(to_number(cursor, value)));
}

mint::Reference mint_math_log10(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(log10(to_number(cursor, value)));
}

mint::Reference mint_math_modf(mint::Cursor& cursor, const mint::Reference& value) {
	double intpart = 0.;
	const auto fractional = modf(to_number(cursor, value), &intpart);
	return mint::create_iterator_from(cursor, mint::create_number(intpart), mint::create_number(fractional));
}

mint::Reference mint_math_exp2(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(exp2(to_number(cursor, value)));
}

mint::Reference mint_math_expm1(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(expm1(to_number(cursor, value)));
}

mint::Reference mint_math_ilogb(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(ilogb(to_number(cursor, value)));
}

mint::Reference mint_math_log1p(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(log1p(to_number(cursor, value)));
}

mint::Reference mint_math_log2(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(log2(to_number(cursor, value)));
}

mint::Reference mint_math_logb(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(logb(to_number(cursor, value)));
}

mint::Reference mint_math_scalbn(mint::Cursor& cursor, const mint::Reference& value, const mint::Reference& exponent) {
	return mint::create_number(scalbln(to_number(cursor, value), to_integer<long>(cursor, exponent)));
}

mint::Reference mint_math_pow(mint::Cursor& cursor, const mint::Reference& x_value, const mint::Reference& y_value) {
	return mint::create_number(pow(to_number(cursor, x_value), to_number(cursor, y_value)));
}

mint::Reference mint_math_sqrt(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(sqrt(to_number(cursor, value)));
}

mint::Reference mint_math_cbrt(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(cbrt(to_number(cursor, value)));
}

mint::Reference mint_math_hypot(mint::Cursor& cursor, const mint::Reference& x_value, const mint::Reference& y_value) {
	return mint::create_number(hypot(to_number(cursor, x_value), to_number(cursor, y_value)));
}

mint::Reference mint_math_erf(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(erf(to_number(cursor, value)));
}

mint::Reference mint_math_erfc(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(erfc(to_number(cursor, value)));
}

mint::Reference mint_math_tgamma(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(tgamma(to_number(cursor, value)));
}

mint::Reference mint_math_lgamma(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(lgamma(to_number(cursor, value)));
}

mint::Reference mint_math_ceil(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(ceil(to_number(cursor, value)));
}

mint::Reference mint_math_floor(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(floor(to_number(cursor, value)));
}

mint::Reference mint_math_fmod(mint::Cursor& cursor, const mint::Reference& x_value, const mint::Reference& y_value) {
	return mint::create_number(fmod(to_number(cursor, x_value), to_number(cursor, y_value)));
}

mint::Reference mint_math_fabs(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(fabs(to_number(cursor, value)));
}

mint::Reference mint_math_trunc(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(trunc(to_number(cursor, value)));
}

mint::Reference mint_math_round(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(round(to_number(cursor, value)));
}

mint::Reference mint_math_rint(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(rint(to_number(cursor, value)));
}

mint::Reference mint_math_nearbyint(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(nearbyint(to_number(cursor, value)));
}

mint::Reference mint_math_remainder(mint::Cursor& cursor, const mint::Reference& x_value,
    const mint::Reference& y_value) {
	return mint::create_number(remainder(to_number(cursor, x_value), to_number(cursor, y_value)));
}

mint::Reference mint_math_remquo(mint::Cursor& cursor, const mint::Reference& x_value, const mint::Reference& y_value) {
	int quot = 0;
	return mint::create_iterator_from(cursor,
	    mint::create_number(remquo(to_number(cursor, x_value), to_number(cursor, y_value), &quot)),
	    mint::create_signed_number(quot));
}

mint::Reference mint_math_signbit(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_boolean(std::signbit(to_number(cursor, value)));
}

mint::Reference mint_math_copysign(mint::Cursor& cursor, const mint::Reference& x_value,
    const mint::Reference& y_value) {
	return mint::create_number(copysign(to_number(cursor, x_value), to_number(cursor, y_value)));
}

mint::Reference mint_math_isnan(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_boolean(std::isnan(to_number(cursor, value)));
}

mint::Reference mint_math_nan(mint::Cursor& /*cursor*/) {
	return mint::create_number(nan(""));
}

mint::Reference mint_math_isinf(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_boolean(std::isinf(to_number(cursor, value)));
}

mint::Reference mint_math_inf(mint::Cursor& cursor, const mint::Reference& sign) {
	return mint::create_number(copysign(std::numeric_limits<double>::infinity(), to_number(cursor, sign)));
}

mint::Reference mint_math_nextafter(mint::Cursor& cursor, const mint::Reference& x_value,
    const mint::Reference& y_value) {
	return mint::create_number(nextafter(to_number(cursor, x_value), to_number(cursor, y_value)));
}

mint::Reference mint_math_nexttoward(mint::Cursor& cursor, const mint::Reference& x_value,
    const mint::Reference& y_value) {
	return mint::create_number(nexttoward(to_number(cursor, x_value), to_number(cursor, y_value)));
}

mint::Reference mint_math_fdim(mint::Cursor& cursor, const mint::Reference& x_value, const mint::Reference& y_value) {
	return mint::create_number(fdim(to_number(cursor, x_value), to_number(cursor, y_value)));
}

mint::Reference mint_math_fma(mint::Cursor& cursor, const mint::Reference& x_value, const mint::Reference& y_value,
    mint::Reference& z_value) {
	return mint::create_number(fma(to_number(cursor, x_value), to_number(cursor, y_value), to_number(cursor, z_value)));
}

}

// Trigonometric functions

MINT_EXPORT_FUNCTION(mint_math_cos, 1);
MINT_EXPORT_FUNCTION(mint_math_sin, 1);
MINT_EXPORT_FUNCTION(mint_math_sin_cos, 1);
MINT_EXPORT_FUNCTION(mint_math_tan, 1);
MINT_EXPORT_FUNCTION(mint_math_acos, 1);
MINT_EXPORT_FUNCTION(mint_math_asin, 1);
MINT_EXPORT_FUNCTION_OVERLOAD(mint_math_atan, 1, mint::Cursor&, const mint::Reference&);
MINT_EXPORT_FUNCTION_OVERLOAD(mint_math_atan, 2, mint::Cursor&, const mint::Reference&, const mint::Reference&);

// Hyperbolic functions

MINT_EXPORT_FUNCTION(mint_math_cosh, 1);
MINT_EXPORT_FUNCTION(mint_math_sinh, 1);
MINT_EXPORT_FUNCTION(mint_math_tanh, 1);
MINT_EXPORT_FUNCTION(mint_math_acosh, 1);
MINT_EXPORT_FUNCTION(mint_math_asinh, 1);
MINT_EXPORT_FUNCTION(mint_math_atanh, 1);

// Exponential and logarithmic functions

MINT_EXPORT_FUNCTION(mint_math_exp, 1);
MINT_EXPORT_FUNCTION(mint_math_frexp, 1);
MINT_EXPORT_FUNCTION(mint_math_ldexp, 2);
MINT_EXPORT_FUNCTION(mint_math_log, 1);
MINT_EXPORT_FUNCTION(mint_math_log10, 1);
MINT_EXPORT_FUNCTION(mint_math_modf, 1);
MINT_EXPORT_FUNCTION(mint_math_exp2, 1);
MINT_EXPORT_FUNCTION(mint_math_expm1, 1);
MINT_EXPORT_FUNCTION(mint_math_ilogb, 1);
MINT_EXPORT_FUNCTION(mint_math_log1p, 1);
MINT_EXPORT_FUNCTION(mint_math_log2, 1);
MINT_EXPORT_FUNCTION(mint_math_logb, 1);
MINT_EXPORT_FUNCTION(mint_math_scalbn, 2);

// Power functions

MINT_EXPORT_FUNCTION(mint_math_pow, 2);
MINT_EXPORT_FUNCTION(mint_math_sqrt, 1);
MINT_EXPORT_FUNCTION(mint_math_cbrt, 1);
MINT_EXPORT_FUNCTION(mint_math_hypot, 2);

// Error and gamma functions

MINT_EXPORT_FUNCTION(mint_math_erf, 1);
MINT_EXPORT_FUNCTION(mint_math_erfc, 1);
MINT_EXPORT_FUNCTION(mint_math_tgamma, 1);
MINT_EXPORT_FUNCTION(mint_math_lgamma, 1);

// Rounding and remainder functions

MINT_EXPORT_FUNCTION(mint_math_ceil, 1);
MINT_EXPORT_FUNCTION(mint_math_floor, 1);
MINT_EXPORT_FUNCTION(mint_math_fmod, 2);
MINT_EXPORT_FUNCTION(mint_math_fabs, 1);
MINT_EXPORT_FUNCTION(mint_math_trunc, 1);
MINT_EXPORT_FUNCTION(mint_math_round, 1);
MINT_EXPORT_FUNCTION(mint_math_rint, 1);
MINT_EXPORT_FUNCTION(mint_math_nearbyint, 1);
MINT_EXPORT_FUNCTION(mint_math_remainder, 2);
MINT_EXPORT_FUNCTION(mint_math_remquo, 2);

// Floating-point manipulation functions

MINT_EXPORT_FUNCTION(mint_math_signbit, 1);
MINT_EXPORT_FUNCTION(mint_math_copysign, 2);
MINT_EXPORT_FUNCTION(mint_math_isnan, 1);
MINT_EXPORT_FUNCTION(mint_math_nan, 0);
MINT_EXPORT_FUNCTION(mint_math_isinf, 1);
MINT_EXPORT_FUNCTION(mint_math_inf, 1);
MINT_EXPORT_FUNCTION(mint_math_nextafter, 2);
MINT_EXPORT_FUNCTION(mint_math_nexttoward, 2);

// Minimum, maximum, difference functions

MINT_EXPORT_FUNCTION(mint_math_fdim, 2);

// Other functions

MINT_EXPORT_FUNCTION(mint_math_fma, 3);
