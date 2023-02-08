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

#include "completer.h"

#include "mint/memory/globaldata.h"
#include "mint/system/filesystem.h"
#include "mint/ast/cursor.h"

#include <cstring>

using namespace mint;
using namespace std;

Completer::Completer(vector<completion_t> &completions, string_view::size_type offset, Cursor *cursor) :
	m_completions(completions),
	m_offset(offset),
	m_cursor(cursor) {

}

bool Completer::on_module_path_token(const vector<string> &context, const string &token, string::size_type offset) {
	if (m_offset > offset + token.size()) {
		return true;
	}
	if (m_offset >= offset) {
		string token_path;
		for (const string &module : context) {
			token_path.append(module);
		}
		token_path.append(token);
		for (const string &path : FileSystem::instance().library_path()) {
			const string root_path = FileSystem::instance().absolute_path(path);
			find_module_recursive_helper(root_path, root_path, token_path);
		}
	}
	return false;
}

bool Completer::on_symbol_token(const vector<string> &context, const string &token, string::size_type offset) {
	if (m_offset > offset + token.size()) {
		return true;
	}
	if (m_offset >= offset) {

		if (context.empty()) {
			for (auto &[symbol, _] : m_cursor->symbols()) {
				if (token_match(symbol.str(), token)) {
					m_completions.push_back({offset, symbol.str(), {}});
				}
			}
		}

		Reference *member = nullptr;
		PackageData *pack = nullptr;
		ClassDescription *desc = nullptr;

		if (resolve_path(context, pack, desc, member)) {
			find_context_symbols_helper(pack, desc, member, token, offset);
		}
	}
	return false;
}

bool Completer::on_symbol_token(const std::vector<std::string> &context, std::string::size_type offset) {
	if (m_offset > offset) {
		return true;
	}
	if (m_offset >= offset) {

		Reference *member = nullptr;
		PackageData *pack = nullptr;
		ClassDescription *desc = nullptr;

		if (resolve_path(context, pack, desc, member)) {
			find_context_symbols_helper(pack, desc, member, {}, offset);
		}
	}
	return false;
}

void Completer::find_module_recursive_helper(const string root_path, const string directory_path, const string &token_path) {
	FileSystem &fs = FileSystem::instance();
	for (auto it = fs.browse(directory_path); it != fs.end(); ++it) {
		const string file_name = *it;
		if (file_name == "." || file_name == "..") {
			continue;
		}
		const string file_path = directory_path + FileSystem::separator + file_name;
		if (fs.is_directory(file_path)) {
			find_module_recursive_helper(root_path, file_path, token_path);
		}
		else if (is_module_file(file_path)) {
			const string module_path = to_module_path(root_path, file_path);
			if (token_match(module_path, token_path)) {
				m_completions.push_back({ m_offset - token_path.size(), module_path });
			}
		}
	}
}

void Completer::find_context_symbols_helper(PackageData *pack, ClassDescription *desc, Reference *member, const string &token, string::size_type offset) {

	if (member) {
		if (member->data()->format == Data::fmt_object) {
			for (auto &[symbol, _] : member->data<Object>()->metadata->members()) {
				if (token_match(symbol.str(), token)) {
					m_completions.push_back({offset, symbol.str(), {}});
				}
			}
		}
	}

	if (desc) {
		Class *metadata = desc->generate();
		for (auto &[symbol, _] : metadata->globals()) {
			if (token_match(symbol.str(), token)) {
				m_completions.push_back({offset, symbol.str(), {}});
			}
		}
		return;
	}

	if (pack) {
		for (auto &[symbol, _] : pack->symbols()) {
			if (token_match(symbol.str(), token)) {
				m_completions.push_back({offset, symbol.str(), {}});
			}
		}
		return;
	}

	GlobalData *global_data = GlobalData::instance();
	for (auto &[symbol, _] : global_data->symbols()) {
		if (token_match(symbol.str(), token)) {
			m_completions.push_back({offset, symbol.str(), {}});
		}
	}
}

bool Completer::token_match(const string &token, const string &pattern) {
#ifdef OS_WINDOWS
	return token.size() >= pattern.size() && !strnicmp(pattern.c_str(), token.c_str(), pattern.size());
#else
	return token.size() >= pattern.size() && !strncasecmp(pattern.c_str(), token.c_str(), pattern.size());
#endif
}

bool Completer::is_module_file(const string &file_path) {
	auto pos = file_path.rfind('.');
	return pos != string::npos && file_path.substr(pos) == ".mn";
}

string Completer::to_module_path(const string &root_path, const string &file_path) {
	string module_path = FileSystem::instance().relative_path(root_path, file_path);
	module_path = module_path.substr(0, module_path.find('.'));
	for_each(module_path.begin(), module_path.end(), [](char &ch) {
		if (ch == FileSystem::separator) {
			ch = '.';
		}
	});
	return module_path;
}

bool Completer::resolve_path(const vector<string> &context, PackageData *&pack, ClassDescription *&desc, Reference *&member) {

	for (const string &token : context) {
		Symbol symbol(token);
		if (desc) {
			desc = desc->find_class_description(symbol);
			if (desc == nullptr) {
				return false;
			}
		}
		else if (pack) {
			desc = pack->find_class_description(symbol);
			if (desc == nullptr) {
				auto it = pack->symbols().find(symbol);
				if (it != pack->symbols().end()) {
					member = &it->second;
				}
				else {
					pack = pack->find_package(symbol);
					if (pack == nullptr) {
						return false;
					}
				}
			}
		}
		else {
			GlobalData *global_data = GlobalData::instance();
			desc = global_data->find_class_description(symbol);
			if (desc == nullptr) {
				pack = global_data->find_package(symbol);
				if (pack == nullptr) {
					return false;
				}
			}
		}
	}

	return true;
}
