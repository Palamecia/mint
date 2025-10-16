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

#include "mint/ast/classregister.h"
#include "mint/ast/symbol.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/casttool.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/reference.h"
#include "mint/system/error.h"
#include "mint/system/string.h"
#include <cinttypes>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace symbols {

static const mint::Symbol d_ptr("d_ptr");

static const std::string int8("int8");
static const std::string uint8("uint8");
static const std::string int16("int16");
static const std::string uint16("uint16");
static const std::string int32("int32");
static const std::string uint32("uint32");
static const std::string int64("int64");
static const std::string uint64("uint64");

}

namespace {

template<std::integral number_t>
number_t strtonum(const char* nptr, char** endptr, int base) {
	return std::is_signed_v<number_t> ? static_cast<number_t>(strtoimax(nptr, endptr, base))
	                                  : static_cast<number_t>(strtoumax(nptr, endptr, base));
}

template<std::integral number_t>
class FixedInt {
public:
	FixedInt() = delete;

	static number_t* create(mint::Cursor& cursor, mint::Reference& value) {

		switch (value.data().format()) {
		case mint::Data::none_format:
		case mint::Data::null_format:
			return new number_t(0);
		case mint::Data::number_format:
		case mint::Data::boolean_format:
			return new number_t(to_integer<number_t>(cursor, value));
		case mint::Data::object_format:
			switch (value.data<mint::Object>().metadata.metatype()) {
			case mint::Class::string:
				return new number_t(mint::to_integer<number_t>(to_string(value)));
			case mint::Class::object:
				if (mint::is_instance_of(value, symbols::int8)) {
					return new number_t(
					    static_cast<number_t>(*get_d_ptr(value).data<mint::LibObject<std::int8_t>>().ptr));
				}
				if (mint::is_instance_of(value, symbols::int16)) {
					return new number_t(
					    static_cast<number_t>(*get_d_ptr(value).data<mint::LibObject<std::int16_t>>().ptr));
				}
				if (mint::is_instance_of(value, symbols::int32)) {
					return new number_t(
					    static_cast<number_t>(*get_d_ptr(value).data<mint::LibObject<std::int32_t>>().ptr));
				}
				if (mint::is_instance_of(value, symbols::int64)) {
					return new number_t(
					    static_cast<number_t>(*get_d_ptr(value).data<mint::LibObject<std::int64_t>>().ptr));
				}
				if (mint::is_instance_of(value, symbols::uint8)) {
					return new number_t(
					    static_cast<number_t>(*get_d_ptr(value).data<mint::LibObject<std::uint8_t>>().ptr));
				}
				if (mint::is_instance_of(value, symbols::uint16)) {
					return new number_t(
					    static_cast<number_t>(*get_d_ptr(value).data<mint::LibObject<std::uint16_t>>().ptr));
				}
				if (mint::is_instance_of(value, symbols::uint32)) {
					return new number_t(
					    static_cast<number_t>(*get_d_ptr(value).data<mint::LibObject<std::uint32_t>>().ptr));
				}
				if (mint::is_instance_of(value, symbols::uint64)) {
					return new number_t(
					    static_cast<number_t>(*get_d_ptr(value).data<mint::LibObject<std::uint64_t>>().ptr));
				}
				mint::error("no valid conversion from {} to {}", type_name(value), name());
				break;
			default:
				mint::error("no valid conversion from {} to {}", type_name(value), name());
				break;
			}
			break;
		default:
			mint::error("no valid conversion from {} to {}", type_name(value), name());
		}

		return nullptr;
	}

	static mint::Reference&& copy_operator(mint::Reference&& value, const mint::Reference& other) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		data = *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr;
		return std::move(value);
	}

	static mint::WeakReference call_operator(mint::AbstractSyntaxTree& ast, const mint::Reference& value) {
		return mint::create_c_object(ast, new number_t(*get_d_ptr(value).data<mint::LibObject<number_t>>().ptr));
	}

	static mint::Reference&& add_operator(mint::Reference&& value, const mint::Reference& other) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		data += *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr;
		return std::move(value);
	}

	static mint::Reference&& sub_operator(mint::Reference&& value, const mint::Reference& other) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		data -= *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr;
		return std::move(value);
	}

	static mint::Reference&& mul_operator(mint::Reference&& value, const mint::Reference& other) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		data *= *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr;
		return std::move(value);
	}

	static mint::Reference&& div_operator(mint::Reference&& value, const mint::Reference& other) {
		if (number_t divider = *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr) {
			number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
			data /= divider;
		}
		else {
			mint::error("division by zero");
		}
		return std::move(value);
	}

	static mint::Reference&& pow_operator(mint::Reference&& value, const mint::Reference& other) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		data = static_cast<number_t>(pow(data, *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr));
		return std::move(value);
	}

	static mint::Reference&& mod_operator(mint::Reference&& value, const mint::Reference& other) {
		if (number_t divider = *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr) {
			number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
			data %= divider;
		}
		else {
			mint::error("modulo by zero");
		}
		return std::move(value);
	}

	static mint::WeakReference eq_operator(mint::Reference& value, const mint::Reference& other) {
		const number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		return mint::create_boolean(data == *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr);
	}

	static mint::WeakReference ne_operator(mint::Reference& value, const mint::Reference& other) {
		const number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		return mint::create_boolean(data != *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr);
	}

	static mint::WeakReference lt_operator(mint::Reference& value, const mint::Reference& other) {
		const number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		return mint::create_boolean(data < *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr);
	}

	static mint::WeakReference gt_operator(mint::Reference& value, const mint::Reference& other) {
		const number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		return mint::create_boolean(data > *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr);
	}

	static mint::WeakReference le_operator(mint::Reference& value, const mint::Reference& other) {
		const number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		return mint::create_boolean(data <= *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr);
	}

	static mint::WeakReference ge_operator(mint::Reference& value, const mint::Reference& other) {
		const number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		return mint::create_boolean(data >= *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr);
	}

	static mint::Reference&& and_operator(mint::Reference&& value, mint::Reference&& other) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		return data ? std::move(other) : std::move(value);
	}

	static mint::Reference&& or_operator(mint::Reference&& value, mint::Reference&& other) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		return data ? std::move(value) : std::move(other);
	}

	static mint::Reference&& band_operator(mint::Reference&& value, const mint::Reference& other) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		data &= *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr;
		return std::move(value);
	}

	static mint::Reference&& bor_operator(mint::Reference&& value, const mint::Reference& other) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		data |= *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr;
		return std::move(value);
	}

	static mint::Reference&& xor_operator(mint::Reference&& value, const mint::Reference& other) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		data ^= *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr;
		return std::move(value);
	}

	static mint::Reference&& inc_operator(mint::Reference&& value) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		++data;
		return std::move(value);
	}

	static mint::Reference&& dec_operator(mint::Reference&& value) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		--data;
		return std::move(value);
	}

	static mint::WeakReference not_operator(mint::Reference& value) {
		const number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		return mint::create_boolean(!data);
	}

	static mint::Reference&& compl_operator(mint::Reference&& value) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		data = ~data;
		return std::move(value);
	}

	static mint::Reference&& pos_operator(mint::Reference&& value) {
		return std::move(value);
	}

	static mint::Reference&& neg_operator(mint::Reference&& value) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		data = -data;
		return std::move(value);
	}

	static mint::Reference&& shift_left_operator(mint::Reference&& value, const mint::Reference& other) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		data = data << *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr;
		return std::move(value);
	}

	static mint::Reference&& shift_right_operator(mint::Reference&& value, const mint::Reference& other) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		data = data >> *get_d_ptr(other).data<mint::LibObject<number_t>>().ptr;
		return std::move(value);
	}

	static mint::WeakReference inclusive_range_operator(mint::AbstractSyntaxTree& ast, mint::Reference& value,
	    const mint::Reference& other) {
		return mint::create_iterator(mint::from_inclusive_range, ast,
		    mint::to_number(*get_d_ptr(value).data<mint::LibObject<number_t>>().ptr),
		    mint::to_number(*get_d_ptr(other).data<mint::LibObject<number_t>>().ptr));
	}

	static mint::WeakReference exclusive_range_operator(mint::AbstractSyntaxTree& ast, mint::Reference& value,
	    const mint::Reference& other) {
		return mint::create_iterator(mint::from_exclusive_range, ast,
		    mint::to_number(*get_d_ptr(value).data<mint::LibObject<number_t>>().ptr),
		    mint::to_number(*get_d_ptr(other).data<mint::LibObject<number_t>>().ptr));
	}

	static mint::WeakReference subscript_operator(mint::AbstractSyntaxTree& ast, mint::Reference& value,
	    std::intmax_t index) {
		const number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		return mint::create_c_object(ast,
		    new number_t(
		        number_t(data / static_cast<number_t>(std::pow(static_cast<std::intmax_t>(mint::decimal_base), index))
		                 % mint::decimal_base)));
	}

	static mint::Reference&& subscript_move_operator(mint::Reference&& value, std::intmax_t index,
	    const mint::Reference& other) {
		number_t& data = *get_d_ptr(value).data<mint::LibObject<number_t>>().ptr;
		data -= static_cast<number_t>(
		            number_t(
		                data / static_cast<number_t>(std::pow(static_cast<std::intmax_t>(mint::decimal_base), index)))
		            % mint::decimal_base)
		        * static_cast<number_t>(std::pow(static_cast<std::intmax_t>(mint::decimal_base), index));
		data += number_t(*get_d_ptr(other).data<mint::LibObject<number_t>>().ptr
		                 * static_cast<number_t>(std::pow(static_cast<std::intmax_t>(mint::decimal_base), index)));
		return std::move(value);
	}

	static mint::WeakReference to_number(mint::Reference& value) {
		return mint::create_number(*get_d_ptr(value).data<mint::LibObject<number_t>>().ptr);
	}

