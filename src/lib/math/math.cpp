#include <memory/functiontool.h>
#include <memory/casttool.h>
#include <cmath>

using namespace mint;
using namespace std;

// Trigonometric functions

MINT_FUNCTION(mint_math_cos, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(cos(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_sin, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(sin(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_sin_cos, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());

	WeakReference result = create_iterator();

	iterator_insert(result.data<Iterator>(), create_number(sin(to_number(cursor, value))));
	iterator_insert(result.data<Iterator>(), create_number(cos(to_number(cursor, value))));
	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_math_tan, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(tan(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_acos, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(acos(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_asin, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(asin(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_atan, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(atan(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_atan, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference valueY = move(helper.popParameter());
	WeakReference valueX = move(helper.popParameter());
	helper.returnValue(create_number(atan2(to_number(cursor, valueX), to_number(cursor, valueY))));
}

// Hyperbolic functions

MINT_FUNCTION(mint_math_cosh, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(cosh(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_sinh, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(sinh(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_tanh, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(tanh(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_acosh, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(acosh(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_asinh, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(asinh(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_atanh, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(atanh(to_number(cursor, value))));
}

// Exponential and logarithmic functions

MINT_FUNCTION(mint_math_exp, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(exp(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_frexp, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());

	WeakReference result = create_iterator();
	int exponent = 0;

	iterator_insert(result.data<Iterator>(), create_number(frexp(to_number(cursor, value), &exponent)));
	iterator_insert(result.data<Iterator>(), create_number(static_cast<double>(exponent)));
	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_math_ldexp, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference exponent = move(helper.popParameter());
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(ldexp(to_number(cursor, value), static_cast<int>(to_integer(cursor, exponent)))));
}

MINT_FUNCTION(mint_math_log, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(log(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_log10, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(log10(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_modf, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	WeakReference result = create_iterator();
	double intpart = 0.;
	double fractional = modf(to_number(cursor, value), &intpart);
	iterator_insert(result.data<Iterator>(), create_number(intpart));
	iterator_insert(result.data<Iterator>(), create_number(fractional));
	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_math_exp2, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(exp2(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_expm1, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(expm1(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_ilogb, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(ilogb(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_log1p, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(log1p(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_log2, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(log2(to_number(cursor, value))));
}

MINT_FUNCTION(mint_math_logb, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(logb(to_number(cursor, value))));
}

// Power functions

MINT_FUNCTION(mint_math_pow, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference valueY = move(helper.popParameter());
	WeakReference valueX = move(helper.popParameter());
	helper.returnValue(create_number(pow(to_number(cursor, valueX), to_number(cursor, valueY))));
}

MINT_FUNCTION(mint_math_sqrt, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = move(helper.popParameter());
	helper.returnValue(create_number(sqrt(to_number(cursor, value))));
}

