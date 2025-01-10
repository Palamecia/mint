/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#include <mint/memory/functiontool.h>
#include <mint/memory/memorytool.h>
#include <mint/memory/casttool.h>
#include <mint/system/error.h>
#include <cinttypes>
#include <cmath>

using namespace mint;

namespace symbols {

static const Symbol d_ptr("d_ptr");

static const std::string int8("int8");
static const std::string uint8("uint8");
static const std::string int16("int16");
static const std::string uint16("uint16");
static const std::string int32("int32");
static const std::string uint32("uint32");
static const std::string int64("int64");
static const std::string uint64("uint64");

}

template<typename number_t>
class fixed_int {
	fixed_int() = delete;
public:
	static number_t *create(Cursor *cursor, WeakReference &value) {

		switch (value.data()->format) {
		case Data::FMT_NONE:
		case Data::FMT_NULL:
			return new number_t(0);
			break;
		case Data::FMT_NUMBER:
		case Data::FMT_BOOLEAN:
			return new number_t(static_cast<number_t>(to_integer(cursor, value)));
		case Data::FMT_OBJECT:
			switch (value.data<Object>()->metadata->metatype()) {
			case Class::STRING:
				return new number_t(from_string(to_string(value)));
			case Class::OBJECT:
				if (value.data<Object>()->metadata->full_name() == symbols::int8) {
					return new number_t(static_cast<number_t>(*get_d_ptr(value).data<LibObject<int8_t>>()->impl));
				}
				if (value.data<Object>()->metadata->full_name() == symbols::int16) {
					return new number_t(static_cast<number_t>(*get_d_ptr(value).data<LibObject<int16_t>>()->impl));
				}
				if (value.data<Object>()->metadata->full_name()== symbols::int32) {
					return new number_t(static_cast<number_t>(*get_d_ptr(value).data<LibObject<int32_t>>()->impl));
				}
				if (value.data<Object>()->metadata->full_name()== symbols::int64) {
					return new number_t(static_cast<number_t>(*get_d_ptr(value).data<LibObject<int64_t>>()->impl));
				}
				if (value.data<Object>()->metadata->full_name()== symbols::uint8) {
					return new number_t(static_cast<number_t>(*get_d_ptr(value).data<LibObject<uint8_t>>()->impl));
				}
				if (value.data<Object>()->metadata->full_name()== symbols::uint16) {
					return new number_t(static_cast<number_t>(*get_d_ptr(value).data<LibObject<uint16_t>>()->impl));
				}
				if (value.data<Object>()->metadata->full_name()== symbols::uint32) {
					return new number_t(static_cast<number_t>(*get_d_ptr(value).data<LibObject<uint32_t>>()->impl));
				}
				if (value.data<Object>()->metadata->full_name()== symbols::uint64) {
					return new number_t(static_cast<number_t>(*get_d_ptr(value).data<LibObject<uint64_t>>()->impl));
				}
				{
					std::string type = type_name(value);
					error("no valid conversion from %s to %s", type.c_str(), name());
				}
				break;
			default:
				{
					std::string type = type_name(value);
					error("no valid conversion from %s to %s", type.c_str(), name());
				}
				break;
			}
			break;
		default:
			std::string type = type_name(value);
			error("no valid conversion from %s to %s", type.c_str(), name());
		}

		return nullptr;
	}