private:
	static std::string_view name();

	static mint::WeakReference get_d_ptr(const mint::Reference& reference) {
		auto& object = reference.data<mint::Object>();
		if (auto* member = object.metadata.find_member(symbols::d_ptr)) {
			return mint::Class::MemberInfo::get(*member, object);
		}
		return {};
	}
};

template<>
std::string_view FixedInt<std::int8_t>::name() {
	return symbols::int8;
}

mint::WeakReference mint_int8_create(mint::Cursor& cursor, mint::Reference& value) {
	return mint::create_c_object(cursor.ast(), FixedInt<std::int8_t>::create(cursor, value));
}

mint::WeakReference mint_int8_delete(mint::Cursor& /*cursor*/, mint::Reference& value) {
	delete value.data<mint::LibObject<std::int8_t>>().ptr;
	return {};
}

mint::WeakReference mint_int8_copy(mint::Cursor& /*cursor*/, mint::Reference& target, const mint::Reference& source) {
	return FixedInt<std::int8_t>::copy_operator(std::move(target), source);
}

mint::WeakReference mint_int8_call(mint::Cursor& cursor, const mint::Reference& value) {
	return FixedInt<std::int8_t>::call_operator(cursor.ast(), value);
}

mint::WeakReference mint_int8_add(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int8_t>::add_operator(std::move(value), other);
}

mint::WeakReference mint_int8_sub(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int8_t>::sub_operator(std::move(value), other);
}

mint::WeakReference mint_int8_mul(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int8_t>::mul_operator(std::move(value), other);
}

mint::WeakReference mint_int8_div(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int8_t>::div_operator(std::move(value), other);
}

mint::WeakReference mint_int8_pow(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int8_t>::pow_operator(std::move(value), other);
}

mint::WeakReference mint_int8_mod(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int8_t>::mod_operator(std::move(value), other);
}

mint::WeakReference mint_int8_eq(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int8_t>::eq_operator(value, other);
}

mint::WeakReference mint_int8_ne(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int8_t>::ne_operator(value, other);
}

mint::WeakReference mint_int8_lt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int8_t>::lt_operator(value, other);
}

mint::WeakReference mint_int8_gt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int8_t>::gt_operator(value, other);
}

mint::WeakReference mint_int8_le(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int8_t>::le_operator(value, other);
}

mint::WeakReference mint_int8_ge(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int8_t>::ge_operator(value, other);
}

mint::WeakReference mint_int8_and(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::int8_t>::and_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_int8_or(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::int8_t>::or_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_int8_band(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int8_t>::band_operator(std::move(value), other);
}

mint::WeakReference mint_int8_bor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int8_t>::bor_operator(std::move(value), other);
}

mint::WeakReference mint_int8_xor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int8_t>::xor_operator(std::move(value), other);
}

mint::WeakReference mint_int8_inc(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int8_t>::inc_operator(std::move(value));
}

mint::WeakReference mint_int8_dec(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int8_t>::dec_operator(std::move(value));
}

mint::WeakReference mint_int8_not(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int8_t>::not_operator(value);
}

mint::WeakReference mint_int8_compl(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int8_t>::compl_operator(std::move(value));
}

mint::WeakReference mint_int8_pos(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int8_t>::pos_operator(std::move(value));
}

mint::WeakReference mint_int8_neg(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int8_t>::neg_operator(std::move(value));
}

mint::WeakReference mint_int8_shift_left(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int8_t>::shift_left_operator(std::move(value), other);
}

mint::WeakReference mint_int8_shift_right(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int8_t>::shift_right_operator(std::move(value), other);
}

mint::WeakReference mint_int8_inclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int8_t>::inclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_int8_exclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int8_t>::exclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_int8_subscript(mint::Cursor& cursor, mint::Reference& value, const mint::Reference& index) {
	return FixedInt<std::int8_t>::subscript_operator(cursor.ast(), value, mint::to_signed_integer(cursor, index));
}

mint::WeakReference mint_int8_subscript_move(mint::Cursor& cursor, mint::Reference& value, const mint::Reference& index,
    mint::Reference& other) {
	return FixedInt<std::int8_t>::subscript_move_operator(std::move(value), mint::to_signed_integer(cursor, index),
	    other);
}

mint::WeakReference mint_int8_to_number(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int8_t>::to_number(value);
}

template<>
std::string_view FixedInt<std::int16_t>::name() {
	return symbols::int16;
}

mint::WeakReference mint_int16_create(mint::Cursor& cursor, mint::Reference& value) {
	return mint::create_c_object(cursor.ast(), FixedInt<std::int16_t>::create(cursor, value));
}

mint::WeakReference mint_int16_delete(mint::Cursor& /*cursor*/, mint::Reference& value) {
	delete value.data<mint::LibObject<std::int16_t>>().ptr;
	return {};
}

mint::WeakReference mint_int16_copy(mint::Cursor& /*cursor*/, mint::Reference& target, const mint::Reference& source) {
	return FixedInt<std::int16_t>::copy_operator(std::move(target), source);
}

mint::WeakReference mint_int16_call(mint::Cursor& cursor, const mint::Reference& value) {
	return FixedInt<std::int16_t>::call_operator(cursor.ast(), value);
}

mint::WeakReference mint_int16_add(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int16_t>::add_operator(std::move(value), other);
}

mint::WeakReference mint_int16_sub(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int16_t>::sub_operator(std::move(value), other);
}

mint::WeakReference mint_int16_mul(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int16_t>::mul_operator(std::move(value), other);
}

