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

#ifndef MDBG_JSON_H
#define MDBG_JSON_H

#include "mint/system/string.h"
#include "utils.h"

#include <cctype>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <typeinfo>
#include <unordered_map>
#include <filesystem>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <cstdint>

class JsonNull {
public:
	JsonNull() = default;

	[[nodiscard]] std::string to_json() const {
		return "null";
	}
};

class JsonNumber {
public:
	JsonNumber(const JsonNumber&) = default;
	JsonNumber(JsonNumber&&) = default;
	~JsonNumber() = default;

	JsonNumber(int value) :
	    _value(static_cast<double>(value)) {}

	JsonNumber(std::size_t value) :
	    _value(static_cast<double>(value)) {}

	JsonNumber(double value) :
	    _value(value) {}

	JsonNumber& operator=(const JsonNumber&) = default;
	JsonNumber& operator=(JsonNumber&&) = default;

	JsonNumber& operator=(int value) {
		_value = static_cast<double>(value);
		return *this;
	}

	JsonNumber& operator=(std::size_t value) {
		_value = static_cast<double>(value);
		return *this;
	}

	JsonNumber& operator=(double value) {
		_value = value;
		return *this;
	}

	operator int() const {
		return static_cast<int>(_value);
	}

	operator std::size_t() const {
		return static_cast<std::size_t>(_value);
	}

	operator double() const {
		return _value;
	}

	[[nodiscard]] std::string to_json() const {
		return mint::to_string(_value);
	}

private:
	double _value;
};

class JsonBoolean {
public:
	JsonBoolean(const JsonBoolean& other) = default;
	JsonBoolean(JsonBoolean&&) = default;
	~JsonBoolean() = default;

	JsonBoolean(bool value) :
	    _value(value) {}

	JsonBoolean& operator=(const JsonBoolean& other) = default;
	JsonBoolean& operator=(JsonBoolean&&) = default;

	JsonBoolean& operator=(bool value) {
		_value = value;
		return *this;
	}

	operator bool() const {
		return _value;
	}

	[[nodiscard]] std::string to_json() const {
		return _value ? "true" : "false";
	}

private:
	bool _value;
};

class JsonString : public std::string {
public:
	JsonString() = default;
	JsonString(const JsonString&) = default;
	JsonString(JsonString&&) = default;
	~JsonString() = default;

	JsonString(const std::string& other) :
	    std::string(other) {}

	JsonString(std::string&& other) noexcept :
	    std::string(std::move(other)) {}

	using std::string::string;

	JsonString& operator=(const JsonString&) = default;
	JsonString& operator=(JsonString&&) = default;

	operator std::filesystem::path() const {
		return {static_cast<std::string>(*this)};
	}

	[[nodiscard]] std::string to_json() const {
		return "\"" + escape(*this) + "\"";
	}

private:
	static std::string escape(const std::string& str) {
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
	std::size_t operator()(const JsonString& string) const noexcept {
		return std::hash<std::string>()(string);
	}
};

class Json;

class JsonArray : public std::vector<Json> {
public:
	JsonArray() = default;
	JsonArray(const JsonArray&) = default;
	JsonArray(JsonArray&&) = default;
	~JsonArray() = default;

	using std::vector<Json>::vector;

	JsonArray& operator=(const JsonArray&) = default;
	JsonArray& operator=(JsonArray&&) = default;

	[[nodiscard]] std::string to_json() const;
};

class JsonObject : public std::unordered_map<JsonString, Json> {
public:
	JsonObject() = default;
	JsonObject(const JsonObject&) = default;
	JsonObject(JsonObject&&) = default;
	~JsonObject() = default;

	JsonObject(const std::unordered_map<JsonString, Json>& other) :
	    std::unordered_map<JsonString, Json>(other) {}

	JsonObject(std::unordered_map<JsonString, Json>&& other) noexcept :
	    std::unordered_map<JsonString, Json>(std::move(other)) {}

	using std::unordered_map<JsonString, Json>::unordered_map;

	JsonObject& operator=(const JsonObject&) = default;
	JsonObject& operator=(JsonObject&&) = default;

	[[nodiscard]] std::string to_json() const;