	static Reference &&copy_operator(Reference &&value, const Reference &other) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		data = *get_d_ptr(other).data<LibObject<number_t>>()->impl;
		return std::move(value);
	}

	static WeakReference call_operator(const Reference &value) {
		return create_object(new number_t(*get_d_ptr(value).data<LibObject<number_t>>()->impl));
	}

	static Reference &&add_operator(Reference &&value, const Reference &other) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		data += *get_d_ptr(other).data<LibObject<number_t>>()->impl;
		return std::move(value);
	}

	static Reference &&sub_operator(Reference &&value, const Reference &other) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		data -= *get_d_ptr(other).data<LibObject<number_t>>()->impl;
		return std::move(value);
	}

	static Reference &&mul_operator(Reference &&value, const Reference &other) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		data *= *get_d_ptr(other).data<LibObject<number_t>>()->impl;
		return std::move(value);
	}

	static Reference &&div_operator(Reference &&value, const Reference &other) {
		if (number_t divider = *get_d_ptr(other).data<LibObject<number_t>>()->impl) {
			number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
			data /= divider;
		}
		else {
			error("division by zero");
		}
		return std::move(value);
	}

	static Reference &&pow_operator(Reference &&value, const Reference &other) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		data = static_cast<number_t>(pow(data, *get_d_ptr(other).data<LibObject<number_t>>()->impl));
		return std::move(value);
	}

	static Reference &&mod_operator(Reference &&value, const Reference &other) {
		if (number_t divider = *get_d_ptr(other).data<LibObject<number_t>>()->impl) {
			number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
			data %= divider;
		}
		else {
			error("modulo by zero");
		}
		return std::move(value);
	}

	static WeakReference eq_operator(const Reference &value, const Reference &other) {
		const number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		return create_boolean(data == *get_d_ptr(other).data<LibObject<number_t>>()->impl);
	}

	static WeakReference ne_operator(const Reference &value, const Reference &other) {
		const number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		return create_boolean(data != *get_d_ptr(other).data<LibObject<number_t>>()->impl);
	}

	static WeakReference lt_operator(const Reference &value, const Reference &other) {
		const number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		return create_boolean(data < *get_d_ptr(other).data<LibObject<number_t>>()->impl);
	}

	static WeakReference gt_operator(const Reference &value, const Reference &other) {
		const number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		return create_boolean(data > *get_d_ptr(other).data<LibObject<number_t>>()->impl);
	}

	static WeakReference le_operator(const Reference &value, const Reference &other) {
		const number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		return create_boolean(data <= *get_d_ptr(other).data<LibObject<number_t>>()->impl);
	}

	static WeakReference ge_operator(const Reference &value, const Reference &other) {
		const number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		return create_boolean(data >= *get_d_ptr(other).data<LibObject<number_t>>()->impl);
	}

	static Reference &&and_operator(Reference &&value, Reference &&other) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		return std::move(data ? other : value);
	}

	static Reference &&or_operator(Reference &&value, Reference &&other) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		return std::move(data ? value : other);
	}

	static Reference &&band_operator(Reference &&value, const Reference &other) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		data &= *get_d_ptr(other).data<LibObject<number_t>>()->impl;
		return std::move(value);
	}

	static Reference &&bor_operator(Reference &&value, const Reference &other) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		data |= *get_d_ptr(other).data<LibObject<number_t>>()->impl;
		return std::move(value);
	}

	static Reference &&xor_operator(Reference &&value, const Reference &other) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		data ^= *get_d_ptr(other).data<LibObject<number_t>>()->impl;
		return std::move(value);
	}

	static Reference &&inc_operator(Reference &&value) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		++data;
		return std::move(value);
	}

	static Reference &&dec_operator(Reference &&value) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		--data;
		return std::move(value);
	}

	static WeakReference not_operator(const Reference &value) {
		const number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		return create_boolean(!data);
	}

	static Reference &&compl_operator(Reference &&value) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		data = ~data;
		return std::move(value);
	}

	static Reference &&pos_operator(Reference &&value) {
		return std::move(value);
	}

	static Reference &&neg_operator(Reference &&value) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		data = -data;
		return std::move(value);
	}

	static Reference &&shift_left_operator(Reference &&value, const Reference &other) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		data = data << *get_d_ptr(other).data<LibObject<number_t>>()->impl;
		return std::move(value);
	}

	static Reference &&shift_right_operator(Reference &&value, const Reference &other) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		data = data >> *get_d_ptr(other).data<LibObject<number_t>>()->impl;
		return std::move(value);
	}

	static WeakReference inclusive_range_operator(const Reference &value, const Reference &other) {
		const number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		return Iterator::fromInclusiveRange(static_cast<double>(data), static_cast<double>(*get_d_ptr(other).data<LibObject<number_t>>()->impl));
	}

	static WeakReference exclusive_range_operator(const Reference &value, const Reference &other) {
		const number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		return Iterator::fromExclusiveRange(static_cast<double>(data), static_cast<double>(*get_d_ptr(other).data<LibObject<number_t>>()->impl));
	}

	static WeakReference subscript_operator(const Reference &value, intmax_t index) {
		const number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		return create_object(new number_t(number_t(data / static_cast<number_t>(pow(10, index)) % 10)));
	}

	static Reference &&subscript_move_operator(Reference &&value, intmax_t index, const Reference &other) {
		number_t &data = *get_d_ptr(value).data<LibObject<number_t>>()->impl;
		data -= static_cast<number_t>(number_t(data / static_cast<number_t>(pow(10, index))) % 10) * static_cast<number_t>(pow(10, index));
		data += number_t(*get_d_ptr(other).data<LibObject<number_t>>()->impl * static_cast<number_t>(pow(10, index)));
		return std::move(value);
	}

	static WeakReference to_number(const Reference &value) {
		return create_number(static_cast<double>(*get_d_ptr(value).data<LibObject<number_t>>()->impl));
	}