mint::WeakReference mint_int16_div(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int16_t>::div_operator(std::move(value), other);
}

mint::WeakReference mint_int16_pow(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int16_t>::pow_operator(std::move(value), other);
}

mint::WeakReference mint_int16_mod(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int16_t>::mod_operator(std::move(value), other);
}

mint::WeakReference mint_int16_eq(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int16_t>::eq_operator(value, other);
}

mint::WeakReference mint_int16_ne(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int16_t>::ne_operator(value, other);
}

mint::WeakReference mint_int16_lt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int16_t>::lt_operator(value, other);
}

mint::WeakReference mint_int16_gt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int16_t>::gt_operator(value, other);
}

mint::WeakReference mint_int16_le(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int16_t>::le_operator(value, other);
}

mint::WeakReference mint_int16_ge(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int16_t>::ge_operator(value, other);
}

mint::WeakReference mint_int16_and(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::int16_t>::and_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_int16_or(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::int16_t>::or_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_int16_band(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int16_t>::band_operator(std::move(value), other);
}

mint::WeakReference mint_int16_bor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int16_t>::bor_operator(std::move(value), other);
}

mint::WeakReference mint_int16_xor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int16_t>::xor_operator(std::move(value), other);
}

mint::WeakReference mint_int16_inc(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int16_t>::inc_operator(std::move(value));
}

mint::WeakReference mint_int16_dec(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int16_t>::dec_operator(std::move(value));
}

mint::WeakReference mint_int16_not(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int16_t>::not_operator(value);
}

mint::WeakReference mint_int16_compl(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int16_t>::compl_operator(std::move(value));
}

mint::WeakReference mint_int16_pos(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int16_t>::pos_operator(std::move(value));
}

mint::WeakReference mint_int16_neg(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int16_t>::neg_operator(std::move(value));
}

mint::WeakReference mint_int16_shift_left(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int16_t>::shift_left_operator(std::move(value), other);
}

mint::WeakReference mint_int16_shift_right(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int16_t>::shift_right_operator(std::move(value), other);
}

mint::WeakReference mint_int16_inclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int16_t>::inclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_int16_exclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int16_t>::exclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_int16_subscript(mint::Cursor& cursor, mint::Reference& value, const mint::Reference& index) {
	return FixedInt<std::int16_t>::subscript_operator(cursor.ast(), value, mint::to_signed_integer(cursor, index));
}

mint::WeakReference mint_int16_subscript_move(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& index, mint::Reference& other) {
	return FixedInt<std::int16_t>::subscript_move_operator(std::move(value), mint::to_signed_integer(cursor, index),
	    other);
}

mint::WeakReference mint_int16_to_number(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int16_t>::to_number(value);
}

template<>
std::string_view FixedInt<std::int32_t>::name() {
	return symbols::int32;
}

mint::WeakReference mint_int32_create(mint::Cursor& cursor, mint::Reference& value) {
	return mint::create_c_object(cursor.ast(), FixedInt<std::int32_t>::create(cursor, value));
}

mint::WeakReference mint_int32_delete(mint::Cursor& /*cursor*/, mint::Reference& value) {
	delete value.data<mint::LibObject<std::int32_t>>().ptr;
	return {};
}

mint::WeakReference mint_int32_copy(mint::Cursor& /*cursor*/, mint::Reference& target, const mint::Reference& source) {
	return FixedInt<std::int32_t>::copy_operator(std::move(target), source);
}

mint::WeakReference mint_int32_call(mint::Cursor& cursor, const mint::Reference& value) {
	return FixedInt<std::int32_t>::call_operator(cursor.ast(), value);
}

mint::WeakReference mint_int32_add(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int32_t>::add_operator(std::move(value), other);
}

mint::WeakReference mint_int32_sub(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int32_t>::sub_operator(std::move(value), other);
}

mint::WeakReference mint_int32_mul(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int32_t>::mul_operator(std::move(value), other);
}

mint::WeakReference mint_int32_div(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int32_t>::div_operator(std::move(value), other);
}

mint::WeakReference mint_int32_pow(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int32_t>::pow_operator(std::move(value), other);
}

mint::WeakReference mint_int32_mod(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int32_t>::mod_operator(std::move(value), other);
}

mint::WeakReference mint_int32_eq(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int32_t>::eq_operator(value, other);
}

mint::WeakReference mint_int32_ne(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int32_t>::ne_operator(value, other);
}

mint::WeakReference mint_int32_lt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int32_t>::lt_operator(value, other);
}

mint::WeakReference mint_int32_gt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int32_t>::gt_operator(value, other);
}

mint::WeakReference mint_int32_le(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int32_t>::le_operator(value, other);
}

mint::WeakReference mint_int32_ge(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int32_t>::ge_operator(value, other);
}

mint::WeakReference mint_int32_and(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::int32_t>::and_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_int32_or(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::int32_t>::or_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_int32_band(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int32_t>::band_operator(std::move(value), other);
}

mint::WeakReference mint_int32_bor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int32_t>::bor_operator(std::move(value), other);
}

mint::WeakReference mint_int32_xor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int32_t>::xor_operator(std::move(value), other);
}

mint::WeakReference mint_int32_inc(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int32_t>::inc_operator(std::move(value));
}

mint::WeakReference mint_int32_dec(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int32_t>::dec_operator(std::move(value));
}

mint::WeakReference mint_int32_not(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int32_t>::not_operator(value);
}

mint::WeakReference mint_int32_compl(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int32_t>::compl_operator(std::move(value));
}

mint::WeakReference mint_int32_pos(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int32_t>::pos_operator(std::move(value));
}

mint::WeakReference mint_int32_neg(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int32_t>::neg_operator(std::move(value));
}

mint::WeakReference mint_int32_shift_left(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int32_t>::shift_left_operator(std::move(value), other);
}

mint::WeakReference mint_int32_shift_right(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int32_t>::shift_right_operator(std::move(value), other);
}

mint::WeakReference mint_int32_inclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int32_t>::inclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_int32_exclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int32_t>::exclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_int32_subscript(mint::Cursor& cursor, mint::Reference& value, const mint::Reference& index) {
	return FixedInt<std::int32_t>::subscript_operator(cursor.ast(), value, mint::to_signed_integer(cursor, index));
}

mint::WeakReference mint_int32_subscript_move(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& index, mint::Reference& other) {
	return FixedInt<std::int32_t>::subscript_move_operator(std::move(value), mint::to_signed_integer(cursor, index),
	    other);
}

mint::WeakReference mint_int32_to_number(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int32_t>::to_number(value);
}

template<>
std::string_view FixedInt<std::int64_t>::name() {
	return symbols::int64;
}

mint::WeakReference mint_int64_create(mint::Cursor& cursor, mint::Reference& value) {
	return mint::create_c_object(cursor.ast(), FixedInt<std::int64_t>::create(cursor, value));
}

mint::WeakReference mint_int64_delete(mint::Cursor& /*cursor*/, mint::Reference& value) {
	delete value.data<mint::LibObject<std::int64_t>>().ptr;
	return {};
}

mint::WeakReference mint_int64_copy(mint::Cursor& /*cursor*/, mint::Reference& target, const mint::Reference& source) {
	return FixedInt<std::int64_t>::copy_operator(std::move(target), source);
}

mint::WeakReference mint_int64_call(mint::Cursor& cursor, const mint::Reference& value) {
	return FixedInt<std::int64_t>::call_operator(cursor.ast(), value);
}

mint::WeakReference mint_int64_add(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int64_t>::add_operator(std::move(value), other);
}

mint::WeakReference mint_int64_sub(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int64_t>::sub_operator(std::move(value), other);
}

mint::WeakReference mint_int64_mul(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int64_t>::mul_operator(std::move(value), other);
}

