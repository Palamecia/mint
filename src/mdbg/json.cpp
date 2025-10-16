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

#include "json.h"
#include <cctype>
#include <format>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

std::string JsonArray::to_json() const {
	return std::format("[{}]", std::views::transform(*this,
	                               [](const auto& item) {
		                               return item.to_json();
	                               })
	                               | std::views::join_with(std::string(",")) | std::ranges::to<std::string>());
}

bool JsonObject::has_attribute(const JsonString& attribute) const {
	return contains(attribute);
}

const JsonNull* JsonObject::get_null(const JsonString& attribute) const {
	if (auto it = find(attribute); it != end()) {
		return it->second.if_null();
	}
	return nullptr;
}

const JsonNumber* JsonObject::get_number(const JsonString& attribute) const {
	if (auto it = find(attribute); it != end()) {
		return it->second.if_number();
	}
	return nullptr;
}

const JsonBoolean* JsonObject::get_boolean(const JsonString& attribute) const {
	if (auto it = find(attribute); it != end()) {
		return it->second.if_boolean();
	}
	return nullptr;
}

const JsonString* JsonObject::get_string(const JsonString& attribute) const {
	if (auto it = find(attribute); it != end()) {
		return it->second.if_string();
	}
	return nullptr;
}

const JsonArray* JsonObject::get_array(const JsonString& attribute) const {
	if (auto it = find(attribute); it != end()) {
		return it->second.if_array();
	}
	return nullptr;
}

const JsonObject* JsonObject::get_object(const JsonString& attribute) const {
	if (auto it = find(attribute); it != end()) {
		return it->second.if_object();
	}
	return nullptr;
}

std::string JsonObject::to_json() const {
	return std::format("{{{}}}", std::views::transform(*this,
	                                 [](const auto& item) {
		                                 return item.first.to_json() + ":" + item.second.to_json();
	                                 })
	                                 | std::views::join_with(std::string(",")) | std::ranges::to<std::string>());
}

Json Json::parse(const std::string& data) {
	std::stringstream stream(data);
	while (std::isblank(stream.peek())) {
		stream.get();
	}
	return parse_value(stream);
}

JsonObject Json::parse_object(std::stringstream& stream) {
	auto object = JsonObject();
	while (std::isblank(stream.peek())) {
		stream.get();
	}
	while (stream.peek() != '}') {
		if (stream.get() != '"') {
			throw std::runtime_error("property keys must be doublequoted");
		}
		auto attr = parse_string(stream);
		while (std::isblank(stream.peek())) {
			stream.get();
		}
		if (stream.get() != ':') {
			throw std::runtime_error("colon expected");
		}
		object.emplace(attr, parse_value(stream));
		while (std::isblank(stream.peek())) {
			stream.get();
		}
		if (stream.peek() != '}') {
			if (stream.get() != ',') {
				throw std::runtime_error("expected comma");
			}
			while (std::isblank(stream.peek())) {
				stream.get();
			}
		}
	}
	stream.get();
	return object;
}

JsonArray Json::parse_array(std::stringstream& stream) {
	auto array = JsonArray();
	while (std::isblank(stream.peek())) {
		stream.get();
	}
	while (stream.peek() != ']') {
		array.emplace_back(parse_value(stream));
		while (std::isblank(stream.peek())) {
			stream.get();
		}
		if (stream.peek() != ']') {
			if (stream.get() != ',') {
				throw std::runtime_error("expected comma");
			}
			while (std::isblank(stream.peek())) {
				stream.get();
			}
		}
	}
	stream.get();
	return array;
}

JsonString Json::parse_string(std::stringstream& stream) {
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
				return buffer;
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
				escape = false;
			}
			buffer.push_back(static_cast<char>(c));
			break;
		}
	}
	throw std::runtime_error("unexpected end of string");
}

Json Json::parse_value(std::stringstream& stream) {
	while (std::isblank(stream.peek())) {
		stream.get();
	}
	int c = stream.peek();
	switch (c) {
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
		break;
	}
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
		return JsonNumber(std::stod(buffer));
	}
	std::string buffer;
	while (std::isalpha(c)) {
		buffer.push_back(static_cast<char>(stream.get()));
		c = stream.peek();
	}
	if (buffer == "null") {
		return JsonNull();
	}
	if (buffer == "false") {
		return JsonBoolean(false);
	}
	if (buffer == "true") {
		return JsonBoolean(true);
	}
	throw std::runtime_error("value expected");
}

int Json::json_escape_sequence(int c) {

	static const std::unordered_map<char, char> g_sequences = {
	    {'b', '\b'},
	    {'f', '\f'},
	    {'n', '\n'},
	    {'r', '\r'},
	    {'t', '\t'},
	};

	if (auto it = g_sequences.find(static_cast<char>(c)); it != g_sequences.end()) {
		return it->second;
	}

	throw std::runtime_error("invalid escape character in string");
}
