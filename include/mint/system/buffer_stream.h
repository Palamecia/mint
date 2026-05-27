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

#ifndef MINT_SYSTEM_BUFFER_STREAM_H
#define MINT_SYSTEM_BUFFER_STREAM_H

#include "mint/config.h"
#include "mint/system/data_stream.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <gsl/pointers>
#include <string>

namespace mint {

class MINT_EXPORT BufferStream : public DataStream {
public:
	explicit BufferStream(std::string buffer);

	[[nodiscard]] bool at_end() const override;

	[[nodiscard]] bool is_valid() const override;
	[[nodiscard]] std::filesystem::path path() const override;

protected:
	int read_char() override;
	int next_buffered_char() override;

private:
	enum class Status : std::uint8_t {
		ready,
		flush,
		over
	};

	std::string _buffer;
	std::size_t _pos = 0;
	Status _status;
};

}

#endif // MINT_SYSTEM_BUFFER_STREAM_H
