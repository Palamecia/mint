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

#include <string>
#include <memory>

class DapMessage {
public:
	enum Type {
		request,
		response,
		event
	};

	static const std::string content_length;

	virtual ~DapMessage() = default;

	static std::unique_ptr<DapMessage> decode(const std::string &data);
	virtual std::string encode() const = 0;

	virtual Type get_type() const = 0;
	virtual int get_seq() const = 0;

protected:
	static int g_next_seq;
};

class DapRequestMessage : public DapMessage {
public:
	DapRequestMessage(const JsonObject *json);

	std::string encode() const override;

	Type get_type() const override;
	int get_seq() const override;

	std::string get_command() const;
	const JsonObject *get_arguments() const;

private:
	int m_seq;
	std::string m_command;
	std::unique_ptr<JsonObject> m_arguments;
};

enum ErrorDestination {
	User = 1,
	Telemetry = 2
};

class DapResponseMessage : public DapMessage {
public:
	DapResponseMessage(const JsonObject *json);
	DapResponseMessage(const DapRequestMessage *request, JsonObject *body);
	DapResponseMessage(const DapRequestMessage *request, const std::string &message, JsonObject *error);

	std::string encode() const override;

	Type get_type() const override;
	int get_seq() const override;

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
	DapEventMessage(const std::string &event, JsonObject *body);

	std::string encode() const override;

	Type get_type() const override;
	int get_seq() const override;

	std::string get_event() const;

private:
	int m_seq;
	std::string m_event;
	std::unique_ptr<JsonObject> m_body;
};

class DapMessageReader {
public:
	virtual ~DapMessageReader() = default;

	std::unique_ptr<DapMessage> nextMessage();

protected:
	static constexpr const size_t invalid_length = static_cast<size_t>(-1);

	virtual size_t read(std::string &data) = 0;

private:
	size_t next_message_length(std::string::size_type &begin) const;

	std::string m_stream;
};

class DapMessageWriter {
public:
	virtual ~DapMessageWriter() = default;

	void appendMessage(std::unique_ptr<DapMessage> message);

protected:
	static constexpr const size_t invalid_length = static_cast<size_t>(-1);

	virtual size_t write(const std::string &data) = 0;
};

#endif // MDBG_DAPMESSAGE_H
