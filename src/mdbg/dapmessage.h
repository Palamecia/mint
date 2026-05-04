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

#ifndef MDBG_DAPMESSAGE_H
#define MDBG_DAPMESSAGE_H

#include "json.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <memory>
#include <string_view>

class DapMessage {
public:
	enum class Type : std::uint8_t {
		request,
		response,
		event
	};

	static const std::string_view content_length;

	DapMessage() = default;
	DapMessage(const DapMessage&) = default;
	DapMessage(DapMessage&&) = default;
	virtual ~DapMessage() = default;

	DapMessage& operator=(const DapMessage&) = default;
	DapMessage& operator=(DapMessage&&) = default;

	[[nodiscard]] static std::unique_ptr<DapMessage> decode(const std::string& data);
	[[nodiscard]] virtual std::string encode() const = 0;

	[[nodiscard]] virtual Type get_type() const = 0;
	[[nodiscard]] virtual int get_seq() const = 0;

protected:
	static int g_next_seq;
};

class DapRequestMessage : public DapMessage {
public:
	DapRequestMessage(const JsonObject& json);

	[[nodiscard]] std::string encode() const override;

	[[nodiscard]] Type get_type() const override;
	[[nodiscard]] int get_seq() const override;

	[[nodiscard]] std::string get_command() const;
	[[nodiscard]] JsonObject get_arguments() const;
	[[nodiscard]] bool has_arguments() const;

private:
	int _seq;
	std::string _command;
	std::optional<const JsonObject> _arguments;
};

using ErrorDestination = std::uint8_t;
constexpr inline ErrorDestination user = 0x01;
constexpr inline ErrorDestination telemetry = 0x02;

class DapResponseMessage : public DapMessage {
public:
	DapResponseMessage(const JsonObject& json);
	DapResponseMessage(const DapRequestMessage& request, std::optional<JsonObject> body);
	DapResponseMessage(const DapRequestMessage& request, std::string message, std::optional<JsonObject> error);

	[[nodiscard]] std::string encode() const override;

	[[nodiscard]] Type get_type() const override;
	[[nodiscard]] int get_seq() const override;

private:
	int _seq;
	int _request_seq;
	bool _success;
	std::string _command;
	std::string _message;
	std::optional<const JsonObject> _body;
	std::optional<const JsonObject> _error;
};

class DapEventMessage : public DapMessage {
public:
	DapEventMessage(const JsonObject& json);
	DapEventMessage(std::string event, std::optional<JsonObject> body);

	[[nodiscard]] std::string encode() const override;

	[[nodiscard]] Type get_type() const override;
	[[nodiscard]] int get_seq() const override;

	[[nodiscard]] std::string get_event() const;

private:
	int _seq;
	std::string _event;
	std::optional<const JsonObject> _body;
};

template<class Visitor>
void visit(Visitor&& visitor, const DapMessage& message) {
	switch (message.get_type()) {
	case DapMessage::Type::request:
		std::invoke(std::forward<Visitor>(visitor), static_cast<const DapRequestMessage&>(message));
		break;
	case DapMessage::Type::response:
		std::invoke(std::forward<Visitor>(visitor), static_cast<const DapResponseMessage&>(message));
		break;
	case DapMessage::Type::event:
		std::invoke(std::forward<Visitor>(visitor), static_cast<const DapEventMessage&>(message));
		break;
	}
}

class DapMessageReader {
public:
	DapMessageReader() = default;
	DapMessageReader(const DapMessageReader&) = delete;
	DapMessageReader(DapMessageReader&&) = delete;
	virtual ~DapMessageReader() = default;

	DapMessageReader& operator=(const DapMessageReader&) = delete;
	DapMessageReader& operator=(DapMessageReader&&) = delete;

	[[nodiscard]] std::unique_ptr<DapMessage> next_message();

protected:
	static constexpr const std::size_t invalid_length = std::numeric_limits<std::size_t>::max();

	virtual std::size_t read(std::string& data) = 0;

private:
	std::size_t next_message_length(std::string::size_type& begin) const;

	std::string _stream;
};

class DapMessageWriter {
public:
	DapMessageWriter() = default;
	DapMessageWriter(const DapMessageWriter&) = delete;
	DapMessageWriter(DapMessageWriter&&) = delete;
	virtual ~DapMessageWriter() = default;

	DapMessageWriter& operator=(const DapMessageWriter&) = delete;
	DapMessageWriter& operator=(DapMessageWriter&&) = delete;

	void append_message(std::unique_ptr<DapMessage> message);

protected:
	static constexpr const std::size_t invalid_length = std::numeric_limits<std::size_t>::max();

	virtual std::size_t write(const std::string& data) = 0;
};

#endif // MDBG_DAPMESSAGE_H
