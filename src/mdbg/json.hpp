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

#ifndef MDBG_JSON_HPP
#define MDBG_JSON_HPP

#include <mint/system/string.h>

#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

class JsonNull;
class JsonObject;
class JsonArray;
class JsonNumber;
class JsonString;
class JsonBoolean;

class Json {
public:
	enum Type : std::uint8_t {
		NULL_TYPE,
		OBJECT_TYPE,
		ARRAY_TYPE,
		NUMBER_TYPE,
		STRING_TYPE,
		BOOLEAN_TYPE
	};

	Json() = default;
	Json(const Json &) = default;
	Json(Json &&) = delete;
	virtual ~Json() = default;

	Json &operator=(const Json &) = default;
	Json &operator=(Json &&) = delete;

	static inline std::unique_ptr<Json> parse(const std::string &data);

	static std::unique_ptr<JsonObject> parse_object(const std::string &data) {
		std::unique_ptr<Json> json = parse(data);
		if (json->is_object()) {
			return std::unique_ptr<JsonObject>(json.release()->to_object());
		}
		return nullptr;
	}

	static std::unique_ptr<JsonArray> parse_array(const std::string &data) {
		std::unique_ptr<Json> json = parse(data);
		if (json->is_array()) {
			return std::unique_ptr<JsonArray>(json.release()->to_array());
		}
		return nullptr;
	}

	[[nodiscard]] bool is_null() const {
		return type() == NULL_TYPE;
	}

	[[nodiscard]] const JsonNull *to_null() const {
		return is_null() ? reinterpret_cast<const JsonNull *>(this) : nullptr;
	}

	[[nodiscard]] JsonNull *to_null() {
		return is_null() ? reinterpret_cast<JsonNull *>(this) : nullptr;
	}

	[[nodiscard]] bool is_object() const {
		return type() == OBJECT_TYPE;
	}

	[[nodiscard]] const JsonObject *to_object() const {
		return is_object() ? reinterpret_cast<const JsonObject *>(this) : nullptr;
	}

	[[nodiscard]] JsonObject *to_object() {
		return is_object() ? reinterpret_cast<JsonObject *>(this) : nullptr;
	}

	[[nodiscard]] bool is_array() const {
		return type() == ARRAY_TYPE;
	}

	[[nodiscard]] const JsonArray *to_array() const {
		return is_array() ? reinterpret_cast<const JsonArray *>(this) : nullptr;
	}

	[[nodiscard]] JsonArray *to_array() {
		return is_array() ? reinterpret_cast<JsonArray *>(this) : nullptr;
	}

	[[nodiscard]] bool is_number() const {
		return type() == NUMBER_TYPE;
	}

	[[nodiscard]] const JsonNumber *to_number() const {
		return is_number() ? reinterpret_cast<const JsonNumber *>(this) : nullptr;
	}

	[[nodiscard]] JsonNumber *to_number() {
		return is_number() ? reinterpret_cast<JsonNumber *>(this) : nullptr;
	}

	[[nodiscard]] bool is_string() const {
		return type() == STRING_TYPE;
	}

	[[nodiscard]] const JsonString *to_string() const {
		return is_string() ? reinterpret_cast<const JsonString *>(this) : nullptr;
	}

	[[nodiscard]] JsonString *to_string() {
		return is_string() ? reinterpret_cast<JsonString *>(this) : nullptr;
	}

	[[nodiscard]] bool is_boolean() const {
		return type() == BOOLEAN_TYPE;
	}

	[[nodiscard]] const JsonBoolean *to_boolean() const {
		return is_boolean() ? reinterpret_cast<const JsonBoolean *>(this) : nullptr;
	}

	[[nodiscard]] JsonBoolean *to_boolean() {
		return is_boolean() ? reinterpret_cast<JsonBoolean *>(this) : nullptr;
	}

	[[nodiscard]] virtual Type type() const = 0;
	[[nodiscard]] virtual std::string to_json() const = 0;
	[[nodiscard]] virtual Json *clone() const = 0;

private:
	static inline JsonObject *parse_object(std::stringstream &stream);
	static inline JsonArray *parse_array(std::stringstream &stream);
	static inline JsonString *parse_string(std::stringstream &stream);
	static inline Json *parse_value(std::stringstream &stream);

