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

#ifndef MINT_DATASTREAM_H
#define MINT_DATASTREAM_H

#include "mint/config.h"

#include <functional>
#include <string>

namespace mint {

class MINT_EXPORT DataStream {
public:
	DataStream();
	virtual ~DataStream();

	int get_char();
	virtual bool at_end() const = 0;

	virtual bool is_valid() const = 0;
	virtual std::string path() const = 0;

	void set_new_line_callback(const std::function<void(size_t)> &callback);
	size_t line_number() const;
	std::string line_error();

protected:
	virtual int read_char() = 0;
	virtual int next_buffered_char() = 0;

private:
	void begin_line();
	void end_line();

	enum State {
		STATE_NEW_LINE,
		STATE_READING
	};

	std::function<void(size_t)> m_new_line_callback;
	size_t m_line_number;
	State m_state;

	std::string m_cached_line;
};

}

#endif // MINT_DATASTREAM_H
