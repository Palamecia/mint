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

#include "mint/ast/symbol.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/memory/builtin/string.h"
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

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
static const std::string data_stream("Serializer.DataStream");

}

namespace {

constexpr const std::string_view base64_alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
                                                   "ghijklmnopqrstuvwxyz0123456789+/";
constexpr const std::string_view base64_url_alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
                                                       "ghijklmnopqrstuvwxyz0123456789-_";

template<class T>
void copy_from_buffer(const std::uint8_t* buffer, T* target) {
	std::ranges::copy_n(buffer, sizeof(T), std::bit_cast<std::uint8_t*>(target));
}

template<class T>
void copy_to_buffer(std::vector<std::uint8_t>& buffer, const T* source) {
	std::ranges::copy_n(std::bit_cast<const std::uint8_t*>(source), sizeof(T), std::back_inserter(buffer));
}

mint::WeakReference get_d_ptr(const mint::Reference& reference) {
	if (auto& object = reference.data<mint::Object>(); auto* info = object.metadata.find_member(symbols::d_ptr)) {
		return mint::Class::MemberInfo::get(*info, object);
	}
	return {};
}

std::string buffer_to_base64(std::vector<std::uint8_t>* buffer, std::string_view alphabet) {

	std::string result((buffer->size() + 2) / 3 * 4, '=');
	std::size_t padlen = 0;
	std::size_t i = 0;

	auto it = buffer->begin();
	while (it != buffer->end()) {

		int chunk = int(*it++) << 16;
		if (it != buffer->end()) {
			chunk |= int(*it++) << 8;
			if (it != buffer->end()) {
				chunk |= int(*it++);
			}
			else {
				padlen = 1;
			}
		}
		else {
			padlen = 2;
		}

		result[i++] = alphabet[(chunk & 0x00fc0000) >> 18];
		result[i++] = alphabet[(chunk & 0x0003f000) >> 12];

		switch (padlen) {
		case 0:
			result[i++] = alphabet[(chunk & 0x00000fc0) >> 6];
			result[i++] = alphabet[(chunk & 0x0000003f)];
			break;
		case 1:
			result[i++] = alphabet[(chunk & 0x00000fc0) >> 6];
			break;
		case 2:
			break;
		}
	}

	return result;
}

bool base64_to_buffer(std::vector<std::uint8_t>* buffer, const std::string& data, std::string_view alphabet) {

	unsigned int buf = 0;
	int nbits = 0;

	for (std::size_t i = 0; i < data.size(); ++i) {
		int ch = data[i];
		if (ch >= alphabet[0] && ch <= alphabet[25]) {
			buf = (buf << 6) | (ch - alphabet[0]);
		}
		else if (ch >= alphabet[26] && ch <= alphabet[51]) {
			buf = (buf << 6) | (ch - alphabet[26] + 26);
		}
		else if (ch >= alphabet[52] && ch <= alphabet[61]) {
			buf = (buf << 6) | (ch - alphabet[52] + 52);
		}
		else if (ch == alphabet[62]) {
			buf = (buf << 6) | 62;
		}
		else if (ch == alphabet[63]) {
			buf = (buf << 6) | 63;
		}
		else if (ch == '=') {
			if ((data.size() % 4) != 0) {
				return false;
			}
			if ((i == data.size() - 1) || (i == data.size() - 2 && data[++i] == '=')) {
				return true;
			}
			else {
				return false;
			}
		}
		else {
			return false;
		}
		nbits += 6;
		if (nbits >= 8) {
			nbits -= 8;
			buffer->emplace_back(buf >> nbits);
			buf &= (1 << nbits) - 1;
		}
	}

	return true;
}

mint::WeakReference mint_datastream_from_utf8_bytes(mint::Cursor& cursor, const mint::Reference& data,
    const mint::Reference& bytes, mint::Reference& count) {

	const std::intmax_t count_int = mint::to_signed_integer(cursor, count);
	const std::string bytes_str = mint::to_string(bytes);
	for (std::intmax_t index = 0; index < count_int; ++index) {
		mint::WeakReference item = array_get_item(data.data<mint::Array>(), index);
		if (const auto data_index = array_index(data.data<mint::Array>(), index); data_index < bytes_str.size()) {
			*get_d_ptr(item).data<mint::LibObject<std::uint8_t>>().ptr = bytes_str[data_index];
		}
		else {
			*get_d_ptr(item).data<mint::LibObject<std::uint8_t>>().ptr = 0;
		}
	}

	return data;
}

mint::WeakReference mint_datastream_create_buffer(mint::Cursor& cursor) {
	return mint::create_c_object(cursor.ast(), new std::vector<std::uint8_t>());
}

mint::WeakReference mint_datastream_delete_buffer(mint::Cursor& /*cursor*/, const mint::Reference& buffer) {
	delete buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	return {};
}

mint::WeakReference mint_datastream_contains_int8(mint::Cursor& cursor, const mint::Reference& buffer,
    mint::Reference& count) {
	return mint::create_boolean(buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->size()
	                            >= sizeof(std::int8_t) * mint::to_unsigned_integer(cursor, count));
}

mint::WeakReference mint_datastream_contains_int16(mint::Cursor& cursor, const mint::Reference& buffer,
    mint::Reference& count) {
	return mint::create_boolean(buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->size()
	                            >= sizeof(std::int16_t) * mint::to_unsigned_integer(cursor, count));
}

mint::WeakReference mint_datastream_contains_int32(mint::Cursor& cursor, const mint::Reference& buffer,
    mint::Reference& count) {
	return mint::create_boolean(buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->size()
	                            >= sizeof(std::int32_t) * mint::to_unsigned_integer(cursor, count));
}

mint::WeakReference mint_datastream_contains_int64(mint::Cursor& cursor, const mint::Reference& buffer,
    mint::Reference& count) {
	return mint::create_boolean(buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->size()
	                            >= sizeof(std::int64_t) * mint::to_unsigned_integer(cursor, count));
}

mint::WeakReference mint_datastream_contains_uint8(mint::Cursor& cursor, const mint::Reference& buffer,
    mint::Reference& count) {
	return mint::create_boolean(buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->size()
	                            >= sizeof(std::uint8_t) * mint::to_unsigned_integer(cursor, count));
}

mint::WeakReference mint_datastream_contains_uint16(mint::Cursor& cursor, const mint::Reference& buffer,
    mint::Reference& count) {
	return mint::create_boolean(buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->size()
	                            >= sizeof(std::uint16_t) * mint::to_unsigned_integer(cursor, count));
}

mint::WeakReference mint_datastream_contains_uint32(mint::Cursor& cursor, const mint::Reference& buffer,
    mint::Reference& count) {
	return mint::create_boolean(buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->size()
	                            >= sizeof(std::uint32_t) * mint::to_unsigned_integer(cursor, count));
}

mint::WeakReference mint_datastream_contains_uint64(mint::Cursor& cursor, const mint::Reference& buffer,
    mint::Reference& count) {
	return mint::create_boolean(buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->size()
	                            >= sizeof(std::uint64_t) * mint::to_unsigned_integer(cursor, count));
}

mint::WeakReference mint_datastream_contains_number(mint::Cursor& /*cursor*/, const mint::Reference& buffer) {
	return mint::create_boolean(
	    buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->size() >= sizeof(mint::Number::value));
}

mint::WeakReference mint_datastream_contains_boolean(mint::Cursor& /*cursor*/, const mint::Reference& buffer) {
	return mint::create_boolean(
	    buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->size() >= sizeof(mint::Boolean::value));
}

mint::WeakReference mint_datastream_contains_string(mint::Cursor& /*cursor*/, const mint::Reference& buffer) {
	auto& buffer_data = *buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	return mint::create_boolean(std::ranges::find(buffer_data, 0) != buffer_data.end());
}

mint::WeakReference mint_datastream_get(mint::Cursor& cursor, const mint::Reference& buffer,
    const mint::Reference& data, const mint::Reference& count) {

	auto* buffer_data = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->data();

	for (std::intmax_t index = 0; index < mint::to_signed_integer(cursor, count); ++index) {
		mint::WeakReference item = array_get_item(data.data<mint::Array>(), index);
		if (mint::is_instance_of(item, mint::Class::Metatype::object)) {
			auto& object = item.data<mint::Object>();
			if (object.metadata.full_name() == symbols::int8) {
				copy_from_buffer(buffer_data, get_d_ptr(item).data<mint::LibObject<std::int8_t>>().ptr);
				std::advance(buffer_data, sizeof(std::int8_t));
			}
			else if (object.metadata.full_name() == symbols::int16) {
				copy_from_buffer(buffer_data, get_d_ptr(item).data<mint::LibObject<std::int16_t>>().ptr);
				std::advance(buffer_data, sizeof(std::int16_t));
			}
			else if (object.metadata.full_name() == symbols::int32) {
				copy_from_buffer(buffer_data, get_d_ptr(item).data<mint::LibObject<std::int32_t>>().ptr);
				std::advance(buffer_data, sizeof(std::int32_t));
			}
			else if (object.metadata.full_name() == symbols::int64) {
				copy_from_buffer(buffer_data, get_d_ptr(item).data<mint::LibObject<std::int64_t>>().ptr);
				std::advance(buffer_data, sizeof(std::int64_t));
			}
			else if (object.metadata.full_name() == symbols::uint8) {
				copy_from_buffer(buffer_data, get_d_ptr(item).data<mint::LibObject<std::uint8_t>>().ptr);
				std::advance(buffer_data, sizeof(std::uint8_t));
			}
			else if (object.metadata.full_name() == symbols::uint16) {
				copy_from_buffer(buffer_data, get_d_ptr(item).data<mint::LibObject<std::uint16_t>>().ptr);
				std::advance(buffer_data, sizeof(std::uint16_t));
			}
			else if (object.metadata.full_name() == symbols::uint32) {
				copy_from_buffer(buffer_data, get_d_ptr(item).data<mint::LibObject<std::uint32_t>>().ptr);
				std::advance(buffer_data, sizeof(std::uint32_t));
			}
			else if (object.metadata.full_name() == symbols::uint64) {
				copy_from_buffer(buffer_data, get_d_ptr(item).data<mint::LibObject<std::uint64_t>>().ptr);
				std::advance(buffer_data, sizeof(std::uint64_t));
			}
		}
	}

	return {};
}

mint::WeakReference mint_datastream_get_substr(mint::Cursor& cursor, const mint::Reference& buffer,
    const mint::Reference& from, mint::Reference& length) {
	std::vector<std::uint8_t>& buffer_data = *buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	return mint::create_string(cursor.ast(),
	    std::string(std::next(std::bit_cast<char*>(buffer_data.data()), mint::to_signed_integer(cursor, from)),
	        mint::to_unsigned_integer(cursor, length)));
}

mint::WeakReference mint_datastream_get(mint::Cursor& /*cursor*/, const mint::Reference& buffer,
    const mint::Reference& data) {

	auto* buffer_data = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->data();

	switch (data.data().format()) {
	case mint::Data::Format::none:
	case mint::Data::Format::null:
	case mint::Data::Format::package:
	case mint::Data::Format::function:
	case mint::Data::Format::coroutine:
		break;

	case mint::Data::Format::number:
		copy_from_buffer(buffer_data, &data.data<mint::Number>().value);
		break;

	case mint::Data::Format::boolean:
		copy_from_buffer(buffer_data, &data.data<mint::Boolean>().value);
		break;

	case mint::Data::Format::object:
		switch (auto& object = data.data<mint::Object>(); object.metadata.metatype()) {
		case mint::Class::Metatype::object:
			if (object.metadata.full_name() == symbols::int8) {
				copy_from_buffer(buffer_data, get_d_ptr(data).data<mint::LibObject<std::int8_t>>().ptr);
				break;
			}
			if (object.metadata.full_name() == symbols::int16) {
				copy_from_buffer(buffer_data, get_d_ptr(data).data<mint::LibObject<std::int16_t>>().ptr);
				break;
			}
			if (object.metadata.full_name() == symbols::int32) {
				copy_from_buffer(buffer_data, get_d_ptr(data).data<mint::LibObject<std::int32_t>>().ptr);
				break;
			}
			if (object.metadata.full_name() == symbols::int64) {
				copy_from_buffer(buffer_data, get_d_ptr(data).data<mint::LibObject<std::int64_t>>().ptr);
				break;
			}
			if (object.metadata.full_name() == symbols::uint8) {
				copy_from_buffer(buffer_data, get_d_ptr(data).data<mint::LibObject<std::uint8_t>>().ptr);
				break;
			}
			if (object.metadata.full_name() == symbols::uint16) {
				copy_from_buffer(buffer_data, get_d_ptr(data).data<mint::LibObject<std::uint16_t>>().ptr);
				break;
			}
			if (object.metadata.full_name() == symbols::uint32) {
				copy_from_buffer(buffer_data, get_d_ptr(data).data<mint::LibObject<std::uint32_t>>().ptr);
				break;
			}
			if (object.metadata.full_name() == symbols::uint64) {
				copy_from_buffer(buffer_data, get_d_ptr(data).data<mint::LibObject<std::uint64_t>>().ptr);
				break;
			}
			break;

		case mint::Class::Metatype::string:
			data.data<mint::String>().str = std::bit_cast<char*>(buffer_data);
			break;

		case mint::Class::Metatype::regex:
		case mint::Class::Metatype::array:
		case mint::Class::Metatype::hash:
		case mint::Class::Metatype::iterator:
		case mint::Class::Metatype::async_iterator:
		case mint::Class::Metatype::library:
		case mint::Class::Metatype::libobject:
			break;
		}
		break;
	}

	return {};
}

mint::WeakReference mint_datastream_to_base64(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	return mint::create_string(cursor.ast(),
	    buffer_to_base64(d_ptr.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr, base64_alphabet));
}

mint::WeakReference mint_datastream_to_base64url(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	return mint::create_string(cursor.ast(),
	    buffer_to_base64(d_ptr.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr, base64_url_alphabet));
}

mint::WeakReference mint_datastream_write_base64(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& data) {
	return mint::create_boolean(base64_to_buffer(d_ptr.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr,
	    to_string(data), base64_alphabet));
}

mint::WeakReference mint_datastream_write_base64url(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& data) {
	return mint::create_boolean(base64_to_buffer(d_ptr.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr,
	    to_string(data), base64_url_alphabet));
}

mint::WeakReference mint_datastream_read(mint::Cursor& /*cursor*/, const mint::Reference& buffer,
    const mint::Reference& data) {

	auto& buffer_object = *buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

	switch (data.data().format()) {
	case mint::Data::Format::none:
	case mint::Data::Format::null:
	case mint::Data::Format::package:
	case mint::Data::Format::function:
	case mint::Data::Format::coroutine:
		break;

	case mint::Data::Format::number:
		copy_from_buffer(buffer_object.data(), &data.data<mint::Number>().value);
		buffer_object.erase(buffer_object.begin(), std::next(buffer_object.begin(), sizeof(mint::Number::value)));
		break;

	case mint::Data::Format::boolean:
		copy_from_buffer(buffer_object.data(), &data.data<mint::Boolean>().value);
		buffer_object.erase(buffer_object.begin(), std::next(buffer_object.begin(), sizeof(mint::Boolean::value)));
		break;

	case mint::Data::Format::object:
		switch (auto& object = data.data<mint::Object>(); object.metadata.metatype()) {
		case mint::Class::Metatype::object:
			if (object.metadata.full_name() == symbols::int8) {
				copy_from_buffer(buffer_object.data(), get_d_ptr(data).data<mint::LibObject<std::int8_t>>().ptr);
				buffer_object.erase(buffer_object.begin(), std::next(buffer_object.begin(), sizeof(std::int8_t)));
				break;
			}
			if (object.metadata.full_name() == symbols::int16) {
				copy_from_buffer(buffer_object.data(), get_d_ptr(data).data<mint::LibObject<std::int16_t>>().ptr);
				buffer_object.erase(buffer_object.begin(), std::next(buffer_object.begin(), sizeof(std::int16_t)));
				break;
			}
			if (object.metadata.full_name() == symbols::int32) {
				copy_from_buffer(buffer_object.data(), get_d_ptr(data).data<mint::LibObject<std::int32_t>>().ptr);
				buffer_object.erase(buffer_object.begin(), std::next(buffer_object.begin(), sizeof(std::int32_t)));
				break;
			}
			if (object.metadata.full_name() == symbols::int64) {
				copy_from_buffer(buffer_object.data(), get_d_ptr(data).data<mint::LibObject<std::int64_t>>().ptr);
				buffer_object.erase(buffer_object.begin(), std::next(buffer_object.begin(), sizeof(std::int64_t)));
				break;
			}
			if (object.metadata.full_name() == symbols::uint8) {
				copy_from_buffer(buffer_object.data(), get_d_ptr(data).data<mint::LibObject<std::uint8_t>>().ptr);
				buffer_object.erase(buffer_object.begin(), std::next(buffer_object.begin(), sizeof(std::uint8_t)));
				break;
			}
			if (object.metadata.full_name() == symbols::uint16) {
				copy_from_buffer(buffer_object.data(), get_d_ptr(data).data<mint::LibObject<std::uint16_t>>().ptr);
				buffer_object.erase(buffer_object.begin(), std::next(buffer_object.begin(), sizeof(std::uint16_t)));
				break;
			}
			if (object.metadata.full_name() == symbols::uint32) {
				copy_from_buffer(buffer_object.data(), get_d_ptr(data).data<mint::LibObject<std::uint32_t>>().ptr);
				buffer_object.erase(buffer_object.begin(), std::next(buffer_object.begin(), sizeof(std::uint32_t)));
				break;
			}
			if (object.metadata.full_name() == symbols::uint64) {
				copy_from_buffer(buffer_object.data(), get_d_ptr(data).data<mint::LibObject<std::uint64_t>>().ptr);
				buffer_object.erase(buffer_object.begin(), std::next(buffer_object.begin(), sizeof(std::uint64_t)));
				break;
			}
			break;

		case mint::Class::Metatype::string:
			data.data<mint::String>().str = std::bit_cast<char*>(buffer_object.data());
			buffer_object.erase(buffer_object.begin(),
			    std::next(buffer_object.begin(), static_cast<std::ptrdiff_t>(data.data<mint::String>().str.size()) + 1));
			break;

		case mint::Class::Metatype::regex:
		case mint::Class::Metatype::array:
		case mint::Class::Metatype::hash:
		case mint::Class::Metatype::iterator:
		case mint::Class::Metatype::async_iterator:
		case mint::Class::Metatype::library:
		case mint::Class::Metatype::libobject:
			break;
		}
		break;
	}

	return {};
}

mint::WeakReference mint_datastream_write(mint::Cursor& /*cursor*/, const mint::Reference& buffer,
    const mint::Reference& data) {

	auto& buffer_object = *buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

	switch (data.data().format()) {
	case mint::Data::Format::none:
	case mint::Data::Format::coroutine:
		break;

	case mint::Data::Format::null:
	case mint::Data::Format::package:
	case mint::Data::Format::function:
		buffer_object.append_range(to_string(data));
		buffer_object.push_back(0);
		break;

	case mint::Data::Format::number:
		copy_to_buffer(buffer_object, &data.data<mint::Number>().value);
		break;

	case mint::Data::Format::boolean:
		copy_to_buffer(buffer_object, &data.data<mint::Boolean>().value);
		break;

	case mint::Data::Format::object:
		switch (auto& object = data.data<mint::Object>(); object.metadata.metatype()) {
		case mint::Class::Metatype::object:
			if (object.metadata.full_name() == symbols::data_stream) {
				buffer_object.append_range(*get_d_ptr(data).data<mint::LibObject<std::vector<std::uint8_t>>>().ptr);
				break;
			}
			if (object.metadata.full_name() == symbols::int8) {
				copy_to_buffer(buffer_object, get_d_ptr(data).data<mint::LibObject<std::int8_t>>().ptr);
				break;
			}
			if (object.metadata.full_name() == symbols::int16) {
				copy_to_buffer(buffer_object, get_d_ptr(data).data<mint::LibObject<std::int16_t>>().ptr);
				break;
			}
			if (object.metadata.full_name() == symbols::int32) {
				copy_to_buffer(buffer_object, get_d_ptr(data).data<mint::LibObject<std::int32_t>>().ptr);
				break;
			}
			if (object.metadata.full_name() == symbols::int64) {
				copy_to_buffer(buffer_object, get_d_ptr(data).data<mint::LibObject<std::int64_t>>().ptr);
				break;
			}
			if (object.metadata.full_name() == symbols::uint8) {
				copy_to_buffer(buffer_object, get_d_ptr(data).data<mint::LibObject<std::uint8_t>>().ptr);
				break;
			}
			if (object.metadata.full_name() == symbols::uint16) {
				copy_to_buffer(buffer_object, get_d_ptr(data).data<mint::LibObject<std::uint16_t>>().ptr);
				break;
			}
			if (object.metadata.full_name() == symbols::uint32) {
				copy_to_buffer(buffer_object, get_d_ptr(data).data<mint::LibObject<std::uint32_t>>().ptr);
				break;
			}
			if (object.metadata.full_name() == symbols::uint64) {
				copy_to_buffer(buffer_object, get_d_ptr(data).data<mint::LibObject<std::uint64_t>>().ptr);
				break;
			}
			break;

		case mint::Class::Metatype::string:
			buffer_object.append_range(data.data<mint::String>().str);
			buffer_object.push_back(0);
			break;

		case mint::Class::Metatype::regex:
		case mint::Class::Metatype::array:
		case mint::Class::Metatype::hash:
		case mint::Class::Metatype::iterator:
		case mint::Class::Metatype::async_iterator:
		case mint::Class::Metatype::library:
		case mint::Class::Metatype::libobject:
			buffer_object.append_range(to_string(data));
			buffer_object.push_back(0);
			break;
		}
		break;
	}

	return {};
}

mint::WeakReference mint_datastream_remove(mint::Cursor& cursor, const mint::Reference& buffer,
    const mint::Reference& count) {
	std::vector<std::uint8_t>* self = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	self->erase(self->begin(), std::next(self->begin(), mint::to_signed_integer(cursor, count)));
	return {};
}

mint::WeakReference mint_datastream_size(mint::Cursor& /*cursor*/, const mint::Reference& buffer) {
	return mint::create_unsigned_number(buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->size());
}

mint::WeakReference mint_datastream_empty(mint::Cursor& /*cursor*/, const mint::Reference& buffer) {
	return mint::create_boolean(buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->empty());
}

}

