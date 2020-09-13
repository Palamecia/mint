#include <memory/functiontool.h>
#include <memory/casttool.h>
#include <memory/builtin/string.h>
#include <algorithm>

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_datastream_create_buffer, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	helper.returnValue(create_object(new vector<uint8_t>));
}

MINT_FUNCTION(mint_datastream_delete_buffer, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference buffer = move(helper.popParameter());
	delete buffer->data<LibObject<vector<uint8_t>>>()->impl;
}

MINT_FUNCTION(mint_datastream_contains_int8, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference count = move(helper.popParameter());
	SharedReference buffer = move(helper.popParameter());
	helper.returnValue(create_boolean(buffer->data<LibObject<vector<uint8_t>>>()->impl->size() >= sizeof(int8_t) * to_number(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_int16, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference count = move(helper.popParameter());
	SharedReference buffer = move(helper.popParameter());
	helper.returnValue(create_boolean(buffer->data<LibObject<vector<uint8_t>>>()->impl->size() >= sizeof(int16_t) * to_number(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_int32, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference count = move(helper.popParameter());
	SharedReference buffer = move(helper.popParameter());
	helper.returnValue(create_boolean(buffer->data<LibObject<vector<uint8_t>>>()->impl->size() >= sizeof(int32_t) * to_number(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_int64, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference count = move(helper.popParameter());
	SharedReference buffer = move(helper.popParameter());
	helper.returnValue(create_boolean(buffer->data<LibObject<vector<uint8_t>>>()->impl->size() >= sizeof(int64_t) * to_number(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_uint8, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference count = move(helper.popParameter());
	SharedReference buffer = move(helper.popParameter());
	helper.returnValue(create_boolean(buffer->data<LibObject<vector<uint8_t>>>()->impl->size() >= sizeof(uint8_t) * to_number(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_uint16, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference count = move(helper.popParameter());
	SharedReference buffer = move(helper.popParameter());
	helper.returnValue(create_boolean(buffer->data<LibObject<vector<uint8_t>>>()->impl->size() >= sizeof(uint16_t) * to_number(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_uint32, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference count = move(helper.popParameter());
	SharedReference buffer = move(helper.popParameter());
	helper.returnValue(create_boolean(buffer->data<LibObject<vector<uint8_t>>>()->impl->size() >= sizeof(uint32_t) * to_number(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_uint64, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference count = move(helper.popParameter());
	SharedReference buffer = move(helper.popParameter());
	helper.returnValue(create_boolean(buffer->data<LibObject<vector<uint8_t>>>()->impl->size() >= sizeof(uint64_t) * to_number(cursor, count)));
}

MINT_FUNCTION(mint_datastream_contains_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference buffer = move(helper.popParameter());
	helper.returnValue(create_boolean(buffer->data<LibObject<vector<uint8_t>>>()->impl->size() >= sizeof(Number::value)));
}

MINT_FUNCTION(mint_datastream_contains_boolean, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference buffer = move(helper.popParameter());
	helper.returnValue(create_boolean(buffer->data<LibObject<vector<uint8_t>>>()->impl->size() >= sizeof(Boolean::value)));
}

MINT_FUNCTION(mint_datastream_contains_string, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference buffer = move(helper.popParameter());
	auto begin = buffer->data<LibObject<vector<uint8_t>>>()->impl->begin();
	auto end = buffer->data<LibObject<vector<uint8_t>>>()->impl->end();
	helper.returnValue(create_boolean(find(begin, end, 0) != end));
}

MINT_FUNCTION(mint_datastream_get, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	SharedReference count = move(helper.popParameter());
	SharedReference data = move(helper.popParameter());
	SharedReference buffer = move(helper.popParameter());

	uint8_t *buffer_data = buffer->data<LibObject<vector<uint8_t>>>()->impl->data();

	for (intmax_t index = 0; index < static_cast<intmax_t>(to_number(cursor, count)); ++index) {
		SharedReference item = array_get_item(data->data<Array>(), index);
		switch (item->data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
		case Data::fmt_number:
		case Data::fmt_boolean:
		case Data::fmt_package:
		case Data::fmt_function:
			break;

		case Data::fmt_object:
			if (Object *object = item->data<Object>()) {
				switch (object->metadata->metatype()) {
				case Class::object:
					if (object->metadata->name() == "int8") {
						int8_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int8_t>>()->impl;
						memcpy(value, buffer_data, sizeof(int8_t));
						buffer_data += sizeof (int8_t);
						break;
					}
					if (object->metadata->name() == "int16") {
						int16_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int16_t>>()->impl;
						memcpy(value, buffer_data, sizeof(int16_t));
						buffer_data += sizeof (int16_t);
						break;
					}
					if (object->metadata->name() == "int32") {
						int32_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int32_t>>()->impl;
						memcpy(value, buffer_data, sizeof(int32_t));
						buffer_data += sizeof (int32_t);
						break;
					}
					if (object->metadata->name() == "int64") {
						int64_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int64_t>>()->impl;
						memcpy(value, buffer_data, sizeof(int64_t));
						buffer_data += sizeof (int64_t);
						break;
					}
					if (object->metadata->name() == "uint8") {
						uint8_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint8_t>>()->impl;
						memcpy(value, buffer_data, sizeof(uint8_t));
						buffer_data += sizeof (uint8_t);
						break;
					}
					if (object->metadata->name() == "uint16") {
						uint16_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint16_t>>()->impl;
						memcpy(value, buffer_data, sizeof(uint16_t));
						buffer_data += sizeof (uint16_t);
						break;
					}
					if (object->metadata->name() == "uint32") {
						uint32_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint32_t>>()->impl;
						memcpy(value, buffer_data, sizeof(uint32_t));
						buffer_data += sizeof (uint32_t);
						break;
					}
					if (object->metadata->name() == "uint64") {
						uint64_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint64_t>>()->impl;
						memcpy(value, buffer_data, sizeof(uint64_t));
						buffer_data += sizeof (uint64_t);
						break;
					}
					break;

				case Class::string:
				case Class::regex:
				case Class::array:
				case Class::hash:
				case Class::iterator:
				case Class::library:
				case Class::libobject:
					break;
				}
			}
			break;
		}
	}
}

MINT_FUNCTION(mint_datastream_get, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference data = move(helper.popParameter());
	SharedReference buffer = move(helper.popParameter());

	vector<uint8_t> &buffer_data = *buffer->data<LibObject<vector<uint8_t>>>()->impl;

	switch (data->data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
	case Data::fmt_package:
	case Data::fmt_function:
		break;

	case Data::fmt_number:
		memcpy(&data->data<Number>()->value, buffer_data.data(), sizeof(Number::value));
		break;

	case Data::fmt_boolean:
		memcpy(&data->data<Boolean>()->value, buffer_data.data(), sizeof(Boolean::value));
		break;

	case Data::fmt_object:
		if (Object *object = data->data<Object>()) {
			switch (object->metadata->metatype()) {
			case Class::object:
				if (object->metadata->name() == "int8") {
					int8_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int8_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int8_t));
					break;
				}
				if (object->metadata->name() == "int16") {
					int16_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int16_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int16_t));
					break;
				}
				if (object->metadata->name() == "int32") {
					int32_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int32_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int32_t));
					break;
				}
				if (object->metadata->name() == "int64") {
					int64_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int64_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int64_t));
					break;
				}
				if (object->metadata->name() == "uint8") {
					uint8_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint8_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint8_t));
					break;
				}
				if (object->metadata->name() == "uint16") {
					uint16_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint16_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint16_t));
					break;
				}
				if (object->metadata->name() == "uint32") {
					uint32_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint32_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint32_t));
					break;
				}
				if (object->metadata->name() == "uint64") {
					uint64_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint64_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint64_t));
					break;
				}
				break;

			case Class::string:
				data->data<String>()->str = reinterpret_cast<char *>(buffer_data.data());
				break;

			case Class::regex:
			case Class::array:
			case Class::hash:
			case Class::iterator:
			case Class::library:
			case Class::libobject:
				break;
			}
		}
		break;
	}
}

MINT_FUNCTION(mint_datastream_read, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference data = move(helper.popParameter());
	SharedReference buffer = move(helper.popParameter());

	vector<uint8_t> &buffer_data = *buffer->data<LibObject<vector<uint8_t>>>()->impl;

	switch (data->data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
	case Data::fmt_package:
	case Data::fmt_function:
		break;

	case Data::fmt_number:
		memcpy(&data->data<Number>()->value, buffer_data.data(), sizeof(Number::value));
		buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(Number::value));
		break;

	case Data::fmt_boolean:
		memcpy(&data->data<Boolean>()->value, buffer_data.data(), sizeof(Boolean::value));
		buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(Boolean::value));
		break;

	case Data::fmt_object:
		if (Object *object = data->data<Object>()) {
			switch (object->metadata->metatype()) {
			case Class::object:
				if (object->metadata->name() == "int8") {
					int8_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int8_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int8_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(int8_t));
					break;
				}
				if (object->metadata->name() == "int16") {
					int16_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int16_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int16_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(int16_t));
					break;
				}
				if (object->metadata->name() == "int32") {
					int32_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int32_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int32_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(int32_t));
					break;
				}
				if (object->metadata->name() == "int64") {
					int64_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int64_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(int64_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(int64_t));
					break;
				}
				if (object->metadata->name() == "uint8") {
					uint8_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint8_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint8_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(uint8_t));
					break;
				}
				if (object->metadata->name() == "uint16") {
					uint16_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint16_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint16_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(uint16_t));
					break;
				}
				if (object->metadata->name() == "uint32") {
					uint32_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint32_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint32_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(uint32_t));
					break;
				}
				if (object->metadata->name() == "uint64") {
					uint64_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint64_t>>()->impl;
					memcpy(value, buffer_data.data(), sizeof(uint64_t));
					buffer_data.erase(buffer_data.begin(), buffer_data.begin() + sizeof(uint64_t));
					break;
				}
				break;

			case Class::string:
				data->data<String>()->str = reinterpret_cast<char *>(buffer_data.data());
				buffer_data.erase(buffer_data.begin(), buffer_data.begin() + data->data<String>()->str.size() + 1);
				break;

			case Class::regex:
			case Class::array:
			case Class::hash:
			case Class::iterator:
			case Class::library:
			case Class::libobject:
				break;
			}
		}
		break;
	}
}