private:
	static const char *name();

	static WeakReference get_d_ptr(const Reference &reference) {

		Object *object = reference.data<Object>();
		auto it = object->metadata->members().find(symbols::d_ptr);

		if (it != object->metadata->members().end()) {
			return WeakReference::share(object->data[it->second->offset]);
		}

		return WeakReference();
	}

	static number_t from_string(const std::string &str) {

		static auto strtonum = [] (const char *nptr, char **endptr, int base) -> number_t {
			return std::is_signed<number_t>::value ?
						static_cast<number_t>(strtoimax(nptr, endptr, base)) : static_cast<number_t>(strtoumax(nptr, endptr, base));
		};

		const char *value = str.c_str();

		if (value[0] == '0') {
			switch (value[1]) {
			case 'b':
			case 'B':
				return strtonum(value + 2, nullptr, 2);

			case 'o':
			case 'O':
				return strtonum(value + 2, nullptr, 8);

			case 'x':
			case 'X':
				return strtonum(value + 2, nullptr, 16);

			default:
				break;
			}
		}

		return strtonum(value, nullptr, 10);
	}
};

template<>
const char *fixed_int<int8_t>::name() {
	return symbols::int8.c_str();
}

MINT_FUNCTION(mint_int8_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(create_object(fixed_int<int8_t>::create(cursor, value)));
}

MINT_FUNCTION(mint_int8_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	delete value.data<LibObject<int8_t>>()->impl;
}

MINT_FUNCTION(mint_int8_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference source = std::move(helper.pop_parameter());
	WeakReference target = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::copy_operator(std::move(target), source));
}

MINT_FUNCTION(mint_int8_call, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::call_operator(std::move(value)));
}

MINT_FUNCTION(mint_int8_add, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::add_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int8_sub, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::sub_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int8_mul, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::mul_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int8_div, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::div_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int8_pow, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::pow_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int8_mod, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::mod_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int8_eq, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::eq_operator(value, other));
}

MINT_FUNCTION(mint_int8_ne, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::ne_operator(value, other));
}

MINT_FUNCTION(mint_int8_lt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::lt_operator(value, other));
}

MINT_FUNCTION(mint_int8_gt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::gt_operator(value, other));
}

MINT_FUNCTION(mint_int8_le, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::le_operator(value, other));
}

MINT_FUNCTION(mint_int8_ge, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::ge_operator(value, other));
}

MINT_FUNCTION(mint_int8_and, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::and_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_int8_or, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::or_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_int8_band, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::band_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int8_bor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::bor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int8_xor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::xor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int8_inc, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::inc_operator(std::move(value)));
}

MINT_FUNCTION(mint_int8_dec, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::dec_operator(std::move(value)));
}

MINT_FUNCTION(mint_int8_not, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::not_operator(value));
}

MINT_FUNCTION(mint_int8_compl, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::compl_operator(std::move(value)));
}

MINT_FUNCTION(mint_int8_pos, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::pos_operator(std::move(value)));
}

MINT_FUNCTION(mint_int8_neg, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::neg_operator(std::move(value)));
}