	[[nodiscard]] bool has_attribute(const JsonString& attribute) const;
	[[nodiscard]] const JsonNull* get_null(const JsonString& attribute) const;
	[[nodiscard]] const JsonNumber* get_number(const JsonString& attribute) const;
	[[nodiscard]] const JsonBoolean* get_boolean(const JsonString& attribute) const;
	[[nodiscard]] const JsonString* get_string(const JsonString& attribute) const;
	[[nodiscard]] const JsonArray* get_array(const JsonString& attribute) const;
	[[nodiscard]] const JsonObject* get_object(const JsonString& attribute) const;
};

class Json {
	std::variant<JsonNull, JsonNumber, JsonBoolean, JsonString, std::unique_ptr<JsonArray>, std::unique_ptr<JsonObject>>
	    _value;
public:
	enum Type : std::uint8_t {
		null_type,
		number_type,
		boolean_type,
		string_type,
		array_type,
		object_type,
	};

	Json() = default;
	Json(Json&&) = default;
	~Json() = default;

	Json(const Json& other) :
	    _value(std::visit(Overloaded {
	                          [](const JsonNull& value) -> decltype(_value) {
		                          return value;
	                          },
	                          [](const JsonNumber& value) -> decltype(_value) {
		                          return value;
	                          },
	                          [](const JsonBoolean& value) -> decltype(_value) {
		                          return value;
	                          },
	                          [](const JsonString& value) -> decltype(_value) {
		                          return value;
	                          },
	                          [](const std::unique_ptr<JsonArray>& value) -> decltype(_value) {
		                          return std::make_unique<JsonArray>(*value);
	                          },
	                          [](const std::unique_ptr<JsonObject>& value) -> decltype(_value) {
		                          return std::make_unique<JsonObject>(*value);
	                          },
	                      },
	        other._value)) {}

	Json(const JsonNull& value) :
	    _value(value) {}

	Json(const JsonNumber& value) :
	    _value(value) {}

	Json(const JsonBoolean& value) :
	    _value(value) {}

	Json(const JsonString& value) :
	    _value(value) {}

	Json(const JsonArray& value) :
	    _value(std::make_unique<JsonArray>(value)) {}

	Json(const JsonObject& value) :
	    _value(std::make_unique<JsonObject>(value)) {}

	Json& operator=(Json&&) = default;

	Json& operator=(const Json& other) {
		if (this == &other) [[unlikely]] {
			return *this;
		}
		_value = std::visit(Overloaded {
		                        [](const JsonNull& value) -> decltype(_value) {
			                        return value;
		                        },
		                        [](const JsonNumber& value) -> decltype(_value) {
			                        return value;
		                        },
		                        [](const JsonBoolean& value) -> decltype(_value) {
			                        return value;
		                        },
		                        [](const JsonString& value) -> decltype(_value) {
			                        return value;
		                        },
		                        [](const std::unique_ptr<JsonArray>& value) -> decltype(_value) {
			                        return std::make_unique<JsonArray>(*value);
		                        },
		                        [](const std::unique_ptr<JsonObject>& value) -> decltype(_value) {
			                        return std::make_unique<JsonObject>(*value);
		                        },
		                    },
		    other._value);
		return *this;
	}

	Json& operator=(const JsonNull& value) {
		_value = value;
		return *this;
	}

	Json& operator=(const JsonNumber& value) {
		_value = value;
		return *this;
	}

	Json& operator=(const JsonBoolean& value) {
		_value = value;
		return *this;
	}

	Json& operator=(const JsonString& value) {
		_value = value;
		return *this;
	}

	Json& operator=(const JsonArray& value) {
		_value = std::make_unique<JsonArray>(value);
		return *this;
	}

	Json& operator=(const JsonObject& value) {
		_value = std::make_unique<JsonObject>(value);
		return *this;
	}

	static inline Json parse(const std::string& data);
	friend inline JsonObject parse_json_object(const std::string& data);
	friend inline JsonArray parse_json_array(const std::string& data);

	[[nodiscard]] Type type() const {
		return std::visit(Overloaded {
		                      [](const JsonNull&) {
			                      return Json::null_type;
		                      },
		                      [](const JsonNumber&) {
			                      return Json::number_type;
		                      },
		                      [](const JsonBoolean&) {
			                      return Json::boolean_type;
		                      },
		                      [](const JsonString&) {
			                      return Json::string_type;
		                      },
		                      [](const std::unique_ptr<JsonArray>&) {
			                      return Json::array_type;
		                      },
		                      [](const std::unique_ptr<JsonObject>&) {
			                      return Json::object_type;
		                      },
		                  },
		    _value);
	}

	[[nodiscard]] bool is_null() const {
		return type() == null_type;
	}

	[[nodiscard]] const JsonNull* if_null() const {
		return std::get_if<JsonNull>(&_value);
	}

	[[nodiscard]] JsonNull* if_null() {
		return std::get_if<JsonNull>(&_value);
	}

