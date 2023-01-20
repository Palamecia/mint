#include <memory/functiontool.h>
#include <memory/builtin/iterator.h>
#include <memory/casttool.h>
#include <cmath>

using namespace mint;
using namespace std;

// Trigonometric functions

MINT_FUNCTION(mint_math_cos, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(cos(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_sin, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(sin(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_sin_cos, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();

	WeakReference result = create_iterator();

	iterator_insert(result.data<Iterator>(), create_number(sin(to_number(cursor, value))));
	iterator_insert(result.data<Iterator>(), create_number(cos(to_number(cursor, value))));
	helper.returnValue(std::move(result));
}

MINT_FUNCTION(mint_math_tan, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(tan(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_acos, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(acos(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_asin, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(asin(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_atan, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(atan(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_atan, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &yValue = helper.popParameter();
	Reference &xValue = helper.popParameter();
	helper.returnValue(create_number(atan2(to_number(cursor, xValue), to_number(cursor, yValue))));
}

// Hyperbolic functions

MINT_FUNCTION(mint_math_cosh, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(cosh(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_sinh, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(sinh(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_tanh, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(tanh(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_acosh, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(acosh(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_asinh, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(asinh(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_atanh, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(atanh(to_number(cursor, value))));
}

// Exponential and logarithmic functions

MINT_FUNCTION(mint_math_exp, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(exp(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_frexp, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();

	WeakReference result = create_iterator();
	int exponent = 0;

	iterator_insert(result.data<Iterator>(), create_number(frexp(to_number(cursor, value), &exponent)));
	iterator_insert(result.data<Iterator>(), create_number(static_cast<double>(exponent)));
	helper.returnValue(std::move(result));
}

MINT_FUNCTION(mint_math_ldexp, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &exponent = helper.popParameter();
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(ldexp(to_number(cursor, value), static_cast<int>(to_integer(cursor, exponent)))));
}

MINT_FUNCTION(mint_math_log, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(log(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_log10, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(log10(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_modf, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	WeakReference result = create_iterator();
	double intpart = 0.;
	double fractional = modf(to_number(cursor, value), &intpart);
	iterator_insert(result.data<Iterator>(), create_number(intpart));
	iterator_insert(result.data<Iterator>(), create_number(fractional));
	helper.returnValue(std::move(result));
}

MINT_FUNCTION(mint_math_exp2, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(exp2(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_expm1, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(expm1(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_ilogb, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(ilogb(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_log1p, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(log1p(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_log2, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(log2(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_logb, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(logb(to_number(cursor, value))));
}


MINT_FUNCTION(mint_math_scalbn, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &exponent = helper.popParameter();
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(scalbln(to_number(cursor, value), to_integer(cursor, exponent))));
}

// Power functions

MINT_FUNCTION(mint_math_pow, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &yValue = helper.popParameter();
	Reference &xValue = helper.popParameter();
	helper.returnValue(create_number(pow(to_number(cursor, xValue), to_number(cursor, yValue))));
}

MINT_FUNCTION(mint_math_sqrt, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(sqrt(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_cbrt, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(cbrt(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_hypot, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &yValue = helper.popParameter();
	Reference &xValue = helper.popParameter();
	helper.returnValue(create_number(hypot(to_number(cursor, xValue), to_number(cursor, yValue))));
}

// Error and gamma functions

MINT_FUNCTION(mint_math_erf, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(erf(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_erfc, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(erfc(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_tgamma, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(tgamma(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_lgamma, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(lgamma(to_number(cursor, value))));
}

// Rounding and remainder functions

MINT_FUNCTION(mint_math_ceil, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(ceil(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_floor, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(floor(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_fmod, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &yValue = helper.popParameter();
	Reference &xValue = helper.popParameter();
	helper.returnValue(create_number(fmod(to_number(cursor, xValue), to_number(cursor, yValue))));
}

MINT_FUNCTION(mint_math_fabs, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(fabs(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_trunc, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(trunc(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_round, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(round(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_rint, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(rint(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_nearbyint, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(nearbyint(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_remainder, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &yValue = helper.popParameter();
	Reference &xValue = helper.popParameter();
	helper.returnValue(create_number(remainder(to_number(cursor, xValue), to_number(cursor, yValue))));
}

MINT_FUNCTION(mint_math_remquo, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &yValue = helper.popParameter();
	Reference &xValue = helper.popParameter();
	int quot = 0;
	WeakReference result = create_iterator();
	iterator_insert(result.data<Iterator>(), create_number(remquo(to_number(cursor, xValue), to_number(cursor, yValue), &quot)));
	iterator_insert(result.data<Iterator>(), create_number(static_cast<double>(quot)));
	helper.returnValue(std::move(result));
}


// Floating-point manipulation functions

MINT_FUNCTION(mint_math_signbit, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_boolean(signbit(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_copysign, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &yValue = helper.popParameter();
	Reference &xValue = helper.popParameter();
	helper.returnValue(create_number(copysign(to_number(cursor, xValue), to_number(cursor, yValue))));
}

MINT_FUNCTION(mint_math_isnan, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_boolean(isnan(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_nan, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	helper.returnValue(create_number(nan("")));
}

MINT_FUNCTION(mint_math_isinf, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_boolean(isinf(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_inf, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &sign = helper.popParameter();
	helper.returnValue(create_number(copysign(numeric_limits<double>::infinity(), to_number(cursor, sign))));
}

MINT_FUNCTION(mint_math_nextafter, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &yValue = helper.popParameter();
	Reference &xValue = helper.popParameter();
	helper.returnValue(create_number(nextafter(to_number(cursor, xValue), to_number(cursor, yValue))));
}

MINT_FUNCTION(mint_math_nexttoward, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &yValue = helper.popParameter();
	Reference &xValue = helper.popParameter();
	helper.returnValue(create_number(nexttoward(to_number(cursor, xValue), to_number(cursor, yValue))));
}

// Minimum, maximum, difference functions

MINT_FUNCTION(mint_math_fdim, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &yValue = helper.popParameter();
	Reference &xValue = helper.popParameter();
	helper.returnValue(create_number(fdim(to_number(cursor, xValue), to_number(cursor, yValue))));
}

// Other functions

MINT_FUNCTION(mint_math_fma, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &zValue = helper.popParameter();
	Reference &yValue = helper.popParameter();
	Reference &xValue = helper.popParameter();
	helper.returnValue(create_number(fma(to_number(cursor, xValue), to_number(cursor, yValue), to_number(cursor, zValue))));
}