MINT_FUNCTION(mint_int8_shift_left, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::shift_left_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int8_shift_right, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::shift_right_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int8_inclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::inclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_int8_exclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::exclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_int8_subscript, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::subscript_operator(value, to_integer(cursor, index)));
}

MINT_FUNCTION(mint_int8_subscript_move, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::subscript_move_operator(std::move(value), to_integer(cursor, index), other));
}

MINT_FUNCTION(mint_int8_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int8_t>::to_number(value));
}

template<>
const char *fixed_int<int16_t>::name() {
	return symbols::int16.c_str();
}

MINT_FUNCTION(mint_int16_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(create_object(fixed_int<int16_t>::create(cursor, value)));
}

MINT_FUNCTION(mint_int16_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	delete value.data<LibObject<int16_t>>()->impl;
}

MINT_FUNCTION(mint_int16_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference source = std::move(helper.pop_parameter());
	WeakReference target = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::copy_operator(std::move(target), source));
}

MINT_FUNCTION(mint_int16_call, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::call_operator(std::move(value)));
}

MINT_FUNCTION(mint_int16_add, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::add_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int16_sub, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::sub_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int16_mul, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::mul_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int16_div, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::div_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int16_pow, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::pow_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int16_mod, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::mod_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int16_eq, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::eq_operator(value, other));
}

MINT_FUNCTION(mint_int16_ne, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::ne_operator(value, other));
}

MINT_FUNCTION(mint_int16_lt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::lt_operator(value, other));
}

MINT_FUNCTION(mint_int16_gt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::gt_operator(value, other));
}

MINT_FUNCTION(mint_int16_le, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::le_operator(value, other));
}

MINT_FUNCTION(mint_int16_ge, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::ge_operator(value, other));
}

MINT_FUNCTION(mint_int16_and, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::and_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_int16_or, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::or_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_int16_band, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::band_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int16_bor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::bor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int16_xor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::xor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int16_inc, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::inc_operator(std::move(value)));
}

MINT_FUNCTION(mint_int16_dec, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::dec_operator(std::move(value)));
}

MINT_FUNCTION(mint_int16_not, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::not_operator(value));
}

MINT_FUNCTION(mint_int16_compl, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::compl_operator(std::move(value)));
}

MINT_FUNCTION(mint_int16_pos, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::pos_operator(std::move(value)));
}

MINT_FUNCTION(mint_int16_neg, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::neg_operator(std::move(value)));
}

MINT_FUNCTION(mint_int16_shift_left, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::shift_left_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int16_shift_right, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::shift_right_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int16_inclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::inclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_int16_exclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::exclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_int16_subscript, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::subscript_operator(value, to_integer(cursor, index)));
}

MINT_FUNCTION(mint_int16_subscript_move, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::subscript_move_operator(std::move(value), to_integer(cursor, index), other));
}

MINT_FUNCTION(mint_int16_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int16_t>::to_number(value));
}

template<>
const char *fixed_int<int32_t>::name() {
	return symbols::int32.c_str();
}

MINT_FUNCTION(mint_int32_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(create_object(fixed_int<int32_t>::create(cursor, value)));
}

MINT_FUNCTION(mint_int32_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	delete value.data<LibObject<int32_t>>()->impl;
}

MINT_FUNCTION(mint_int32_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference source = std::move(helper.pop_parameter());
	WeakReference target = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::copy_operator(std::move(target), source));
}

MINT_FUNCTION(mint_int32_call, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::call_operator(std::move(value)));
}

MINT_FUNCTION(mint_int32_add, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::add_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int32_sub, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::sub_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int32_mul, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::mul_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int32_div, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::div_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int32_pow, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::pow_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int32_mod, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::mod_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int32_eq, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::eq_operator(value, other));
}

MINT_FUNCTION(mint_int32_ne, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::ne_operator(value, other));
}

MINT_FUNCTION(mint_int32_lt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::lt_operator(value, other));
}

MINT_FUNCTION(mint_int32_gt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::gt_operator(value, other));
}

MINT_FUNCTION(mint_int32_le, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::le_operator(value, other));
}

MINT_FUNCTION(mint_int32_ge, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::ge_operator(value, other));
}