mint::WeakReference mint_int64_div(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int64_t>::div_operator(std::move(value), other);
}

mint::WeakReference mint_int64_pow(mint::Cursor&, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int64_t>::pow_operator(std::move(value), other);
}

mint::WeakReference mint_int64_mod(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int64_t>::mod_operator(std::move(value), other);
}

mint::WeakReference mint_int64_eq(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int64_t>::eq_operator(value, other);
}

mint::WeakReference mint_int64_ne(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int64_t>::ne_operator(value, other);
}

mint::WeakReference mint_int64_lt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int64_t>::lt_operator(value, other);
}

mint::WeakReference mint_int64_gt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int64_t>::gt_operator(value, other);
}

mint::WeakReference mint_int64_le(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int64_t>::le_operator(value, other);
}

mint::WeakReference mint_int64_ge(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int64_t>::ge_operator(value, other);
}

mint::WeakReference mint_int64_and(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::int64_t>::and_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_int64_or(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::int64_t>::or_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_int64_band(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int64_t>::band_operator(std::move(value), other);
}

mint::WeakReference mint_int64_bor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int64_t>::bor_operator(std::move(value), other);
}

mint::WeakReference mint_int64_xor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::int64_t>::xor_operator(std::move(value), other);
}

mint::WeakReference mint_int64_inc(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int64_t>::inc_operator(std::move(value));
}

mint::WeakReference mint_int64_dec(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int64_t>::dec_operator(std::move(value));
}

mint::WeakReference mint_int64_not(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int64_t>::not_operator(value);
}

mint::WeakReference mint_int64_compl(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int64_t>::compl_operator(std::move(value));
}

mint::WeakReference mint_int64_pos(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int64_t>::pos_operator(std::move(value));
}

mint::WeakReference mint_int64_neg(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int64_t>::neg_operator(std::move(value));
}

mint::WeakReference mint_int64_shift_left(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int64_t>::shift_left_operator(std::move(value), other);
}

mint::WeakReference mint_int64_shift_right(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int64_t>::shift_right_operator(std::move(value), other);
}

mint::WeakReference mint_int64_inclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int64_t>::inclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_int64_exclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::int64_t>::exclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_int64_subscript(mint::Cursor& cursor, mint::Reference& value, const mint::Reference& index) {
	return FixedInt<std::int64_t>::subscript_operator(cursor.ast(), value, mint::to_signed_integer(cursor, index));
}

mint::WeakReference mint_int64_subscript_move(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& index, mint::Reference& other) {
	return FixedInt<std::int64_t>::subscript_move_operator(std::move(value), mint::to_signed_integer(cursor, index),
	    other);
}

mint::WeakReference mint_int64_to_number(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::int64_t>::to_number(value);
}

template<>
std::string_view FixedInt<std::uint8_t>::name() {
	return symbols::uint8;
}

mint::WeakReference mint_uint8_create(mint::Cursor& cursor, mint::Reference& value) {
	return mint::create_c_object(cursor.ast(), FixedInt<std::uint8_t>::create(cursor, value));
}

mint::WeakReference mint_uint8_delete(mint::Cursor& /*cursor*/, mint::Reference& value) {
	delete value.data<mint::LibObject<std::uint8_t>>().ptr;
	return {};
}

mint::WeakReference mint_uint8_copy(mint::Cursor& /*cursor*/, mint::Reference& target, const mint::Reference& source) {
	return FixedInt<std::uint8_t>::copy_operator(std::move(target), source);
}

mint::WeakReference mint_uint8_call(mint::Cursor& cursor, const mint::Reference& value) {
	return FixedInt<std::uint8_t>::call_operator(cursor.ast(), value);
}

mint::WeakReference mint_uint8_add(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint8_t>::add_operator(std::move(value), other);
}

mint::WeakReference mint_uint8_sub(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint8_t>::sub_operator(std::move(value), other);
}

mint::WeakReference mint_uint8_mul(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint8_t>::mul_operator(std::move(value), other);
}

mint::WeakReference mint_uint8_div(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint8_t>::div_operator(std::move(value), other);
}

mint::WeakReference mint_uint8_pow(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint8_t>::pow_operator(std::move(value), other);
}

mint::WeakReference mint_uint8_mod(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint8_t>::mod_operator(std::move(value), other);
}

mint::WeakReference mint_uint8_eq(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint8_t>::eq_operator(value, other);
}

mint::WeakReference mint_uint8_ne(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint8_t>::ne_operator(value, other);
}

mint::WeakReference mint_uint8_lt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint8_t>::lt_operator(value, other);
}

mint::WeakReference mint_uint8_gt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint8_t>::gt_operator(value, other);
}

mint::WeakReference mint_uint8_le(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint8_t>::le_operator(value, other);
}

mint::WeakReference mint_uint8_ge(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint8_t>::ge_operator(value, other);
}

mint::WeakReference mint_uint8_and(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::uint8_t>::and_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_uint8_or(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::uint8_t>::or_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_uint8_band(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint8_t>::band_operator(std::move(value), other);
}

mint::WeakReference mint_uint8_bor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint8_t>::bor_operator(std::move(value), other);
}

mint::WeakReference mint_uint8_xor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint8_t>::xor_operator(std::move(value), other);
}

mint::WeakReference mint_uint8_inc(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint8_t>::inc_operator(std::move(value));
}

mint::WeakReference mint_uint8_dec(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint8_t>::dec_operator(std::move(value));
}

mint::WeakReference mint_uint8_not(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint8_t>::not_operator(value);
}

mint::WeakReference mint_uint8_compl(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint8_t>::compl_operator(std::move(value));
}

mint::WeakReference mint_uint8_pos(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint8_t>::pos_operator(std::move(value));
}

mint::WeakReference mint_uint8_shift_left(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint8_t>::shift_left_operator(std::move(value), other);
}

mint::WeakReference mint_uint8_shift_right(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint8_t>::shift_right_operator(std::move(value), other);
}

mint::WeakReference mint_uint8_inclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint8_t>::inclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_uint8_exclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint8_t>::exclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_uint8_subscript(mint::Cursor& cursor, mint::Reference& value, const mint::Reference& index) {
	return FixedInt<std::uint8_t>::subscript_operator(cursor.ast(), value, mint::to_signed_integer(cursor, index));
}

mint::WeakReference mint_uint8_subscript_move(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& index, mint::Reference& other) {
	return FixedInt<std::uint8_t>::subscript_move_operator(std::move(value), mint::to_signed_integer(cursor, index),
	    other);
}

mint::WeakReference mint_uint8_to_number(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint8_t>::to_number(value);
}

template<>
std::string_view FixedInt<std::uint16_t>::name() {
	return symbols::uint16;
}

mint::WeakReference mint_uint16_create(mint::Cursor& cursor, mint::Reference& value) {
	return mint::create_c_object(cursor.ast(), FixedInt<std::uint16_t>::create(cursor, value));
}

mint::WeakReference mint_uint16_delete(mint::Cursor& /*cursor*/, mint::Reference& value) {
	delete value.data<mint::LibObject<std::uint16_t>>().ptr;
	return {};
}

mint::WeakReference mint_uint16_copy(mint::Cursor& /*cursor*/, mint::Reference& target, const mint::Reference& source) {
	return FixedInt<std::uint16_t>::copy_operator(std::move(target), source);
}

mint::WeakReference mint_uint16_call(mint::Cursor& cursor, const mint::Reference& value) {
	return FixedInt<std::uint16_t>::call_operator(cursor.ast(), value);
}

mint::WeakReference mint_uint16_add(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint16_t>::add_operator(std::move(value), other);
}

