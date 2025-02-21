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

#include "dapmessage.h"

#include <regex>

namespace {

std::string::size_type regex_find(const std::string &str, const std::regex &re,
										 std::string::size_type from = std::string::npos) {
	std::smatch match;
	while (regex_search(str, match, re)) {
		std::string::size_type pos = match.position();
		if (from == std::string::npos) {
			return pos;
		}
		if (from <= pos) {
			return pos;
		}
	}
	return std::string::npos;
}

}

const std::string DapMessage::CONTENT_LENGTH = "Content-Length: ";
int DapMessage::g_next_seq = 1;

std::unique_ptr<DapMessage> DapMessage::decode(const std::string &data) {
	if (std::unique_ptr<Json> json = Json::parse(data)) {
		if (const JsonObject *object = json->to_object()) {
			if (const JsonString *type = object->get_string("type")) {
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
	}
	return nullptr;
}

DapRequestMessage::DapRequestMessage(const JsonObject *json) :
	m_seq(attribute_or_default(json->get_number("seq"), -1)),
	m_command(*json->get_string("command")),
	m_arguments(attribute_or_default(json->get_object("arguments"), nullptr)) {}

std::string DapRequestMessage::encode() const {
	std::stringstream stream;
	stream << "{"
		   << R"("type":"request",)";
	if (m_seq != -1) {
		stream << "\"seq\":" << m_seq << ",";
	}
	stream << R"("command":")" << m_command << "\"";
	if (m_arguments) {
		stream << ",\"arguments\":" << m_arguments->to_json();
	}
	stream << "}";
	return stream.str();
}

DapMessage::Type DapRequestMessage::get_type() const {
	return REQUEST;
}

int DapRequestMessage::get_seq() const {
	return m_seq;
}

std::string DapRequestMessage::get_command() const {
	return m_command;
}

const JsonObject *DapRequestMessage::get_arguments() const {
	return m_arguments.get();
}

DapResponseMessage::DapResponseMessage(const JsonObject *json) :
	m_seq(*json->get_number("seq")),
	m_request_seq(*json->get_number("request_seq")),
	m_success(*json->get_boolean("success")),
	m_command(*json->get_string("command")),
	m_message(*json->get_string("message")),
	m_body(new JsonObject(*json->get_object("body"))),
	m_error(attribute_or_default(json->get_object("error"), nullptr)) {}

DapResponseMessage::DapResponseMessage(const DapRequestMessage *request, JsonObject *body) :
	m_seq(g_next_seq++),
	m_request_seq(request->get_seq()),
	m_success(true),
	m_command(request->get_command()),
	m_body(body) {}

DapResponseMessage::DapResponseMessage(const DapRequestMessage *request, std::string message, JsonObject *error) :
	m_seq(g_next_seq++),
	m_request_seq(request->get_seq()),
	m_success(false),
	m_command(request->get_command()),
	m_message(std::move(message)),
	m_error(error) {}

std::string DapResponseMessage::encode() const {
	std::stringstream stream;
	stream << "{"
		   << R"("type":"response",)";
	if (m_seq != -1) {
		stream << "\"seq\":" << m_seq << ",";
	}
	if (m_request_seq != -1) {
		stream << "\"request_seq\":" << m_request_seq << ",";
	}
	stream << R"("command":")" << m_command << "\","
		   << "\"success\":" << (m_success ? "true" : "false");
	if (m_success) {
		if (m_body) {
			stream << ",\"body\":" << m_body->to_json();
		}
	}
	else {
		stream << R"("message":")" << m_message << "\","
			   << R"("error":")" << m_error->to_json() << "\"";
	}
	stream << "}";
	return stream.str();
}

DapMessage::Type DapResponseMessage::get_type() const {
	return RESPONSE;
}

int DapResponseMessage::get_seq() const {
	return m_seq;
}

DapEventMessage::DapEventMessage(const JsonObject *json) :
	m_seq(attribute_or_default(json->get_number("seq"), -1)),
	m_event(*json->get_string("event")),
	m_body(new JsonObject(*json->get_object("body"))) {}

DapEventMessage::DapEventMessage(std::string event, JsonObject *body) :
	m_seq(g_next_seq++),
	m_event(std::move(event)),
	m_body(body) {}

std::string DapEventMessage::encode() const {
	std::stringstream stream;
	stream << "{"
		   << R"("type":"event",)";
	if (m_seq != -1) {
		stream << "\"seq\":" << m_seq << ",";
	}
	stream << R"("event":")" << m_event << "\"";
	if (m_body) {
		stream << ",\"body\":" << m_body->to_json();
	}
	stream << "}";
	return stream.str();
}

DapMessage::Type DapEventMessage::get_type() const {
	return EVENT;
}

int DapEventMessage::get_seq() const {
	return m_seq;
}

std::string DapEventMessage::get_event() const {
	return m_event;
}

std::unique_ptr<DapMessage> DapMessageReader::next_message() {

	read(m_stream);

	auto begin = std::string::npos;
	auto length = next_message_length(begin);

	if (length != INVALID_LENGTH && length <= m_stream.size()) {
		if (std::unique_ptr<DapMessage> message = DapMessage::decode(m_stream.substr(begin, length - begin))) {
			m_stream.erase(0, begin + length);
			return message;
		}
	}

	return nullptr;
}

size_t DapMessageReader::next_message_length(std::string::size_type &begin) const {

	auto index = m_stream.find(DapMessage::CONTENT_LENGTH);
	if (index != std::string::npos) {
		auto eol = regex_find(m_stream, std::regex("\\r?\\n"), index);
		begin = regex_find(m_stream, std::regex(R"(\r?\n\r?\n)"), index);
		if (begin != std::string::npos) {
			begin += m_stream[begin] == '\r' ? 2 : 1;
			begin += m_stream[begin] == '\r' ? 2 : 1;
			return begin
				   + stoull(m_stream.substr(index + DapMessage::CONTENT_LENGTH.length(),
											eol - index - DapMessage::CONTENT_LENGTH.length()));
		}
	}

	return INVALID_LENGTH;
}

void DapMessageWriter::append_message(std::unique_ptr<DapMessage> message) {
	const std::string data = message->encode();
	write(DapMessage::CONTENT_LENGTH + std::to_string(data.length()) + "\r\n\r\n" + data);
}