MINT_FUNCTION(mint_int32_and, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::and_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_int32_or, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::or_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_int32_band, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::band_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int32_bor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::bor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int32_xor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::xor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int32_inc, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::inc_operator(std::move(value)));
}

MINT_FUNCTION(mint_int32_dec, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::dec_operator(std::move(value)));
}

MINT_FUNCTION(mint_int32_not, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::not_operator(value));
}

MINT_FUNCTION(mint_int32_compl, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::compl_operator(std::move(value)));
}

MINT_FUNCTION(mint_int32_pos, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::pos_operator(std::move(value)));
}

MINT_FUNCTION(mint_int32_neg, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::neg_operator(std::move(value)));
}

MINT_FUNCTION(mint_int32_shift_left, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::shift_left_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int32_shift_right, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::shift_right_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int32_inclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::inclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_int32_exclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::exclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_int32_subscript, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::subscript_operator(value, to_integer(cursor, index)));
}

MINT_FUNCTION(mint_int32_subscript_move, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::subscript_move_operator(std::move(value), to_integer(cursor, index), other));
}

MINT_FUNCTION(mint_int32_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int32_t>::to_number(value));
}

template<>
const char *fixed_int<int64_t>::name() {
	return symbols::int64.c_str();
}

MINT_FUNCTION(mint_int64_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(create_object(fixed_int<int64_t>::create(cursor, value)));
}

MINT_FUNCTION(mint_int64_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	delete value.data<LibObject<int64_t>>()->impl;
}

MINT_FUNCTION(mint_int64_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference source = std::move(helper.pop_parameter());
	WeakReference target = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::copy_operator(std::move(target), source));
}

MINT_FUNCTION(mint_int64_call, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::call_operator(std::move(value)));
}

MINT_FUNCTION(mint_int64_add, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::add_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int64_sub, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::sub_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int64_mul, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::mul_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int64_div, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::div_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int64_pow, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::pow_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int64_mod, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::mod_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int64_eq, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::eq_operator(value, other));
}

MINT_FUNCTION(mint_int64_ne, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::ne_operator(value, other));
}

MINT_FUNCTION(mint_int64_lt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::lt_operator(value, other));
}

MINT_FUNCTION(mint_int64_gt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::gt_operator(value, other));
}

MINT_FUNCTION(mint_int64_le, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::le_operator(value, other));
}

MINT_FUNCTION(mint_int64_ge, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::ge_operator(value, other));
}

MINT_FUNCTION(mint_int64_and, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::and_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_int64_or, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::or_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_int64_band, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::band_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int64_bor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::bor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int64_xor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::xor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int64_inc, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::inc_operator(std::move(value)));
}

MINT_FUNCTION(mint_int64_dec, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::dec_operator(std::move(value)));
}

MINT_FUNCTION(mint_int64_not, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::not_operator(value));
}

MINT_FUNCTION(mint_int64_compl, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::compl_operator(std::move(value)));
}

MINT_FUNCTION(mint_int64_pos, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::pos_operator(std::move(value)));
}

MINT_FUNCTION(mint_int64_neg, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::neg_operator(std::move(value)));
}

MINT_FUNCTION(mint_int64_shift_left, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::shift_left_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int64_shift_right, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::shift_right_operator(std::move(value), other));
}

MINT_FUNCTION(mint_int64_inclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::inclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_int64_exclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::exclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_int64_subscript, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::subscript_operator(value, to_integer(cursor, index)));
}

MINT_FUNCTION(mint_int64_subscript_move, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::subscript_move_operator(std::move(value), to_integer(cursor, index), other));
}

MINT_FUNCTION(mint_int64_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<int64_t>::to_number(value));
}

template<>
const char *fixed_int<uint8_t>::name() {
	return symbols::uint8.c_str();
}

MINT_FUNCTION(mint_uint8_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(create_object(fixed_int<uint8_t>::create(cursor, value)));
}

MINT_FUNCTION(mint_uint8_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	delete value.data<LibObject<uint8_t>>()->impl;
}

MINT_FUNCTION(mint_uint8_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference source = std::move(helper.pop_parameter());
	WeakReference target = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::copy_operator(std::move(target), source));
}