	[[nodiscard]] bool is_number() const {
		return type() == number_type;
	}

	[[nodiscard]] const JsonNumber* if_number() const {
		return std::get_if<JsonNumber>(&_value);
	}

	[[nodiscard]] JsonNumber* if_number() {
		return std::get_if<JsonNumber>(&_value);
	}

	[[nodiscard]] bool is_boolean() const {
		return type() == boolean_type;
	}

	[[nodiscard]] const JsonBoolean* if_boolean() const {
		return std::get_if<JsonBoolean>(&_value);
	}

	[[nodiscard]] JsonBoolean* if_boolean() {
		return std::get_if<JsonBoolean>(&_value);
	}

	[[nodiscard]] bool is_string() const {
		return type() == string_type;
	}

	[[nodiscard]] const JsonString* if_string() const {
		return std::get_if<JsonString>(&_value);
	}

	[[nodiscard]] JsonString* if_string() {
		return std::get_if<JsonString>(&_value);
	}

	[[nodiscard]] bool is_array() const {
		return type() == array_type;
	}

	[[nodiscard]] const JsonArray* if_array() const {
		if (const auto* value = std::get_if<std::unique_ptr<JsonArray>>(&_value)) {
			return value->get();
		}
		return nullptr;
	}

	[[nodiscard]] JsonArray* if_array() {
		if (auto* value = std::get_if<std::unique_ptr<JsonArray>>(&_value)) {
			return value->get();
		}
		return nullptr;
	}

	[[nodiscard]] bool is_object() const {
		return type() == object_type;
	}

	[[nodiscard]] const JsonObject* if_object() const {
		if (const auto* value = std::get_if<std::unique_ptr<JsonObject>>(&_value)) {
			return value->get();
		}
		return nullptr;
	}

	[[nodiscard]] JsonObject* if_object() {
		if (auto* value = std::get_if<std::unique_ptr<JsonObject>>(&_value)) {
			return value->get();
		}
		return nullptr;
	}

	[[nodiscard]] std::string to_json() const {
		return std::visit(Overloaded {
		                      [](const JsonNull& value) {
			                      return value.to_json();
		                      },
		                      [](const JsonNumber& value) {
			                      return value.to_json();
		                      },
		                      [](const JsonBoolean& value) {
			                      return value.to_json();
		                      },
		                      [](const JsonString& value) {
			                      return value.to_json();
		                      },
		                      [](const std::unique_ptr<JsonArray>& value) {
			                      return value->to_json();
		                      },
		                      [](const std::unique_ptr<JsonObject>& value) {
			                      return value->to_json();
		                      },
		                  },
		    _value);
	}

private:
	static JsonObject parse_object(std::stringstream& stream);
	static JsonArray parse_array(std::stringstream& stream);
	static JsonString parse_string(std::stringstream& stream);
	static Json parse_value(std::stringstream& stream);

	static int json_escape_sequence(int c);
};

inline JsonObject parse_json_object(const std::string& data) {
	std::stringstream stream(data);
	while (std::isblank(stream.peek())) {
		stream.get();
	}
	if (stream.peek() == '{') {
		stream.get();
		return Json::parse_object(stream);
	}
	throw std::bad_cast();
}

inline JsonArray parse_json_array(const std::string& data) {
	std::stringstream stream(data);
	while (std::isblank(stream.peek())) {
		stream.get();
	}
	if (stream.peek() == '[') {
		stream.get();
		return Json::parse_array(stream);
	}
	throw std::bad_cast();
}

inline std::optional<const JsonObject> attribute_as_optional(const JsonObject* attr) {
	return attr ? std::optional<JsonObject>(*attr) : std::nullopt;
}

inline std::optional<JsonObject> attribute_as_optional(JsonObject* attr) {
	return attr ? std::optional<JsonObject>(*attr) : std::nullopt;
}

inline int attribute_or_default(const JsonNumber* attr, int default_value = {}) {
	return attr ? static_cast<int>(*attr) : default_value;
}

inline std::size_t attribute_or_default(const JsonNumber* attr, std::size_t default_value = {}) {
	return attr ? static_cast<std::size_t>(*attr) : default_value;
}

inline std::string attribute_or_default(const JsonString* attr, const std::string& default_value = {}) {
	return attr ? *attr : default_value;
}

inline JsonObject attribute_or_default(const JsonObject* attr, const JsonObject& default_value = {}) {
	return attr ? *attr : default_value;
}

#endif // MDBG_JSON_H
