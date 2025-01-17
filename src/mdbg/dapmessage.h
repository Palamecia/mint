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

#ifndef MDBG_DAPMESSAGE_H
#define MDBG_DAPMESSAGE_H

#include "json.hpp"

#include <cstdint>
#include <string>
#include <memory>

class DapMessage {
public:
	enum Type : std::uint8_t {
		REQUEST,
		RESPONSE,
		EVENT
	};

	static const std::string CONTENT_LENGTH;

	DapMessage() = default;
	DapMessage(const DapMessage &) = delete;
	DapMessage(DapMessage &&) = delete;
	virtual ~DapMessage() = default;

	DapMessage &operator=(const DapMessage &) = delete;
	DapMessage &operator=(DapMessage &&) = delete;

	[[nodiscard]] static std::unique_ptr<DapMessage> decode(const std::string &data);
	[[nodiscard]] virtual std::string encode() const = 0;

	[[nodiscard]] virtual Type get_type() const = 0;
	[[nodiscard]] virtual int get_seq() const = 0;

protected:
	static int g_next_seq;
};

class DapRequestMessage : public DapMessage {
public:
	DapRequestMessage(const JsonObject *json);

	[[nodiscard]] std::string encode() const override;

	[[nodiscard]] Type get_type() const override;
	[[nodiscard]] int get_seq() const override;

	[[nodiscard]] std::string get_command() const;
	[[nodiscard]] const JsonObject *get_arguments() const;

private:
	int m_seq;
	std::string m_command;
	std::unique_ptr<JsonObject> m_arguments;
};

enum ErrorDestination : std::uint8_t {
	USER = 1,
	TELEMETRY = 2
};

class DapResponseMessage : public DapMessage {
public:
	DapResponseMessage(const JsonObject *json);
	DapResponseMessage(const DapRequestMessage *request, JsonObject *body);
	DapResponseMessage(const DapRequestMessage *request, std::string message, JsonObject *error);

	[[nodiscard]] std::string encode() const override;

	[[nodiscard]] Type get_type() const override;
	[[nodiscard]] int get_seq() const override;

private:
	int m_seq;
	int m_request_seq;
	bool m_success;
	std::string m_command;
	std::string m_message;
	std::unique_ptr<JsonObject> m_body;
	std::unique_ptr<JsonObject> m_error;
};

class DapEventMessage : public DapMessage {
public:
	DapEventMessage(const JsonObject *json);
	DapEventMessage(std::string event, JsonObject *body);

	[[nodiscard]] std::string encode() const override;

	[[nodiscard]] Type get_type() const override;
	[[nodiscard]] int get_seq() const override;

	[[nodiscard]] std::string get_event() const;

private:
	int m_seq;
	std::string m_event;
	std::unique_ptr<JsonObject> m_body;
};

class DapMessageReader {
public:
	DapMessageReader() = default;
	DapMessageReader(const DapMessageReader &) = delete;
	DapMessageReader(DapMessageReader &&) = delete;
	virtual ~DapMessageReader() = default;

	DapMessageReader &operator=(const DapMessageReader &) = delete;
	DapMessageReader &operator=(DapMessageReader &&) = delete;

	[[nodiscard]] std::unique_ptr<DapMessage> next_message();

protected:
	static constexpr const size_t INVALID_LENGTH = static_cast<size_t>(-1);

	virtual size_t read(std::string &data) = 0;

private:
	size_t next_message_length(std::string::size_type &begin) const;

	std::string m_stream;
};

class DapMessageWriter {
public:
	DapMessageWriter() = default;
	DapMessageWriter(const DapMessageWriter &) = delete;
	DapMessageWriter(DapMessageWriter &&) = delete;
	virtual ~DapMessageWriter() = default;

	DapMessageWriter &operator=(const DapMessageWriter &) = delete;
	DapMessageWriter &operator=(DapMessageWriter &&) = delete;

	void append_message(std::unique_ptr<DapMessage> message);

protected:
	static constexpr const size_t INVALID_LENGTH = static_cast<size_t>(-1);

	virtual size_t write(const std::string &data) = 0;
};

#endif // MDBG_DAPMESSAGE_H