MINT_FUNCTION(mint_uint8_call, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::call_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint8_add, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::add_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint8_sub, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::sub_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint8_mul, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::mul_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint8_div, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::div_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint8_pow, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::pow_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint8_mod, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::mod_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint8_eq, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::eq_operator(value, other));
}

MINT_FUNCTION(mint_uint8_ne, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::ne_operator(value, other));
}

MINT_FUNCTION(mint_uint8_lt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::lt_operator(value, other));
}

MINT_FUNCTION(mint_uint8_gt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::gt_operator(value, other));
}

MINT_FUNCTION(mint_uint8_le, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::le_operator(value, other));
}

MINT_FUNCTION(mint_uint8_ge, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::ge_operator(value, other));
}

MINT_FUNCTION(mint_uint8_and, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::and_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_uint8_or, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::or_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_uint8_band, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::band_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint8_bor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::bor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint8_xor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::xor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint8_inc, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::inc_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint8_dec, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::dec_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint8_not, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::not_operator(value));
}

MINT_FUNCTION(mint_uint8_compl, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::compl_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint8_pos, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::pos_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint8_shift_left, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::shift_left_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint8_shift_right, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::shift_right_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint8_inclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::inclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_uint8_exclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::exclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_uint8_subscript, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::subscript_operator(value, to_integer(cursor, index)));
}

MINT_FUNCTION(mint_uint8_subscript_move, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::subscript_move_operator(std::move(value), to_integer(cursor, index), other));
}

MINT_FUNCTION(mint_uint8_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint8_t>::to_number(value));
}

template<>
const char *fixed_int<uint16_t>::name() {
	return symbols::uint16.c_str();
}

MINT_FUNCTION(mint_uint16_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(create_object(fixed_int<uint16_t>::create(cursor, value)));
}

MINT_FUNCTION(mint_uint16_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	delete value.data<LibObject<uint16_t>>()->impl;
}

MINT_FUNCTION(mint_uint16_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference source = std::move(helper.pop_parameter());
	WeakReference target = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::copy_operator(std::move(target), source));
}

MINT_FUNCTION(mint_uint16_call, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::call_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint16_add, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::add_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint16_sub, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::sub_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint16_mul, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::mul_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint16_div, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::div_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint16_pow, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::pow_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint16_mod, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::mod_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint16_eq, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::eq_operator(value, other));
}

MINT_FUNCTION(mint_uint16_ne, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::ne_operator(value, other));
}

MINT_FUNCTION(mint_uint16_lt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::lt_operator(value, other));
}

MINT_FUNCTION(mint_uint16_gt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::gt_operator(value, other));
}

MINT_FUNCTION(mint_uint16_le, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::le_operator(value, other));
}

MINT_FUNCTION(mint_uint16_ge, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::ge_operator(value, other));
}

MINT_FUNCTION(mint_uint16_and, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::and_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_uint16_or, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::or_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_uint16_band, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::band_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint16_bor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::bor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint16_xor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::xor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint16_inc, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::inc_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint16_dec, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::dec_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint16_not, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::not_operator(value));
}

MINT_FUNCTION(mint_uint16_compl, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::compl_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint16_pos, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::pos_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint16_shift_left, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::shift_left_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint16_shift_right, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::shift_right_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint16_inclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::inclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_uint16_exclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::exclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_uint16_subscript, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::subscript_operator(value, to_integer(cursor, index)));
}

MINT_FUNCTION(mint_uint16_subscript_move, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::subscript_move_operator(std::move(value), to_integer(cursor, index), other));
}

MINT_FUNCTION(mint_uint16_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint16_t>::to_number(value));
}

template<>
const char *fixed_int<uint32_t>::name() {
	return symbols::uint32.c_str();
}

MINT_FUNCTION(mint_uint32_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(create_object(fixed_int<uint32_t>::create(cursor, value)));
}

MINT_FUNCTION(mint_uint32_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	delete value.data<LibObject<uint32_t>>()->impl;
}

MINT_FUNCTION(mint_uint32_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference source = std::move(helper.pop_parameter());
	WeakReference target = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::copy_operator(std::move(target), source));
}

