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

#ifndef MINT_FILESTREAM_H
#define MINT_FILESTREAM_H

#include "mint/system/datastream.h"

#include <filesystem>

namespace mint {

class MINT_EXPORT FileStream : public DataStream {
public:
	explicit FileStream(const std::filesystem::path &name);
	FileStream(FileStream &&) = delete;
	FileStream(const FileStream &) = delete;
	~FileStream() override;

	FileStream &operator=(FileStream &&) = delete;
	FileStream &operator=(const FileStream &) = delete;

	[[nodiscard]] bool at_end() const override;

	[[nodiscard]] bool is_valid() const override;
	[[nodiscard]] std::filesystem::path path() const override;

protected:
	int read_char() override;
	int next_buffered_char() override;

private:
	FILE *m_file;
	std::filesystem::path m_path;
	bool m_over;
};

}

#endif // MINT_FILESTREAM_H
