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

#ifndef MINT_SYSTEM_DATASTREAM_H
#define MINT_SYSTEM_DATASTREAM_H

#include "mint/config.h"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <cstdint>
#include <string>

namespace mint {

class MINT_EXPORT DataStream {
public:
	DataStream() = default;
	DataStream(DataStream&&) = default;
	DataStream(const DataStream&) = default;
	virtual ~DataStream() = default;

	DataStream& operator=(DataStream&&) = default;
	DataStream& operator=(const DataStream&) = default;

	int get_char();
	[[nodiscard]] virtual bool at_end() const = 0;

	[[nodiscard]] virtual bool is_valid() const = 0;
	[[nodiscard]] virtual std::filesystem::path path() const = 0;

	void set_new_line_callback(const std::function<void(std::size_t)>& callback);
	[[nodiscard]] std::size_t line_number() const;
	[[nodiscard]] std::string line_error();

protected:
	virtual int read_char() = 0;
	virtual int next_buffered_char() = 0;

private:
	void begin_line();
	void end_line();

	enum class State : std::uint8_t {
		state_new_line,
		state_reading
	};

	std::function<void(std::size_t)> _new_line_callback = [](std::size_t) {};
	std::size_t _line_number = 1;
	State _state = State::state_new_line;

	std::string _cached_line;
};

}

#endif // MINT_SYSTEM_DATASTREAM_H