MINT_FUNCTION(mint_uint32_call, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::call_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint32_add, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::add_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint32_sub, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::sub_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint32_mul, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::mul_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint32_div, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::div_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint32_pow, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::pow_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint32_mod, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::mod_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint32_eq, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::eq_operator(value, other));
}

MINT_FUNCTION(mint_uint32_ne, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::ne_operator(value, other));
}

MINT_FUNCTION(mint_uint32_lt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::lt_operator(value, other));
}

MINT_FUNCTION(mint_uint32_gt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::gt_operator(value, other));
}

MINT_FUNCTION(mint_uint32_le, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::le_operator(value, other));
}

MINT_FUNCTION(mint_uint32_ge, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::ge_operator(value, other));
}

MINT_FUNCTION(mint_uint32_and, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::and_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_uint32_or, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::or_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_uint32_band, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::band_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint32_bor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::bor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint32_xor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::xor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint32_inc, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::inc_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint32_dec, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::dec_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint32_not, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::not_operator(value));
}

MINT_FUNCTION(mint_uint32_compl, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::compl_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint32_pos, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::pos_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint32_shift_left, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::shift_left_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint32_shift_right, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::shift_right_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint32_inclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::inclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_uint32_exclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::exclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_uint32_subscript, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::subscript_operator(value, to_integer(cursor, index)));
}

MINT_FUNCTION(mint_uint32_subscript_move, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::subscript_move_operator(std::move(value), to_integer(cursor, index), other));
}

MINT_FUNCTION(mint_uint32_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint32_t>::to_number(value));
}

template<>
const char *fixed_int<uint64_t>::name() {
	return symbols::uint64.c_str();
}

MINT_FUNCTION(mint_uint64_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(create_object(fixed_int<uint64_t>::create(cursor, value)));
}

MINT_FUNCTION(mint_uint64_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	delete value.data<LibObject<uint64_t>>()->impl;
}

MINT_FUNCTION(mint_uint64_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference source = std::move(helper.pop_parameter());
	WeakReference target = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::copy_operator(std::move(target), source));
}

MINT_FUNCTION(mint_uint64_call, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::call_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint64_add, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::add_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint64_sub, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::sub_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint64_mul, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::mul_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint64_div, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::div_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint64_pow, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::pow_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint64_mod, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::mod_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint64_eq, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::eq_operator(value, other));
}

MINT_FUNCTION(mint_uint64_ne, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::ne_operator(value, other));
}

MINT_FUNCTION(mint_uint64_lt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::lt_operator(value, other));
}

MINT_FUNCTION(mint_uint64_gt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::gt_operator(value, other));
}

MINT_FUNCTION(mint_uint64_le, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::le_operator(value, other));
}

MINT_FUNCTION(mint_uint64_ge, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::ge_operator(value, other));
}

MINT_FUNCTION(mint_uint64_and, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::and_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_uint64_or, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::or_operator(std::move(value), std::move(other)));
}

MINT_FUNCTION(mint_uint64_band, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::band_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint64_bor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::bor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint64_xor, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::xor_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint64_inc, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::inc_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint64_dec, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::dec_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint64_not, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::not_operator(value));
}

MINT_FUNCTION(mint_uint64_compl, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::compl_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint64_pos, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::pos_operator(std::move(value)));
}

MINT_FUNCTION(mint_uint64_shift_left, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::shift_left_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint64_shift_right, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::shift_right_operator(std::move(value), other));
}

MINT_FUNCTION(mint_uint64_inclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::inclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_uint64_exclusive_range, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::exclusive_range_operator(value, other));
}

MINT_FUNCTION(mint_uint64_subscript, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::subscript_operator(value, to_integer(cursor, index)));
}

MINT_FUNCTION(mint_uint64_subscript_move, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	WeakReference other = std::move(helper.pop_parameter());
	WeakReference index = std::move(helper.pop_parameter());
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::subscript_move_operator(std::move(value), to_integer(cursor, index), other));
}

MINT_FUNCTION(mint_uint64_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference value = std::move(helper.pop_parameter());
	helper.return_value(fixed_int<uint64_t>::to_number(value));
}