MINT_EXPORT_FUNCTION(mint_datastream_from_utf8_bytes, 3)
MINT_EXPORT_FUNCTION(mint_datastream_create_buffer, 0)
MINT_EXPORT_FUNCTION(mint_datastream_delete_buffer, 1)
MINT_EXPORT_FUNCTION(mint_datastream_contains_int8, 2)
MINT_EXPORT_FUNCTION(mint_datastream_contains_int16, 2)
MINT_EXPORT_FUNCTION(mint_datastream_contains_int32, 2)
MINT_EXPORT_FUNCTION(mint_datastream_contains_int64, 2)
MINT_EXPORT_FUNCTION(mint_datastream_contains_uint8, 2)
MINT_EXPORT_FUNCTION(mint_datastream_contains_uint16, 2)
MINT_EXPORT_FUNCTION(mint_datastream_contains_uint32, 2)
MINT_EXPORT_FUNCTION(mint_datastream_contains_uint64, 2)
MINT_EXPORT_FUNCTION(mint_datastream_contains_number, 1)
MINT_EXPORT_FUNCTION(mint_datastream_contains_boolean, 1)
MINT_EXPORT_FUNCTION(mint_datastream_contains_string, 1)
MINT_EXPORT_FUNCTION_OVERLOAD(mint_datastream_get, 3, mint::Cursor&, const mint::Reference&, const mint::Reference&,
    const mint::Reference&)
MINT_EXPORT_FUNCTION(mint_datastream_get_substr, 3)
MINT_EXPORT_FUNCTION_OVERLOAD(mint_datastream_get, 2, mint::Cursor&, const mint::Reference&, const mint::Reference&)
MINT_EXPORT_FUNCTION(mint_datastream_to_base64, 1)
MINT_EXPORT_FUNCTION(mint_datastream_to_base64url, 1)
MINT_EXPORT_FUNCTION(mint_datastream_write_base64, 2)
MINT_EXPORT_FUNCTION(mint_datastream_write_base64url, 2)
MINT_EXPORT_FUNCTION(mint_datastream_read, 2)
MINT_EXPORT_FUNCTION(mint_datastream_write, 2)
MINT_EXPORT_FUNCTION(mint_datastream_remove, 2)
MINT_EXPORT_FUNCTION(mint_datastream_size, 1)
MINT_EXPORT_FUNCTION(mint_datastream_empty, 1)