mint::WeakReference mint_uint16_sub(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint16_t>::sub_operator(std::move(value), other);
}

mint::WeakReference mint_uint16_mul(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint16_t>::mul_operator(std::move(value), other);
}

mint::WeakReference mint_uint16_div(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint16_t>::div_operator(std::move(value), other);
}

mint::WeakReference mint_uint16_pow(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint16_t>::pow_operator(std::move(value), other);
}

mint::WeakReference mint_uint16_mod(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint16_t>::mod_operator(std::move(value), other);
}

mint::WeakReference mint_uint16_eq(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint16_t>::eq_operator(value, other);
}

mint::WeakReference mint_uint16_ne(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint16_t>::ne_operator(value, other);
}

mint::WeakReference mint_uint16_lt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint16_t>::lt_operator(value, other);
}

mint::WeakReference mint_uint16_gt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint16_t>::gt_operator(value, other);
}

mint::WeakReference mint_uint16_le(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint16_t>::le_operator(value, other);
}

mint::WeakReference mint_uint16_ge(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint16_t>::ge_operator(value, other);
}

mint::WeakReference mint_uint16_and(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::uint16_t>::and_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_uint16_or(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::uint16_t>::or_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_uint16_band(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint16_t>::band_operator(std::move(value), other);
}

mint::WeakReference mint_uint16_bor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint16_t>::bor_operator(std::move(value), other);
}

mint::WeakReference mint_uint16_xor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint16_t>::xor_operator(std::move(value), other);
}

mint::WeakReference mint_uint16_inc(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint16_t>::inc_operator(std::move(value));
}

mint::WeakReference mint_uint16_dec(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint16_t>::dec_operator(std::move(value));
}

mint::WeakReference mint_uint16_not(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint16_t>::not_operator(value);
}

mint::WeakReference mint_uint16_compl(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint16_t>::compl_operator(std::move(value));
}

mint::WeakReference mint_uint16_pos(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint16_t>::pos_operator(std::move(value));
}

mint::WeakReference mint_uint16_shift_left(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint16_t>::shift_left_operator(std::move(value), other);
}

mint::WeakReference mint_uint16_shift_right(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint16_t>::shift_right_operator(std::move(value), other);
}

mint::WeakReference mint_uint16_inclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint16_t>::inclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_uint16_exclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint16_t>::exclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_uint16_subscript(mint::Cursor& cursor, mint::Reference& value, const mint::Reference& index) {
	return FixedInt<std::uint16_t>::subscript_operator(cursor.ast(), value, mint::to_signed_integer(cursor, index));
}

mint::WeakReference mint_uint16_subscript_move(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& index, mint::Reference& other) {
	return FixedInt<std::uint16_t>::subscript_move_operator(std::move(value), mint::to_signed_integer(cursor, index),
	    other);
}

mint::WeakReference mint_uint16_to_number(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint16_t>::to_number(value);
}

template<>
std::string_view FixedInt<std::uint32_t>::name() {
	return symbols::uint32;
}

mint::WeakReference mint_uint32_create(mint::Cursor& cursor, mint::Reference& value) {
	return mint::create_c_object(cursor.ast(), FixedInt<std::uint32_t>::create(cursor, value));
}

mint::WeakReference mint_uint32_delete(mint::Cursor& /*cursor*/, mint::Reference& value) {
	delete value.data<mint::LibObject<std::uint32_t>>().ptr;
	return {};
}

mint::WeakReference mint_uint32_copy(mint::Cursor& /*cursor*/, mint::Reference& target, const mint::Reference& source) {
	return FixedInt<std::uint32_t>::copy_operator(std::move(target), source);
}

mint::WeakReference mint_uint32_call(mint::Cursor& cursor, const mint::Reference& value) {
	return FixedInt<std::uint32_t>::call_operator(cursor.ast(), value);
}

mint::WeakReference mint_uint32_add(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint32_t>::add_operator(std::move(value), other);
}

mint::WeakReference mint_uint32_sub(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint32_t>::sub_operator(std::move(value), other);
}

mint::WeakReference mint_uint32_mul(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint32_t>::mul_operator(std::move(value), other);
}

mint::WeakReference mint_uint32_div(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint32_t>::div_operator(std::move(value), other);
}

mint::WeakReference mint_uint32_pow(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint32_t>::pow_operator(std::move(value), other);
}

mint::WeakReference mint_uint32_mod(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint32_t>::mod_operator(std::move(value), other);
}

mint::WeakReference mint_uint32_eq(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint32_t>::eq_operator(value, other);
}

mint::WeakReference mint_uint32_ne(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint32_t>::ne_operator(value, other);
}

mint::WeakReference mint_uint32_lt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint32_t>::lt_operator(value, other);
}

mint::WeakReference mint_uint32_gt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint32_t>::gt_operator(value, other);
}

mint::WeakReference mint_uint32_le(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint32_t>::le_operator(value, other);
}

mint::WeakReference mint_uint32_ge(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint32_t>::ge_operator(value, other);
}

mint::WeakReference mint_uint32_and(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::uint32_t>::and_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_uint32_or(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::uint32_t>::or_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_uint32_band(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint32_t>::band_operator(std::move(value), other);
}

mint::WeakReference mint_uint32_bor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint32_t>::bor_operator(std::move(value), other);
}

mint::WeakReference mint_uint32_xor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint32_t>::xor_operator(std::move(value), other);
}

mint::WeakReference mint_uint32_inc(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint32_t>::inc_operator(std::move(value));
}

mint::WeakReference mint_uint32_dec(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint32_t>::dec_operator(std::move(value));
}

mint::WeakReference mint_uint32_not(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint32_t>::not_operator(value);
}

mint::WeakReference mint_uint32_compl(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint32_t>::compl_operator(std::move(value));
}

mint::WeakReference mint_uint32_pos(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint32_t>::pos_operator(std::move(value));
}

mint::WeakReference mint_uint32_shift_left(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint32_t>::shift_left_operator(std::move(value), other);
}

mint::WeakReference mint_uint32_shift_right(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint32_t>::shift_right_operator(std::move(value), other);
}

mint::WeakReference mint_uint32_inclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint32_t>::inclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_uint32_exclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint32_t>::exclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_uint32_subscript(mint::Cursor& cursor, mint::Reference& value, const mint::Reference& index) {
	return FixedInt<std::uint32_t>::subscript_operator(cursor.ast(), value, mint::to_signed_integer(cursor, index));
}

mint::WeakReference mint_uint32_subscript_move(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& index, mint::Reference& other) {
	return FixedInt<std::uint32_t>::subscript_move_operator(std::move(value), mint::to_signed_integer(cursor, index),
	    other);
}

mint::WeakReference mint_uint32_to_number(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint32_t>::to_number(value);
}

template<>
std::string_view FixedInt<std::uint64_t>::name() {
	return symbols::uint64;
}

mint::WeakReference mint_uint64_create(mint::Cursor& cursor, mint::Reference& value) {
	return mint::create_c_object(cursor.ast(), FixedInt<std::uint64_t>::create(cursor, value));
}

mint::WeakReference mint_uint64_delete(mint::Cursor& /*cursor*/, mint::Reference& value) {
	delete value.data<mint::LibObject<std::uint64_t>>().ptr;
	return {};
}

mint::WeakReference mint_uint64_copy(mint::Cursor& /*cursor*/, mint::Reference& target, const mint::Reference& source) {
	return FixedInt<std::uint64_t>::copy_operator(std::move(target), source);
}

mint::WeakReference mint_uint64_call(mint::Cursor& cursor, const mint::Reference& value) {
	return FixedInt<std::uint64_t>::call_operator(cursor.ast(), value);
}