	static inline int json_escape_sequence(int c);
};

class JsonNull : public Json {
public:
	JsonNull() = default;

	[[nodiscard]] Type type() const override {
		return NULL_TYPE;
	}

	[[nodiscard]] std::string to_json() const override {
		return "null";
	}

	[[nodiscard]] Json *clone() const override {
		return new JsonNull;
	}
};

class JsonNumber : public Json {
public:
	JsonNumber(const JsonNumber &other) = default;
	JsonNumber(JsonNumber &&) = delete;
	~JsonNumber() override = default;

	JsonNumber(int value) :
		m_value(static_cast<double>(value)) {}

	JsonNumber(size_t value) :
		m_value(static_cast<double>(value)) {}

	JsonNumber(double value) :
		m_value(value) {}

	JsonNumber &operator=(const JsonNumber &other) = default;

	JsonNumber &operator=(int value) {
		m_value = static_cast<double>(value);
		return *this;
	}

	JsonNumber &operator=(size_t value) {
		m_value = static_cast<double>(value);
		return *this;
	}

	JsonNumber &operator=(double value) {
		m_value = value;
		return *this;
	}

	JsonNumber &operator=(JsonNumber &&) = delete;

	operator int() const {
		return static_cast<int>(m_value);
	}

	operator size_t() const {
		return static_cast<size_t>(m_value);
	}

	operator double() const {
		return m_value;
	}

	[[nodiscard]] Type type() const override {
		return NUMBER_TYPE;
	}

	[[nodiscard]] std::string to_json() const override {
		return mint::to_string(m_value);
	}

	[[nodiscard]] Json *clone() const override {
		return new JsonNumber(*this);
	}

private:
	double m_value;
};

class JsonString : public Json, public std::string {
public:
	JsonString() = default;
	JsonString(const JsonString &other) = default;
	JsonString(JsonString &&) = delete;
	~JsonString() override = default;

	JsonString(const std::string &string) :
		std::string(string) {}

	using std::string::basic_string;

	JsonString &operator=(const JsonString &other) = default;
	JsonString &operator=(JsonString &&) = delete;
	using std::string::operator=;

	[[nodiscard]] Type type() const override {
		return STRING_TYPE;
	}

	[[nodiscard]] std::string to_json() const override {
		return "\"" + escape(*this) + "\"";
	}

	[[nodiscard]] Json *clone() const override {
		return new JsonString(*this);
	}

private:
	static std::string escape(const std::string &str) {
		std::stringstream stream;
		for (char ch : str) {
			switch (ch) {
			case '\\':
				stream << "\\\\";
				break;
			case '"':
				stream << "\\\"";
				break;
			case '\n':
				stream << "\\n";
				break;
			default:
				stream << ch;
			}
		}
		return stream.str();
	}
};

template<>
struct std::hash<JsonString> {
	std::size_t operator()(const JsonString &string) const noexcept {
		return std::hash<std::string>()(string);
	}
};

class JsonBoolean : public Json {
public:
	JsonBoolean(const JsonBoolean &other) = default;
	JsonBoolean(JsonBoolean &&) = delete;
	~JsonBoolean() override = default;

	JsonBoolean(bool value) :
		m_value(value) {}

	JsonBoolean &operator=(const JsonBoolean &other) = default;
	JsonBoolean &operator=(JsonBoolean &&) = delete;

	JsonBoolean &operator=(bool value) {
		m_value = value;
		return *this;
	}

	operator bool() const {
		return m_value;
	}

	[[nodiscard]] Type type() const override {
		return BOOLEAN_TYPE;
	}

	[[nodiscard]] std::string to_json() const override {
		return m_value ? "true" : "false";
	}

	[[nodiscard]] Json *clone() const override {
		return new JsonBoolean(*this);
	}

private:
	bool m_value;
};

class JsonObject : public Json, public std::unordered_map<JsonString, Json *> {
public:
	JsonObject() = default;
	JsonObject(JsonObject &&) = delete;

	JsonObject(const std::unordered_map<JsonString, Json *> &object) :
		std::unordered_map<JsonString, Json *>(object) {}

	using std::unordered_map<JsonString, Json *>::unordered_map;

