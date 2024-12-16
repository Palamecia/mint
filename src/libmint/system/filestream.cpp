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

#include "mint/system/filestream.h"
#include "mint/system/filesystem.h"

using namespace mint;

FileStream::FileStream(const std::string &name) :
	m_file(open_file(name.c_str(), "r")),
	m_path(name),
	m_over(false) {

}

FileStream::~FileStream() {

	if (m_file) {
		fclose(m_file);
	}
}

bool FileStream::at_end() const {
	return m_over;
}

bool FileStream::is_valid() const {
	return m_file != nullptr;
}

std::string FileStream::path() const {
	return m_path;
}

int FileStream::read_char() {
	
	int c = next_buffered_char();

	switch (c) {
	case EOF:
		m_over = true;
		break;
	default:
		break;
	}

	return c;
}

int FileStream::next_buffered_char() {
	return fgetc(m_file);
}