mint::WeakReference mint_uint64_add(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint64_t>::add_operator(std::move(value), other);
}

mint::WeakReference mint_uint64_sub(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint64_t>::sub_operator(std::move(value), other);
}

mint::WeakReference mint_uint64_mul(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint64_t>::mul_operator(std::move(value), other);
}

mint::WeakReference mint_uint64_div(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint64_t>::div_operator(std::move(value), other);
}

mint::WeakReference mint_uint64_pow(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint64_t>::pow_operator(std::move(value), other);
}

mint::WeakReference mint_uint64_mod(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint64_t>::mod_operator(std::move(value), other);
}

mint::WeakReference mint_uint64_eq(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint64_t>::eq_operator(value, other);
}

mint::WeakReference mint_uint64_ne(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint64_t>::ne_operator(value, other);
}

mint::WeakReference mint_uint64_lt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint64_t>::lt_operator(value, other);
}

mint::WeakReference mint_uint64_gt(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint64_t>::gt_operator(value, other);
}

mint::WeakReference mint_uint64_le(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint64_t>::le_operator(value, other);
}

mint::WeakReference mint_uint64_ge(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint64_t>::ge_operator(value, other);
}

mint::WeakReference mint_uint64_and(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::uint64_t>::and_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_uint64_or(mint::Cursor& /*cursor*/, mint::Reference& value, mint::Reference& other) {
	return FixedInt<std::uint64_t>::or_operator(std::move(value), std::move(other));
}

mint::WeakReference mint_uint64_band(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint64_t>::band_operator(std::move(value), other);
}

mint::WeakReference mint_uint64_bor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint64_t>::bor_operator(std::move(value), other);
}

mint::WeakReference mint_uint64_xor(mint::Cursor& /*cursor*/, mint::Reference& value, const mint::Reference& other) {
	return FixedInt<std::uint64_t>::xor_operator(std::move(value), other);
}

mint::WeakReference mint_uint64_inc(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint64_t>::inc_operator(std::move(value));
}

mint::WeakReference mint_uint64_dec(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint64_t>::dec_operator(std::move(value));
}

mint::WeakReference mint_uint64_not(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint64_t>::not_operator(value);
}

mint::WeakReference mint_uint64_compl(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint64_t>::compl_operator(std::move(value));
}

mint::WeakReference mint_uint64_pos(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint64_t>::pos_operator(std::move(value));
}

mint::WeakReference mint_uint64_shift_left(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint64_t>::shift_left_operator(std::move(value), other);
}

mint::WeakReference mint_uint64_shift_right(mint::Cursor& /*cursor*/, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint64_t>::shift_right_operator(std::move(value), other);
}

mint::WeakReference mint_uint64_inclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint64_t>::inclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_uint64_exclusive_range(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& other) {
	return FixedInt<std::uint64_t>::exclusive_range_operator(cursor.ast(), value, other);
}

mint::WeakReference mint_uint64_subscript(mint::Cursor& cursor, mint::Reference& value, const mint::Reference& index) {
	return FixedInt<std::uint64_t>::subscript_operator(cursor.ast(), value, mint::to_signed_integer(cursor, index));
}

mint::WeakReference mint_uint64_subscript_move(mint::Cursor& cursor, mint::Reference& value,
    const mint::Reference& index, mint::Reference& other) {
	return FixedInt<std::uint64_t>::subscript_move_operator(std::move(value), mint::to_signed_integer(cursor, index),
	    other);
}

mint::WeakReference mint_uint64_to_number(mint::Cursor& /*cursor*/, mint::Reference& value) {
	return FixedInt<std::uint64_t>::to_number(value);
}

}

MINT_EXPORT_FUNCTION(mint_int8_create, 1);
MINT_EXPORT_FUNCTION(mint_int8_delete, 1);
MINT_EXPORT_FUNCTION(mint_int8_copy, 2);
MINT_EXPORT_FUNCTION(mint_int8_call, 1);
MINT_EXPORT_FUNCTION(mint_int8_add, 2);
MINT_EXPORT_FUNCTION(mint_int8_sub, 2);
MINT_EXPORT_FUNCTION(mint_int8_mul, 2);
MINT_EXPORT_FUNCTION(mint_int8_div, 2);
MINT_EXPORT_FUNCTION(mint_int8_pow, 2);
MINT_EXPORT_FUNCTION(mint_int8_mod, 2);
MINT_EXPORT_FUNCTION(mint_int8_eq, 2);
MINT_EXPORT_FUNCTION(mint_int8_ne, 2);
MINT_EXPORT_FUNCTION(mint_int8_lt, 2);
MINT_EXPORT_FUNCTION(mint_int8_gt, 2);
MINT_EXPORT_FUNCTION(mint_int8_le, 2);
MINT_EXPORT_FUNCTION(mint_int8_ge, 2);
MINT_EXPORT_FUNCTION(mint_int8_and, 2);
MINT_EXPORT_FUNCTION(mint_int8_or, 2);
MINT_EXPORT_FUNCTION(mint_int8_band, 2);
MINT_EXPORT_FUNCTION(mint_int8_bor, 2);
MINT_EXPORT_FUNCTION(mint_int8_xor, 2);
MINT_EXPORT_FUNCTION(mint_int8_inc, 1);
MINT_EXPORT_FUNCTION(mint_int8_dec, 1);
MINT_EXPORT_FUNCTION(mint_int8_not, 1);
MINT_EXPORT_FUNCTION(mint_int8_compl, 1);
MINT_EXPORT_FUNCTION(mint_int8_pos, 1);
MINT_EXPORT_FUNCTION(mint_int8_neg, 1);
MINT_EXPORT_FUNCTION(mint_int8_shift_left, 2);
MINT_EXPORT_FUNCTION(mint_int8_shift_right, 2);
MINT_EXPORT_FUNCTION(mint_int8_inclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_int8_exclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_int8_subscript, 2);
MINT_EXPORT_FUNCTION(mint_int8_subscript_move, 3);
MINT_EXPORT_FUNCTION(mint_int8_to_number, 1);

MINT_EXPORT_FUNCTION(mint_int16_create, 1);
MINT_EXPORT_FUNCTION(mint_int16_delete, 1);
MINT_EXPORT_FUNCTION(mint_int16_copy, 2);
MINT_EXPORT_FUNCTION(mint_int16_call, 1);
MINT_EXPORT_FUNCTION(mint_int16_add, 2);
MINT_EXPORT_FUNCTION(mint_int16_sub, 2);
MINT_EXPORT_FUNCTION(mint_int16_mul, 2);
MINT_EXPORT_FUNCTION(mint_int16_div, 2);
MINT_EXPORT_FUNCTION(mint_int16_pow, 2);
MINT_EXPORT_FUNCTION(mint_int16_mod, 2);
MINT_EXPORT_FUNCTION(mint_int16_eq, 2);
MINT_EXPORT_FUNCTION(mint_int16_ne, 2);
MINT_EXPORT_FUNCTION(mint_int16_lt, 2);
MINT_EXPORT_FUNCTION(mint_int16_gt, 2);
MINT_EXPORT_FUNCTION(mint_int16_le, 2);
MINT_EXPORT_FUNCTION(mint_int16_ge, 2);
MINT_EXPORT_FUNCTION(mint_int16_and, 2);
MINT_EXPORT_FUNCTION(mint_int16_or, 2);
MINT_EXPORT_FUNCTION(mint_int16_band, 2);
MINT_EXPORT_FUNCTION(mint_int16_bor, 2);
MINT_EXPORT_FUNCTION(mint_int16_xor, 2);
MINT_EXPORT_FUNCTION(mint_int16_inc, 1);
MINT_EXPORT_FUNCTION(mint_int16_dec, 1);
MINT_EXPORT_FUNCTION(mint_int16_not, 1);
MINT_EXPORT_FUNCTION(mint_int16_compl, 1);
MINT_EXPORT_FUNCTION(mint_int16_pos, 1);
MINT_EXPORT_FUNCTION(mint_int16_neg, 1);
MINT_EXPORT_FUNCTION(mint_int16_shift_left, 2);
MINT_EXPORT_FUNCTION(mint_int16_shift_right, 2);
MINT_EXPORT_FUNCTION(mint_int16_inclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_int16_exclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_int16_subscript, 2);
MINT_EXPORT_FUNCTION(mint_int16_subscript_move, 3);
MINT_EXPORT_FUNCTION(mint_int16_to_number, 1);