MINT_FUNCTION(mint_datastream_write, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference data = move(helper.popParameter());
	SharedReference buffer = move(helper.popParameter());

	vector<uint8_t> &buffer_data = *buffer->data<LibObject<vector<uint8_t>>>()->impl;

	switch (data->data()->format) {
	case Data::fmt_none:
		break;

	case Data::fmt_null:
	case Data::fmt_package:
	case Data::fmt_function:
		copy_n(to_string(data).data(), to_string(data).size(), back_inserter(buffer_data));
		buffer_data.push_back(0);
		break;

	case Data::fmt_number:
		copy_n(reinterpret_cast<uint8_t *>(&data->data<Number>()->value), sizeof(Number::value), back_inserter(buffer_data));
		break;

	case Data::fmt_boolean:
		copy_n(reinterpret_cast<uint8_t *>(&data->data<Boolean>()->value), sizeof(Boolean::value), back_inserter(buffer_data));
		break;

	case Data::fmt_object:
		if (Object *object = data->data<Object>()) {
			switch (object->metadata->metatype()) {
			case Class::object:
				if (object->metadata->name() == "DataStream") {
					vector<uint8_t> *other = object->data[object->metadata->members()["buffer"]->offset].data<LibObject<vector<uint8_t>>>()->impl;
					copy_n(other->data(), other->size(), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->name() == "int8") {
					int8_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int8_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(int8_t), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->name() == "int16") {
					int16_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int16_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(int16_t), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->name() == "int32") {
					int32_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int32_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(int32_t), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->name() == "int64") {
					int64_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<int64_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(int64_t), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->name() == "uint8") {
					uint8_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint8_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(uint8_t), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->name() == "uint16") {
					uint16_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint16_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(uint16_t), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->name() == "uint32") {
					uint32_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint32_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(uint32_t), back_inserter(buffer_data));
					break;
				}
				if (object->metadata->name() == "uint64") {
					uint64_t *value = object->data[object->metadata->members()["value"]->offset].data<LibObject<uint64_t>>()->impl;
					copy_n(reinterpret_cast<uint8_t *>(value), sizeof(uint64_t), back_inserter(buffer_data));
					break;
				}
				break;

			case Class::string:
				copy_n(data->data<String>()->str.data(), data->data<String>()->str.size(), back_inserter(buffer_data));
				buffer_data.push_back(0);
				break;

			case Class::regex:
			case Class::array:
			case Class::hash:
			case Class::iterator:
			case Class::library:
			case Class::libobject:
				copy_n(to_string(data).data(), to_string(data).size(), back_inserter(buffer_data));
				buffer_data.push_back(0);
				break;
			}
		}
		break;
	}
}

MINT_FUNCTION(mint_datastream_remove, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference count = move(helper.popParameter());
	SharedReference buffer = move(helper.popParameter());

	vector<uint8_t> *self = buffer->data<LibObject<vector<uint8_t>>>()->impl;
	self->erase(self->begin(), self->begin() + static_cast<uintmax_t>(to_number(cursor, count)));
}

MINT_FUNCTION(mint_datastream_size, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference buffer = move(helper.popParameter());
	helper.returnValue(create_number(static_cast<double>(buffer->data<LibObject<vector<uint8_t>>>()->impl->size())));
}

MINT_FUNCTION(mint_datastream_empty, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference buffer = move(helper.popParameter());
	helper.returnValue(create_boolean(buffer->data<LibObject<vector<uint8_t>>>()->impl->empty()));
}
