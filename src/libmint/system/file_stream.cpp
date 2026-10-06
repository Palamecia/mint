/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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

#include "mint/system/file_stream.h"
#include "mint/system/filesystem.h"

#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

using namespace mint;

FileStream::FileStream(const std::filesystem::path& name) :
    _file(open_file(name, "r")),
    _path(name),
    _over(false) {}

FileStream::~FileStream() {
	if (_file) {
		std::fclose(_file);
	}
}

bool FileStream::at_end() const {
	return _over;
}

std::string FileStream::get_line(std::size_t line_number) {
	if (line_number == 0) {
		return {};
	}
	errno = 0;
	auto file = std::ifstream(_path);
	if (!file.is_open()) {
		throw std::system_error(errno ? errno : EIO, std::system_category(), _path.string());
	}
	auto line = std::string();
	for (auto current_line = std::size_t {1}; std::getline(file, line); ++current_line) {
		if (current_line == line_number) {
			return line;
		}
	}
	if (file.bad()) {
		throw std::system_error(errno, std::system_category(), _path.string());
	}
	return {};
}

bool FileStream::is_valid() const {
	return _file != nullptr;
}

std::filesystem::path FileStream::path() const {
	return _path;
}

int FileStream::read_char() {

	const auto c = fgetc(_file);

	switch (c) {
	case EOF:
		_over = true;
		break;
	default:
		break;
	}

	return c;
}
