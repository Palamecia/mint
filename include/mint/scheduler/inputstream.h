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

#ifndef MINT_INPUTSTREAM_H
#define MINT_INPUTSTREAM_H

#include "mint/system/datastream.h"
#include "mint/system/terminal.h"

namespace mint {

class MINT_EXPORT InputStream : public DataStream {
public:
	~InputStream();

	static InputStream &instance();
	
	bool at_end() const override;

	bool is_valid() const override;
	std::string path() const override;

	void next();

	void set_highlighter(std::function<std::string(std::string_view, std::string_view::size_type)> highlight);
	void set_completion_generator(std::function<bool(std::string_view, std::string_view::size_type, std::vector<completion_t> &)> generator);
	void set_brace_matcher(std::function<std::pair<std::string_view::size_type, bool>(std::string_view, std::string_view::size_type)> matcher);

protected:
	InputStream();

	void update_buffer();

	int read_char() override;
	int next_buffered_char() override;

private:
	enum Status {
		ready,
		could_start_comment,
		single_line_comment,
		multi_line_comment,
		could_end_comment,
		single_quote_string,
		single_quote_string_escape_next,
		double_quote_string,
		double_quote_string_escape_next,
		breaking,
		over
	};

	std::string m_buffer;
	char *m_cptr = nullptr;
	size_t m_level = 0;
	Status m_status = ready;
	bool m_must_fetch_more = true;
	Terminal m_terminal;
};

}

#endif // MINT_INPUTSTREAM_H