MINT_EXPORT_FUNCTION(mint_int32_create, 1);
MINT_EXPORT_FUNCTION(mint_int32_delete, 1);
MINT_EXPORT_FUNCTION(mint_int32_copy, 2);
MINT_EXPORT_FUNCTION(mint_int32_call, 1);
MINT_EXPORT_FUNCTION(mint_int32_add, 2);
MINT_EXPORT_FUNCTION(mint_int32_sub, 2);
MINT_EXPORT_FUNCTION(mint_int32_mul, 2);
MINT_EXPORT_FUNCTION(mint_int32_div, 2);
MINT_EXPORT_FUNCTION(mint_int32_pow, 2);
MINT_EXPORT_FUNCTION(mint_int32_mod, 2);
MINT_EXPORT_FUNCTION(mint_int32_eq, 2);
MINT_EXPORT_FUNCTION(mint_int32_ne, 2);
MINT_EXPORT_FUNCTION(mint_int32_lt, 2);
MINT_EXPORT_FUNCTION(mint_int32_gt, 2);
MINT_EXPORT_FUNCTION(mint_int32_le, 2);
MINT_EXPORT_FUNCTION(mint_int32_ge, 2);
MINT_EXPORT_FUNCTION(mint_int32_and, 2);
MINT_EXPORT_FUNCTION(mint_int32_or, 2);
MINT_EXPORT_FUNCTION(mint_int32_band, 2);
MINT_EXPORT_FUNCTION(mint_int32_bor, 2);
MINT_EXPORT_FUNCTION(mint_int32_xor, 2);
MINT_EXPORT_FUNCTION(mint_int32_inc, 1);
MINT_EXPORT_FUNCTION(mint_int32_dec, 1);
MINT_EXPORT_FUNCTION(mint_int32_not, 1);
MINT_EXPORT_FUNCTION(mint_int32_compl, 1);
MINT_EXPORT_FUNCTION(mint_int32_pos, 1);
MINT_EXPORT_FUNCTION(mint_int32_neg, 1);
MINT_EXPORT_FUNCTION(mint_int32_shift_left, 2);
MINT_EXPORT_FUNCTION(mint_int32_shift_right, 2);
MINT_EXPORT_FUNCTION(mint_int32_inclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_int32_exclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_int32_subscript, 2);
MINT_EXPORT_FUNCTION(mint_int32_subscript_move, 3);
MINT_EXPORT_FUNCTION(mint_int32_to_number, 1);

MINT_EXPORT_FUNCTION(mint_int64_create, 1);
MINT_EXPORT_FUNCTION(mint_int64_delete, 1);
MINT_EXPORT_FUNCTION(mint_int64_copy, 2);
MINT_EXPORT_FUNCTION(mint_int64_call, 1);
MINT_EXPORT_FUNCTION(mint_int64_add, 2);
MINT_EXPORT_FUNCTION(mint_int64_sub, 2);
MINT_EXPORT_FUNCTION(mint_int64_mul, 2);
MINT_EXPORT_FUNCTION(mint_int64_div, 2);
MINT_EXPORT_FUNCTION(mint_int64_pow, 2);
MINT_EXPORT_FUNCTION(mint_int64_mod, 2);
MINT_EXPORT_FUNCTION(mint_int64_eq, 2);
MINT_EXPORT_FUNCTION(mint_int64_ne, 2);
MINT_EXPORT_FUNCTION(mint_int64_lt, 2);
MINT_EXPORT_FUNCTION(mint_int64_gt, 2);
MINT_EXPORT_FUNCTION(mint_int64_le, 2);
MINT_EXPORT_FUNCTION(mint_int64_ge, 2);
MINT_EXPORT_FUNCTION(mint_int64_and, 2);
MINT_EXPORT_FUNCTION(mint_int64_or, 2);
MINT_EXPORT_FUNCTION(mint_int64_band, 2);
MINT_EXPORT_FUNCTION(mint_int64_bor, 2);
MINT_EXPORT_FUNCTION(mint_int64_xor, 2);
MINT_EXPORT_FUNCTION(mint_int64_inc, 1);
MINT_EXPORT_FUNCTION(mint_int64_dec, 1);
MINT_EXPORT_FUNCTION(mint_int64_not, 1);
MINT_EXPORT_FUNCTION(mint_int64_compl, 1);
MINT_EXPORT_FUNCTION(mint_int64_pos, 1);
MINT_EXPORT_FUNCTION(mint_int64_neg, 1);
MINT_EXPORT_FUNCTION(mint_int64_shift_left, 2);
MINT_EXPORT_FUNCTION(mint_int64_shift_right, 2);
MINT_EXPORT_FUNCTION(mint_int64_inclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_int64_exclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_int64_subscript, 2);
MINT_EXPORT_FUNCTION(mint_int64_subscript_move, 3);
MINT_EXPORT_FUNCTION(mint_int64_to_number, 1);

MINT_EXPORT_FUNCTION(mint_uint8_create, 1);
MINT_EXPORT_FUNCTION(mint_uint8_delete, 1);
MINT_EXPORT_FUNCTION(mint_uint8_copy, 2);
MINT_EXPORT_FUNCTION(mint_uint8_call, 1);
MINT_EXPORT_FUNCTION(mint_uint8_add, 2);
MINT_EXPORT_FUNCTION(mint_uint8_sub, 2);
MINT_EXPORT_FUNCTION(mint_uint8_mul, 2);
MINT_EXPORT_FUNCTION(mint_uint8_div, 2);
MINT_EXPORT_FUNCTION(mint_uint8_pow, 2);
MINT_EXPORT_FUNCTION(mint_uint8_mod, 2);
MINT_EXPORT_FUNCTION(mint_uint8_eq, 2);
MINT_EXPORT_FUNCTION(mint_uint8_ne, 2);
MINT_EXPORT_FUNCTION(mint_uint8_lt, 2);
MINT_EXPORT_FUNCTION(mint_uint8_gt, 2);
MINT_EXPORT_FUNCTION(mint_uint8_le, 2);
MINT_EXPORT_FUNCTION(mint_uint8_ge, 2);
MINT_EXPORT_FUNCTION(mint_uint8_and, 2);
MINT_EXPORT_FUNCTION(mint_uint8_or, 2);
MINT_EXPORT_FUNCTION(mint_uint8_band, 2);
MINT_EXPORT_FUNCTION(mint_uint8_bor, 2);
MINT_EXPORT_FUNCTION(mint_uint8_xor, 2);
MINT_EXPORT_FUNCTION(mint_uint8_inc, 1);
MINT_EXPORT_FUNCTION(mint_uint8_dec, 1);
MINT_EXPORT_FUNCTION(mint_uint8_not, 1);
MINT_EXPORT_FUNCTION(mint_uint8_compl, 1);
MINT_EXPORT_FUNCTION(mint_uint8_pos, 1);
MINT_EXPORT_FUNCTION(mint_uint8_shift_left, 2);
MINT_EXPORT_FUNCTION(mint_uint8_shift_right, 2);
MINT_EXPORT_FUNCTION(mint_uint8_inclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_uint8_exclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_uint8_subscript, 2);
MINT_EXPORT_FUNCTION(mint_uint8_subscript_move, 3);
MINT_EXPORT_FUNCTION(mint_uint8_to_number, 1);