	JsonObject(const JsonObject &other) :
		std::unordered_map<JsonString, Json *>(other.size()) {
		for (const auto &attr : other) {
			emplace(attr.first, attr.second->clone());
		}
	}

	~JsonObject() {
		std::for_each(begin(), end(), [](value_type &attr) {
			delete attr.second;
		});
	}

	JsonObject &operator=(const JsonObject &) = default;
	JsonObject &operator=(JsonObject &&) = delete;

	[[nodiscard]] Type type() const override {
		return OBJECT_TYPE;
	}

	[[nodiscard]] std::string to_json() const override {
		std::stringstream stream;
		stream << "{";
		for (auto it = begin(); it != end(); ++it) {
			if (it != begin()) {
				stream << ",";
			}
			stream << it->first.to_json() << ":" << it->second->to_json();
		}
		stream << "}";
		return stream.str();
	}

	[[nodiscard]] Json *clone() const override {
		return new JsonObject(*this);
	}

	[[nodiscard]] bool has_attribute(const JsonString &attribute) const {
		return find(attribute) != end();
	}

	[[nodiscard]] const JsonNull *get_null(const JsonString &attribute) const {
		auto it = find(attribute);
		if (it != end()) {
			return it->second->to_null();
		}
		return nullptr;
	}

	[[nodiscard]] const JsonObject *get_object(const JsonString &attribute) const {
		auto it = find(attribute);
		if (it != end()) {
			return it->second->to_object();
		}
		return nullptr;
	}

	[[nodiscard]] const JsonArray *get_array(const JsonString &attribute) const {
		auto it = find(attribute);
		if (it != end()) {
			return it->second->to_array();
		}
		return nullptr;
	}

	[[nodiscard]] const JsonNumber *get_number(const JsonString &attribute) const {
		auto it = find(attribute);
		if (it != end()) {
			return it->second->to_number();
		}
		return nullptr;
	}

	[[nodiscard]] const JsonString *get_string(const JsonString &attribute) const {
		auto it = find(attribute);
		if (it != end()) {
			return it->second->to_string();
		}
		return nullptr;
	}

	[[nodiscard]] const JsonBoolean *get_boolean(const JsonString &attribute) const {
		auto it = find(attribute);
		if (it != end()) {
			return it->second->to_boolean();
		}
		return nullptr;
	}
};

class JsonArray : public Json, public std::vector<Json *> {
public:
	JsonArray() = default;
	JsonArray(JsonArray &&) = delete;

	JsonArray(const std::vector<Json *> &array) :
		std::vector<Json *>(array) {}

	using std::vector<Json *>::vector;

	JsonArray(const JsonArray &other) :
		std::vector<Json *>({}) {
		std::vector<Json *>::reserve(other.size());
		for (auto *item : other) {
			emplace_back(item->clone());
		}
	}

	~JsonArray() {
		std::for_each(begin(), end(), [](value_type &item) {
			delete item;
		});
	}

	JsonArray &operator=(const JsonArray &) = default;
	JsonArray &operator=(JsonArray &&) = delete;

	[[nodiscard]] Type type() const override {
		return ARRAY_TYPE;
	}

	[[nodiscard]] std::string to_json() const override {
		std::stringstream stream;
		stream << "[";
		for (auto it = begin(); it != end(); ++it) {
			if (it != begin()) {
				stream << ",";
			}
			stream << (*it)->to_json();
		}
		stream << "]";
		return stream.str();
	}

	[[nodiscard]] Json *clone() const override {
		return new JsonArray(*this);
	}
};

std::unique_ptr<Json> Json::parse(const std::string &data) {
	std::stringstream stream(data);
	while (std::isblank(stream.peek())) {
		stream.get();
	}
	switch (stream.get()) {
	case '{':
		return std::unique_ptr<Json>(parse_object(stream));
	case ']':
		return std::unique_ptr<Json>(parse_array(stream));
	default:
		break;
	}
	return nullptr;
}

