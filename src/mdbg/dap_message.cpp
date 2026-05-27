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

#include "dap_message.h"
#include "json.h"
#include "log.h"

#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <print>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace {

std::string::size_type regex_find(const std::string& str, const std::regex& re,
    std::string::size_type from = std::string::npos) {
	std::smatch match;
	while (regex_search(str, match, re)) {
		const auto pos = match.position();
		if (from == std::string::npos) {
			return pos;
		}
		if (std::cmp_less_equal(from, pos)) {
			return pos;
		}
	}
	return std::string::npos;
}

}

const std::string_view DapMessage::content_length = "Content-Length: ";
int DapMessage::g_next_seq = 1;

std::unique_ptr<DapMessage> DapMessage::decode(const std::string& data) {
	try {
		const auto object = parse_json_object(data);
		if (const JsonString* type = object.get_string("type")) {
			if (*type == "request") {
				return std::make_unique<DapRequestMessage>(object);
			}
			if (*type == "response") {
				return std::make_unique<DapResponseMessage>(object);
			}
			if (*type == "event") {
				return std::make_unique<DapEventMessage>(object);
			}
		}
	}
	catch (std::exception& error) {
		std::println(Logger::default_logger(), "Failed to decode message: {}", error.what());
	}
	return nullptr;
}

DapRequestMessage::DapRequestMessage(const JsonObject& json) :
    _seq(attribute_or_default(json.get_number("seq"), -1)),
    _command(attribute_or_default(json.get_string("command"))),
    _arguments(attribute_as_optional(json.get_object("arguments"))) {}

std::string DapRequestMessage::encode() const {
	std::stringstream stream;
	stream << "{"
	       << R"("type":"request",)";
	if (_seq != -1) {
		stream << "\"seq\":" << _seq << ",";
	}
	stream << R"("command":")" << _command << "\"";
	if (_arguments) {
		stream << ",\"arguments\":" << _arguments->to_json();
	}
	stream << "}";
	return stream.str();
}

DapMessage::Type DapRequestMessage::get_type() const {
	return DapMessage::Type::request;
}

int DapRequestMessage::get_seq() const {
	return _seq;
}

std::string DapRequestMessage::get_command() const {
	return _command;
}

JsonObject DapRequestMessage::get_arguments() const {
	return _arguments.value_or({});
}

bool DapRequestMessage::has_arguments() const {
	return _arguments.has_value();
}

DapResponseMessage::DapResponseMessage(const JsonObject& json) :
    _seq(*json.get_number("seq")),
    _request_seq(*json.get_number("request_seq")),
    _success(*json.get_boolean("success")),
    _command(*json.get_string("command")),
    _message(*json.get_string("message")),
    _body(attribute_as_optional(json.get_object("body"))),
    _error(attribute_as_optional(json.get_object("error"))) {}

DapResponseMessage::DapResponseMessage(const DapRequestMessage& request, std::optional<JsonObject> body) :
    _seq(g_next_seq++),
    _request_seq(request.get_seq()),
    _success(true),
    _command(request.get_command()),
    _body(std::move(body)) {}

DapResponseMessage::DapResponseMessage(const DapRequestMessage& request, std::string message,
    std::optional<JsonObject> error) :
    _seq(g_next_seq++),
    _request_seq(request.get_seq()),
    _success(false),
    _command(request.get_command()),
    _message(std::move(message)),
    _error(std::move(error)) {}

std::string DapResponseMessage::encode() const {
	std::stringstream stream;
	stream << "{"
	       << R"("type":"response",)";
	if (_seq != -1) {
		stream << "\"seq\":" << _seq << ",";
	}
	if (_request_seq != -1) {
		stream << "\"request_seq\":" << _request_seq << ",";
	}
	stream << R"("command":")" << _command << "\","
	       << "\"success\":" << (_success ? "true" : "false");
	if (_success) {
		if (_body) {
			stream << ",\"body\":" << _body->to_json();
		}
	}
	else {
		stream << R"("message":")" << _message << "\"";
		if (_error) {
			stream << R"(,"error":")" << _error->to_json() << "\"";
		}
	}
	stream << "}";
	return stream.str();
}

DapMessage::Type DapResponseMessage::get_type() const {
	return DapMessage::Type::response;
}

int DapResponseMessage::get_seq() const {
	return _seq;
}

DapEventMessage::DapEventMessage(const JsonObject& json) :
    _seq(attribute_or_default(json.get_number("seq"), -1)),
    _event(attribute_or_default(json.get_string("event"))),
    _body(attribute_as_optional(json.get_object("body"))) {}

DapEventMessage::DapEventMessage(std::string event, std::optional<JsonObject> body) :
    _seq(g_next_seq++),
    _event(std::move(event)),
    _body(std::move(body)) {}

std::string DapEventMessage::encode() const {
	std::stringstream stream;
	stream << "{"
	       << R"("type":"event",)";
	if (_seq != -1) {
		stream << "\"seq\":" << _seq << ",";
	}
	stream << R"("event":")" << _event << "\"";
	if (_body) {
		stream << ",\"body\":" << _body->to_json();
	}
	stream << "}";
	return stream.str();
}

DapMessage::Type DapEventMessage::get_type() const {
	return DapMessage::Type::event;
}

int DapEventMessage::get_seq() const {
	return _seq;
}

std::string DapEventMessage::get_event() const {
	return _event;
}

std::unique_ptr<DapMessage> DapMessageReader::next_message() {

	read(_stream);

	auto begin = std::string::npos;
	auto length = next_message_length(begin);

	if (length != invalid_length && length <= _stream.size()) {
		if (std::unique_ptr<DapMessage> message = DapMessage::decode(_stream.substr(begin, length - begin))) {
			_stream.erase(0, begin + length);
			return message;
		}
	}

	return nullptr;
}

std::size_t DapMessageReader::next_message_length(std::string::size_type& begin) const {

	if (auto index = _stream.find(DapMessage::content_length); index != std::string::npos) {
		auto eol = regex_find(_stream, std::regex("\\r?\\n"), index);
		begin = regex_find(_stream, std::regex(R"(\r?\n\r?\n)"), index);
		if (begin != std::string::npos) {
			begin += _stream[begin] == '\r' ? 2 : 1;
			begin += _stream[begin] == '\r' ? 2 : 1;
			return begin
			       + std::stoull(_stream.substr(index + DapMessage::content_length.length(),
			           eol - index - DapMessage::content_length.length()));
		}
	}

	return invalid_length;
}

void DapMessageWriter::append_message(std::unique_ptr<DapMessage> message) {
	const std::string data = message->encode();
	write(std::string(DapMessage::content_length) + std::to_string(data.length()) + "\r\n\r\n" + data);
}