MINT_EXPORT_FUNCTION(mint_uint16_create, 1);
MINT_EXPORT_FUNCTION(mint_uint16_delete, 1);
MINT_EXPORT_FUNCTION(mint_uint16_copy, 2);
MINT_EXPORT_FUNCTION(mint_uint16_call, 1);
MINT_EXPORT_FUNCTION(mint_uint16_add, 2);
MINT_EXPORT_FUNCTION(mint_uint16_sub, 2);
MINT_EXPORT_FUNCTION(mint_uint16_mul, 2);
MINT_EXPORT_FUNCTION(mint_uint16_div, 2);
MINT_EXPORT_FUNCTION(mint_uint16_pow, 2);
MINT_EXPORT_FUNCTION(mint_uint16_mod, 2);
MINT_EXPORT_FUNCTION(mint_uint16_eq, 2);
MINT_EXPORT_FUNCTION(mint_uint16_ne, 2);
MINT_EXPORT_FUNCTION(mint_uint16_lt, 2);
MINT_EXPORT_FUNCTION(mint_uint16_gt, 2);
MINT_EXPORT_FUNCTION(mint_uint16_le, 2);
MINT_EXPORT_FUNCTION(mint_uint16_ge, 2);
MINT_EXPORT_FUNCTION(mint_uint16_and, 2);
MINT_EXPORT_FUNCTION(mint_uint16_or, 2);
MINT_EXPORT_FUNCTION(mint_uint16_band, 2);
MINT_EXPORT_FUNCTION(mint_uint16_bor, 2);
MINT_EXPORT_FUNCTION(mint_uint16_xor, 2);
MINT_EXPORT_FUNCTION(mint_uint16_inc, 1);
MINT_EXPORT_FUNCTION(mint_uint16_dec, 1);
MINT_EXPORT_FUNCTION(mint_uint16_not, 1);
MINT_EXPORT_FUNCTION(mint_uint16_compl, 1);
MINT_EXPORT_FUNCTION(mint_uint16_pos, 1);
MINT_EXPORT_FUNCTION(mint_uint16_shift_left, 2);
MINT_EXPORT_FUNCTION(mint_uint16_shift_right, 2);
MINT_EXPORT_FUNCTION(mint_uint16_inclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_uint16_exclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_uint16_subscript, 2);
MINT_EXPORT_FUNCTION(mint_uint16_subscript_move, 3);
MINT_EXPORT_FUNCTION(mint_uint16_to_number, 1);

MINT_EXPORT_FUNCTION(mint_uint32_create, 1);
MINT_EXPORT_FUNCTION(mint_uint32_delete, 1);
MINT_EXPORT_FUNCTION(mint_uint32_copy, 2);
MINT_EXPORT_FUNCTION(mint_uint32_call, 1);
MINT_EXPORT_FUNCTION(mint_uint32_add, 2);
MINT_EXPORT_FUNCTION(mint_uint32_sub, 2);
MINT_EXPORT_FUNCTION(mint_uint32_mul, 2);
MINT_EXPORT_FUNCTION(mint_uint32_div, 2);
MINT_EXPORT_FUNCTION(mint_uint32_pow, 2);
MINT_EXPORT_FUNCTION(mint_uint32_mod, 2);
MINT_EXPORT_FUNCTION(mint_uint32_eq, 2);
MINT_EXPORT_FUNCTION(mint_uint32_ne, 2);
MINT_EXPORT_FUNCTION(mint_uint32_lt, 2);
MINT_EXPORT_FUNCTION(mint_uint32_gt, 2);
MINT_EXPORT_FUNCTION(mint_uint32_le, 2);
MINT_EXPORT_FUNCTION(mint_uint32_ge, 2);
MINT_EXPORT_FUNCTION(mint_uint32_and, 2);
MINT_EXPORT_FUNCTION(mint_uint32_or, 2);
MINT_EXPORT_FUNCTION(mint_uint32_band, 2);
MINT_EXPORT_FUNCTION(mint_uint32_bor, 2);
MINT_EXPORT_FUNCTION(mint_uint32_xor, 2);
MINT_EXPORT_FUNCTION(mint_uint32_inc, 1);
MINT_EXPORT_FUNCTION(mint_uint32_dec, 1);
MINT_EXPORT_FUNCTION(mint_uint32_not, 1);
MINT_EXPORT_FUNCTION(mint_uint32_compl, 1);
MINT_EXPORT_FUNCTION(mint_uint32_pos, 1);
MINT_EXPORT_FUNCTION(mint_uint32_shift_left, 2);
MINT_EXPORT_FUNCTION(mint_uint32_shift_right, 2);
MINT_EXPORT_FUNCTION(mint_uint32_inclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_uint32_exclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_uint32_subscript, 2);
MINT_EXPORT_FUNCTION(mint_uint32_subscript_move, 3);
MINT_EXPORT_FUNCTION(mint_uint32_to_number, 1);

MINT_EXPORT_FUNCTION(mint_uint64_create, 1);
MINT_EXPORT_FUNCTION(mint_uint64_delete, 1);
MINT_EXPORT_FUNCTION(mint_uint64_copy, 2);
MINT_EXPORT_FUNCTION(mint_uint64_call, 1);
MINT_EXPORT_FUNCTION(mint_uint64_add, 2);
MINT_EXPORT_FUNCTION(mint_uint64_sub, 2);
MINT_EXPORT_FUNCTION(mint_uint64_mul, 2);
MINT_EXPORT_FUNCTION(mint_uint64_div, 2);
MINT_EXPORT_FUNCTION(mint_uint64_pow, 2);
MINT_EXPORT_FUNCTION(mint_uint64_mod, 2);
MINT_EXPORT_FUNCTION(mint_uint64_eq, 2);
MINT_EXPORT_FUNCTION(mint_uint64_ne, 2);
MINT_EXPORT_FUNCTION(mint_uint64_lt, 2);
MINT_EXPORT_FUNCTION(mint_uint64_gt, 2);
MINT_EXPORT_FUNCTION(mint_uint64_le, 2);
MINT_EXPORT_FUNCTION(mint_uint64_ge, 2);
MINT_EXPORT_FUNCTION(mint_uint64_and, 2);
MINT_EXPORT_FUNCTION(mint_uint64_or, 2);
MINT_EXPORT_FUNCTION(mint_uint64_band, 2);
MINT_EXPORT_FUNCTION(mint_uint64_bor, 2);
MINT_EXPORT_FUNCTION(mint_uint64_xor, 2);
MINT_EXPORT_FUNCTION(mint_uint64_inc, 1);
MINT_EXPORT_FUNCTION(mint_uint64_dec, 1);
MINT_EXPORT_FUNCTION(mint_uint64_not, 1);
MINT_EXPORT_FUNCTION(mint_uint64_compl, 1);
MINT_EXPORT_FUNCTION(mint_uint64_pos, 1);
MINT_EXPORT_FUNCTION(mint_uint64_shift_left, 2);
MINT_EXPORT_FUNCTION(mint_uint64_shift_right, 2);
MINT_EXPORT_FUNCTION(mint_uint64_inclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_uint64_exclusive_range, 2);
MINT_EXPORT_FUNCTION(mint_uint64_subscript, 2);
MINT_EXPORT_FUNCTION(mint_uint64_subscript_move, 3);
MINT_EXPORT_FUNCTION(mint_uint64_to_number, 1);
