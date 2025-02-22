/**
 * Copyright (c) 2025 Gauvain CHERY.
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
#include <mint/memory/casttool.h>
#include <mint/memory/builtin/string.h>
#include <algorithm>

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
static const std::string DataStream("Serializer.DataStream");

}

namespace {

static constexpr const char *BASE64_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
											   "ghijklmnopqrstuvwxyz0123456789+/";
static constexpr const char *BASE64_URL_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
												   "ghijklmnopqrstuvwxyz0123456789-_";

WeakReference get_d_ptr(Reference &reference) {

	auto *object = reference.data<Object>();
	auto it = object->metadata->members().find(symbols::d_ptr);

	if (it != object->metadata->members().end()) {
		return WeakReference::share(Class::MemberInfo::get(it->second, object));
	}

	return {};
}

std::string buffer_to_base64(std::vector<uint8_t> *buffer, const char *alphabet) {

	std::string result((buffer->size() + 2) / 3 * 4, '=');
	size_t padlen = 0;
	size_t i = 0;

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

bool base64_to_buffer(std::vector<uint8_t> *buffer, const std::string &data, const char *alphabet) {

	unsigned int buf = 0;
	int nbits = 0;

	for (size_t i = 0; i < data.size(); ++i) {
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
			else if ((i == data.size() - 1) || (i == data.size() - 2 && data[++i] == '=')) {
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

}

MINT_FUNCTION(mint_datastream_from_utf8_bytes, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &count = helper.pop_parameter();
	Reference &bytes = helper.pop_parameter();
	Reference &data = helper.pop_parameter();

	const intmax_t count_int = to_integer(cursor, count);
	const std::string bytes_str = to_string(bytes);
	for (intmax_t index = 0; index < count_int; ++index) {
		WeakReference item = array_get_item(data.data<Array>(), index);
		if (index < bytes_str.size()) {
			*get_d_ptr(item).data<LibObject<uint8_t>>()->impl = bytes_str[index];
		}
		else {
			*get_d_ptr(item).data<LibObject<uint8_t>>()->impl = 0;
		}
	}

	helper.return_value(std::move(data));
}

MINT_FUNCTION(mint_datastream_create_buffer, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	helper.return_value(create_object(new std::vector<uint8_t>));
}

MINT_FUNCTION(mint_datastream_delete_buffer, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &buffer = helper.pop_parameter();
	delete buffer.data<LibObject<std::vector<uint8_t>>>()->impl;
}

MINT_FUNCTION(mint_datastream_contains_int8, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &count = helper.pop_parameter();
	Reference &buffer = helper.pop_parameter();
	helper.return_value(create_boolean(buffer.data<LibObject<std::vector<uint8_t>>>()->impl->size()
									   >= sizeof(int8_t) * to_integer(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_int16, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &count = helper.pop_parameter();
	Reference &buffer = helper.pop_parameter();
	helper.return_value(create_boolean(buffer.data<LibObject<std::vector<uint8_t>>>()->impl->size()
									   >= sizeof(int16_t) * to_integer(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_int32, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &count = helper.pop_parameter();
	Reference &buffer = helper.pop_parameter();
	helper.return_value(create_boolean(buffer.data<LibObject<std::vector<uint8_t>>>()->impl->size()
									   >= sizeof(int32_t) * to_integer(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_int64, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &count = helper.pop_parameter();
	Reference &buffer = helper.pop_parameter();
	helper.return_value(create_boolean(buffer.data<LibObject<std::vector<uint8_t>>>()->impl->size()
									   >= sizeof(int64_t) * to_integer(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_uint8, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &count = helper.pop_parameter();
	Reference &buffer = helper.pop_parameter();
	helper.return_value(create_boolean(buffer.data<LibObject<std::vector<uint8_t>>>()->impl->size()
									   >= sizeof(uint8_t) * to_integer(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_uint16, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &count = helper.pop_parameter();
	Reference &buffer = helper.pop_parameter();
	helper.return_value(create_boolean(buffer.data<LibObject<std::vector<uint8_t>>>()->impl->size()
									   >= sizeof(uint16_t) * to_integer(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_uint32, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &count = helper.pop_parameter();
	Reference &buffer = helper.pop_parameter();
	helper.return_value(create_boolean(buffer.data<LibObject<std::vector<uint8_t>>>()->impl->size()
									   >= sizeof(uint32_t) * to_integer(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_uint64, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &count = helper.pop_parameter();
	Reference &buffer = helper.pop_parameter();
	helper.return_value(create_boolean(buffer.data<LibObject<std::vector<uint8_t>>>()->impl->size()
									   >= sizeof(uint64_t) * to_integer(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &buffer = helper.pop_parameter();
	helper.return_value(
		create_boolean(buffer.data<LibObject<std::vector<uint8_t>>>()->impl->size() >= sizeof(Number::value)));
}

MINT_FUNCTION(mint_datastream_contains_boolean, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &buffer = helper.pop_parameter();
	helper.return_value(
		create_boolean(buffer.data<LibObject<std::vector<uint8_t>>>()->impl->size() >= sizeof(Boolean::value)));
}

MINT_FUNCTION(mint_datastream_contains_string, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &buffer = helper.pop_parameter();
	auto begin = buffer.data<LibObject<std::vector<uint8_t>>>()->impl->begin();
	auto end = buffer.data<LibObject<std::vector<uint8_t>>>()->impl->end();
	helper.return_value(create_boolean(find(begin, end, 0) != end));
}

MINT_FUNCTION(mint_datastream_get, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &count = helper.pop_parameter();
	Reference &data = helper.pop_parameter();
	Reference &buffer = helper.pop_parameter();

	uint8_t *buffer_data = buffer.data<LibObject<std::vector<uint8_t>>>()->impl->data();

	for (intmax_t index = 0; index < to_integer(cursor, count); ++index) {
		WeakReference item = array_get_item(data.data<Array>(), index);
		switch (item.data()->format) {
		case Data::FMT_NONE:
		case Data::FMT_NULL:
		case Data::FMT_NUMBER:
		case Data::FMT_BOOLEAN:
		case Data::FMT_PACKAGE:
		case Data::FMT_FUNCTION:
			break;

		case Data::FMT_OBJECT:
			if (auto *object = item.data<Object>()) {
				switch (object->metadata->metatype()) {
				case Class::OBJECT:
					if (object->metadata->full_name() == symbols::int8) {
						int8_t *value = get_d_ptr(item).data<LibObject<int8_t>>()->impl;
						memcpy(value, buffer_data, sizeof(int8_t));
						buffer_data += sizeof(int8_t);
						break;
					}
					if (object->metadata->full_name() == symbols::int16) {
						int16_t *value = get_d_ptr(item).data<LibObject<int16_t>>()->impl;
						memcpy(value, buffer_data, sizeof(int16_t));
						buffer_data += sizeof(int16_t);
						break;
					}
					if (object->metadata->full_name() == symbols::int32) {
						int32_t *value = get_d_ptr(item).data<LibObject<int32_t>>()->impl;
						memcpy(value, buffer_data, sizeof(int32_t));
						buffer_data += sizeof(int32_t);
						break;
					}
					if (object->metadata->full_name() == symbols::int64) {
						int64_t *value = get_d_ptr(item).data<LibObject<int64_t>>()->impl;
						memcpy(value, buffer_data, sizeof(int64_t));
						buffer_data += sizeof(int64_t);
						break;
					}
					if (object->metadata->full_name() == symbols::uint8) {
						uint8_t *value = get_d_ptr(item).data<LibObject<uint8_t>>()->impl;
						memcpy(value, buffer_data, sizeof(uint8_t));
						buffer_data += sizeof(uint8_t);
						break;
					}
					if (object->metadata->full_name() == symbols::uint16) {
						uint16_t *value = get_d_ptr(item).data<LibObject<uint16_t>>()->impl;
						memcpy(value, buffer_data, sizeof(uint16_t));
						buffer_data += sizeof(uint16_t);
						break;
					}
					if (object->metadata->full_name() == symbols::uint32) {
						uint32_t *value = get_d_ptr(item).data<LibObject<uint32_t>>()->impl;
						memcpy(value, buffer_data, sizeof(uint32_t));
						buffer_data += sizeof(uint32_t);
						break;
					}
					if (object->metadata->full_name() == symbols::uint64) {
						uint64_t *value = get_d_ptr(item).data<LibObject<uint64_t>>()->impl;
						memcpy(value, buffer_data, sizeof(uint64_t));
						buffer_data += sizeof(uint64_t);
						break;
					}
					break;

				case Class::STRING:
				case Class::REGEX:
				case Class::ARRAY:
				case Class::HASH:
				case Class::ITERATOR:
				case Class::LIBRARY:
				case Class::LIBOBJECT:
					break;
				}
			}
			break;
		}
	}
}

MINT_FUNCTION(mint_datastream_get_substr, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &length = helper.pop_parameter();
	Reference &from = helper.pop_parameter();
	Reference &buffer = helper.pop_parameter();

	std::vector<uint8_t> &buffer_data = *buffer.data<LibObject<std::vector<uint8_t>>>()->impl;
	helper.return_value(
		create_string(std::string(reinterpret_cast<char *>(buffer_data.data()) + to_integer(cursor, from),
								  to_integer(cursor, length))));
}

MINT_FUNCTION(mint_datastream_get, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &data = helper.pop_parameter();
	Reference &buffer = helper.pop_parameter();

	std::vector<uint8_t> &buffer_data = *buffer.data<LibObject<std::vector<uint8_t>>>()->impl;

	switch (data.data()->format) {
	case Data::FMT_NONE:
	case Data::FMT_NULL:
	case Data::FMT_PACKAGE:
	case Data::FMT_FUNCTION:
		break;

	case Data::FMT_NUMBER:
		memcpy(&data.data<Number>()->value, buffer_data.data(), sizeof(Number::value));
		break;

	case Data::FMT_BOOLEAN:
		memcpy(&data.data<Boolean>()->value, buffer_data.data(), sizeof(Boolean::value));
		break;

	case Data::FMT_OBJECT:
		if (auto *object = data.data<Object>()) {
			switch (object->metadata->metatype()) {
			case Class::OBJECT:
				if (object->metadata->full_name() == symbols::int8) {
					int8_t *value = get_d_ptr(data).data<LibObject<int8_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int8_t));
					break;
				}
				if (object->metadata->full_name() == symbols::int16) {
					int16_t *value = get_d_ptr(data).data<LibObject<int16_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int16_t));
					break;
				}
				if (object->metadata->full_name() == symbols::int32) {
					int32_t *value = get_d_ptr(data).data<LibObject<int32_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int32_t));
					break;
				}
				if (object->metadata->full_name() == symbols::int64) {
					int64_t *value = get_d_ptr(data).data<LibObject<int64_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int64_t));
					break;
				}
				if (object->metadata->full_name() == symbols::uint8) {
					uint8_t *value = get_d_ptr(data).data<LibObject<uint8_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint8_t));
					break;
				}
				if (object->metadata->full_name() == symbols::uint16) {
					uint16_t *value = get_d_ptr(data).data<LibObject<uint16_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint16_t));
					break;
				}
				if (object->metadata->full_name() == symbols::uint32) {
					uint32_t *value = get_d_ptr(data).data<LibObject<uint32_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint32_t));
					break;
				}
				if (object->metadata->full_name() == symbols::uint64) {
					uint64_t *value = get_d_ptr(data).data<LibObject<uint64_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint64_t));
					break;
				}
				break;

			case Class::STRING:
				data.data<String>()->str = reinterpret_cast<char *>(buffer_data.data());
				break;

			case Class::REGEX:
			case Class::ARRAY:
			case Class::HASH:
			case Class::ITERATOR:
			case Class::LIBRARY:
			case Class::LIBOBJECT:
				break;
			}
		}
		break;
	}
}

MINT_FUNCTION(mint_datastream_to_base64, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &d_ptr = helper.pop_parameter();

	helper.return_value(
		create_string(buffer_to_base64(d_ptr.data<LibObject<std::vector<uint8_t>>>()->impl, BASE64_ALPHABET)));
}

MINT_FUNCTION(mint_datastream_to_base64url, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &d_ptr = helper.pop_parameter();

	helper.return_value(
		create_string(buffer_to_base64(d_ptr.data<LibObject<std::vector<uint8_t>>>()->impl, BASE64_URL_ALPHABET)));
}

MINT_FUNCTION(mint_datastream_write_base64, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &data = helper.pop_parameter();
	const Reference &d_ptr = helper.pop_parameter();

	helper.return_value(create_boolean(
		base64_to_buffer(d_ptr.data<LibObject<std::vector<uint8_t>>>()->impl, to_string(data), BASE64_ALPHABET)));
}

MINT_FUNCTION(mint_datastream_write_base64url, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &data = helper.pop_parameter();
	const Reference &d_ptr = helper.pop_parameter();

	helper.return_value(create_boolean(
		base64_to_buffer(d_ptr.data<LibObject<std::vector<uint8_t>>>()->impl, to_string(data), BASE64_URL_ALPHABET)));
}

MINT_FUNCTION(mint_datastream_read, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &data = helper.pop_parameter();
	Reference &buffer = helper.pop_parameter();

	std::vector<uint8_t> &buffer_data = *buffer.data<LibObject<std::vector<uint8_t>>>()->impl;

	switch (data.data()->format) {
	case Data::FMT_NONE:
	case Data::FMT_NULL:
	case Data::FMT_PACKAGE:
	case Data::FMT_FUNCTION:
		break;

	case Data::FMT_NUMBER:
		memcpy(&data.data<Number>()->value, buffer_data.data(), sizeof(Number::value));
		buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(Number::value));
		break;

	case Data::FMT_BOOLEAN:
		memcpy(&data.data<Boolean>()->value, buffer_data.data(), sizeof(Boolean::value));
		buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(Boolean::value));
		break;

	case Data::FMT_OBJECT:
		if (auto *object = data.data<Object>()) {
			switch (object->metadata->metatype()) {
			case Class::OBJECT:
				if (object->metadata->full_name() == symbols::int8) {
					int8_t *value = get_d_ptr(data).data<LibObject<int8_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int8_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(int8_t));
					break;
				}
				if (object->metadata->full_name() == symbols::int16) {
					int16_t *value = get_d_ptr(data).data<LibObject<int16_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int16_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(int16_t));
					break;
				}
				if (object->metadata->full_name() == symbols::int32) {
					int32_t *value = get_d_ptr(data).data<LibObject<int32_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int32_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(int32_t));
					break;
				}
				if (object->metadata->full_name() == symbols::int64) {
					int64_t *value = get_d_ptr(data).data<LibObject<int64_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int64_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(int64_t));
					break;
				}
				if (object->metadata->full_name() == symbols::uint8) {
					uint8_t *value = get_d_ptr(data).data<LibObject<uint8_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint8_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(uint8_t));
					break;
				}
				if (object->metadata->full_name() == symbols::uint16) {
					uint16_t *value = get_d_ptr(data).data<LibObject<uint16_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint16_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(uint16_t));
					break;
				}
				if (object->metadata->full_name() == symbols::uint32) {
					uint32_t *value = get_d_ptr(data).data<LibObject<uint32_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint32_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(uint32_t));
					break;
				}
				if (object->metadata->full_name() == symbols::uint64) {
					uint64_t *value = get_d_ptr(data).data<LibObject<uint64_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint64_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(uint64_t));
					break;
				}
				break;

			case Class::STRING:
				data.data<String>()->str = reinterpret_cast<char *>(buffer_data.data());
				buffer_data.erase(buffer_data.begin(), buffer_data.begin() + data.data<String>()->str.size() + 1);
				break;

			case Class::REGEX:
			case Class::ARRAY:
			case Class::HASH:
			case Class::ITERATOR:
			case Class::LIBRARY:
			case Class::LIBOBJECT:
				break;
			}
		}
		break;
	}
}

MINT_FUNCTION(mint_datastream_write, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &data = helper.pop_parameter();
	Reference &buffer = helper.pop_parameter();

	std::vector<uint8_t> &buffer_data = *buffer.data<LibObject<std::vector<uint8_t>>>()->impl;

	switch (data.data()->format) {
	case Data::FMT_NONE:
		break;

	case Data::FMT_NULL:
	case Data::FMT_PACKAGE:
	case Data::FMT_FUNCTION:
		{
			std::string data_str = to_string(data);
			copy_n(data_str.data(), data_str.size(), back_inserter(buffer_data));
			buffer_data.push_back(0);
		}
		break;

	case Data::FMT_NUMBER:
		copy_n(reinterpret_cast<uint8_t *>(&data.data<Number>()->value), sizeof(Number::value),
			   back_inserter(buffer_data));
		break;

	case Data::FMT_BOOLEAN:
		copy_n(reinterpret_cast<uint8_t *>(&data.data<Boolean>()->value), sizeof(Boolean::value),
			   back_inserter(buffer_data));
		break;

	case Data::FMT_OBJECT:
		if (auto *object = data.data<Object>()) {
			switch (object->metadata->metatype()) {
			case Class::OBJECT:
				if (object->metadata->full_name() == symbols::DataStream) {
					std::vector<uint8_t> *other = get_d_ptr(data).data<LibObject<std::vector<uint8_t>>>()->impl;
					copy_n(other->data(), other->size(), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->full_name() == symbols::int8) {
					int8_t *value = get_d_ptr(data).data<LibObject<int8_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(int8_t), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->full_name() == symbols::int16) {
					int16_t *value = get_d_ptr(data).data<LibObject<int16_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(int16_t), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->full_name() == symbols::int32) {
					int32_t *value = get_d_ptr(data).data<LibObject<int32_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(int32_t), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->full_name() == symbols::int64) {
					int64_t *value = get_d_ptr(data).data<LibObject<int64_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(int64_t), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->full_name() == symbols::uint8) {
					uint8_t *value = get_d_ptr(data).data<LibObject<uint8_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(uint8_t), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->full_name() == symbols::uint16) {
					uint16_t *value = get_d_ptr(data).data<LibObject<uint16_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(uint16_t), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->full_name() == symbols::uint32) {
					uint32_t *value = get_d_ptr(data).data<LibObject<uint32_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(uint32_t), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->full_name() == symbols::uint64) {
					uint64_t *value = get_d_ptr(data).data<LibObject<uint64_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(uint64_t), back_inserter(buffer_data));
					break;
				}
				break;

			case Class::STRING:
				copy_n(data.data<String>()->str.data(), data.data<String>()->str.size(), back_inserter(buffer_data));
				buffer_data.push_back(0);
				break;

			case Class::REGEX:
			case Class::ARRAY:
			case Class::HASH:
			case Class::ITERATOR:
			case Class::LIBRARY:
			case Class::LIBOBJECT:
				{
					std::string data_str = to_string(data);
					copy_n(data_str.data(), data_str.size(), back_inserter(buffer_data));
					buffer_data.push_back(0);
				}
				break;
			}
		}
		break;
	}
}

MINT_FUNCTION(mint_datastream_remove, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &count = helper.pop_parameter();
	Reference &buffer = helper.pop_parameter();

	std::vector<uint8_t> *self = buffer.data<LibObject<std::vector<uint8_t>>>()->impl;
	self->erase(self->begin(), self->begin() + to_integer(cursor, count));
}

MINT_FUNCTION(mint_datastream_size, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &buffer = helper.pop_parameter();
	helper.return_value(
		create_number(static_cast<double>(buffer.data<LibObject<std::vector<uint8_t>>>()->impl->size())));
}

MINT_FUNCTION(mint_datastream_empty, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &buffer = helper.pop_parameter();
	helper.return_value(create_boolean(buffer.data<LibObject<std::vector<uint8_t>>>()->impl->empty()));
}
