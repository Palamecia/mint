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

#ifndef MINT_BUFFERSTREAM_H
#define MINT_BUFFERSTREAM_H

#include "mint/system/datastream.h"

namespace mint {

class MINT_EXPORT BufferStream : public DataStream {
public:
	explicit BufferStream(const std::string &buffer);
	~BufferStream() override;

	BufferStream(const BufferStream &other) = delete;
	BufferStream &operator =(const BufferStream &other) = delete;

	bool at_end() const override;

	bool is_valid() const override;
	std::string path() const override;

protected:
	int read_char() override;
	int next_buffered_char() override;

private:
	enum Status {
		READY,
		FLUSH,
		OVER
	};

	const char *m_buffer;
	const char *m_cptr;
	Status m_status;
};

}

#endif // MINT_BUFFERSTREAM_H