JsonObject *Json::parse_object(std::stringstream &stream) {
	std::unique_ptr<JsonObject> object(new JsonObject);
	while (std::isblank(stream.peek())) {
		stream.get();
	}
	while (stream.peek() != '}') {
		if (stream.get() != '"') {
			return nullptr;
		}
		std::unique_ptr<JsonString> attr(parse_string(stream));
		if (attr == nullptr) {
			return nullptr;
		}
		while (std::isblank(stream.peek())) {
			stream.get();
		}
		if (stream.get() != ':') {
			return nullptr;
		}
		Json *value = parse_value(stream);
		if (value == nullptr) {
			return nullptr;
		}
		object->emplace(*attr, value);
		while (std::isblank(stream.peek())) {
			stream.get();
		}
		if (stream.peek() != '}') {
			if (stream.get() != ',') {
				return nullptr;
			}
			while (std::isblank(stream.peek())) {
				stream.get();
			}
		}
	}
	stream.get();
	return object.release();
}

JsonArray *Json::parse_array(std::stringstream &stream) {
	std::unique_ptr<JsonArray> array(new JsonArray);
	while (std::isblank(stream.peek())) {
		stream.get();
	}
	while (stream.peek() != ']') {
		Json *value = parse_value(stream);
		if (value == nullptr) {
			return nullptr;
		}
		array->emplace_back(value);
		while (std::isblank(stream.peek())) {
			stream.get();
		}
		if (stream.peek() != ']') {
			if (stream.get() != ',') {
				return nullptr;
			}
			while (std::isblank(stream.peek())) {
				stream.get();
			}
		}
	}
	stream.get();
	return array.release();
}

JsonString *Json::parse_string(std::stringstream &stream) {
	std::string buffer;
	bool escape = false;
	while (!stream.eof()) {
		switch (int c = stream.get()) {
		case '"':
			if (escape) {
				buffer.push_back(static_cast<char>(c));
				escape = false;
			}
			else {
				return new JsonString(buffer);
			}
			break;
		case '\\':
			if (escape) {
				buffer.push_back(static_cast<char>(c));
				escape = false;
			}
			else {
				escape = true;
			}
			break;
		default:
			if (escape) {
				c = json_escape_sequence(c);
				if (c == EOF) {
					return nullptr;
				}
				escape = false;
			}
			buffer.push_back(static_cast<char>(c));
			break;
		}
	}
	return nullptr;
}

Json *Json::parse_value(std::stringstream &stream) {
	while (std::isblank(stream.peek())) {
		stream.get();
	}
	switch (int c = stream.peek()) {
	case '{':
		stream.get();
		return parse_object(stream);
	case '[':
		stream.get();
		return parse_array(stream);
	case '"':
		stream.get();
		return parse_string(stream);
	default:
		if (std::isdigit(c)) {
			std::string buffer;
			while (std::isdigit(c)) {
				buffer += static_cast<char>(stream.get());
				c = stream.peek();
			}
			if (c == '.') {
				buffer += static_cast<char>(stream.get());
				c = stream.peek();
				while (std::isdigit(c)) {
					buffer += static_cast<char>(stream.get());
					c = stream.peek();
				}
			}
			return new JsonNumber(stod(buffer));
		}
		else {
			std::string buffer;
			while (std::isalpha(c)) {
				buffer.push_back(static_cast<char>(stream.get()));
				c = stream.peek();
			}
			if (buffer == "null") {
				return new JsonNull();
			}
			if (buffer == "false") {
				return new JsonBoolean(false);
			}
			if (buffer == "true") {
				return new JsonBoolean(true);
			}
		}
		break;
	}
	return nullptr;
}

int Json::json_escape_sequence(int c) {

	static const std::unordered_map<char, char> g_sequences = {
		{'b', '\b'}, {'f', '\f'}, {'n', '\n'}, {'r', '\r'}, {'t', '\t'},
	};

	auto it = g_sequences.find(static_cast<char>(c));
	if (it != g_sequences.end()) {
		return it->second;
	}
	return EOF;
}

inline int attribute_or_default(const JsonNumber *attr, int default_value) {
	return attr ? static_cast<int>(*attr) : default_value;
}

inline size_t attribute_or_default(const JsonNumber *attr, size_t default_value) {
	return attr ? static_cast<size_t>(*attr) : default_value;
}

inline JsonObject *attribute_or_default(const JsonObject *attr, JsonObject *default_value) {
	return attr ? new JsonObject(*attr) : default_value;
}

#endif // MDBG_JSON_HPP
