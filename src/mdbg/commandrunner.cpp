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

#include "commandrunner.h"
#include "debugger.h"
#include "mint/debug/cursordebugger.h"
#include "mint/debug/debuginterface.h"
#include "mint/debug/debugtool.h"
#include "mint/system/filesystem.h"
#include "mint/system/terminal.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <format>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <print>
#include <ranges>
#include <regex>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

inline bool is_end_of_token(char ch) {
	return std::isblank(ch) || ch == '\n';
}

inline bool is_end_of_stream(char ch) {
	return ch == '\n';
}

inline bool is_numeric_token(std::string_view token) {
	return std::ranges::all_of(token, [](char ch) {
		return std::isdigit(ch);
	});
}

template<class Pred>
std::string_view::size_type find_if(std::string_view sv, Pred pred) {
	if (auto it = std::ranges::find_if(sv, pred); it != sv.end()) {
		return std::distance(sv.begin(), it);
	}
	return std::string_view::npos;
}

std::string get_command(std::string_view& stream) {

	std::string::size_type start = 0;
	while (start < stream.size() && std::isblank(stream[start])) {
		++start;
	}

	auto token = stream.substr(start);
	if (token.empty()) {
		stream = token;
		return std::string(token);
	}

	auto pos = find_if(token, is_end_of_token);
	if (pos == std::string_view::npos) {
		auto command = std::string(token);
		stream = token.substr(token.size());
		return command;
	}

	auto command = std::string(token.substr(0, pos));
	stream = token.substr(pos);
	return command;
}

std::string to_vt100(const std::string& str) {
	std::string converted = str;
	converted = std::regex_replace(converted, std::regex(R"(\*\*(.+?)\*\*)"),
	    MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_BOLD) "$1"));
	converted = std::regex_replace(converted, std::regex(R"(__(.+?)__)"),
	    MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_BOLD) "$1"));
	converted = std::regex_replace(converted, std::regex(R"(\*(.+?)\*)"),
	    MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_ITALIC) "$1"));
	converted = std::regex_replace(converted, std::regex(R"(_(.+?)_)"),
	    MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_ITALIC) "$1"));
	return converted;
}

bool find_module_recursive_helper(const std::filesystem::path& root_path, const std::filesystem::path& directory_path,
    std::string_view token_path, std::string_view::size_type offset, std::vector<mint::Completion>& completion) {
	bool match = false;
	for (const auto& entry : std::filesystem::directory_iterator {directory_path}) {
		if (entry.is_directory()) {
			if (find_module_recursive_helper(root_path, entry.path(), token_path, offset, completion)) {
				match = true;
			}
		}
		else if (mint::is_module_file(entry.path())) {
			const std::string module_path = mint::FileSystem::to_module_path(root_path, entry.path());
			if (module_path.starts_with(token_path)) {
				completion.push_back({
				    .offset = offset,
				    .token = module_path,
				    .hint = {},
				});
				match = true;
			}
		}
	}
	return match;
}

}

CommandRunner::Parameter::Parameter(const std::vector<std::string>& names) :
    _type(action),
    _names(names) {}

CommandRunner::Parameter::Parameter(Type type, const std::string& name) :
    _type(type),
    _names({name}) {}

bool CommandRunner::Parameter::match(std::string_view& stream) const {

	std::string::size_type start = 0;
	while (start < stream.size() && std::isblank(stream[start])) {
		++start;
	}

	auto token = stream.substr(start);
	if (token.empty()) {
		return false;
	}

	switch (_type) {
	case action:
		if (auto pos = find_if(token, is_end_of_token); pos != std::string_view::npos) {
			if (auto it = std::ranges::find(_names, token.substr(0, pos)); it != _names.end()) {
				stream = token.substr(pos);
				return true;
			}
		}
		break;
	case module:
		if (auto pos = find_if(token, is_end_of_token); pos != std::string_view::npos) {
			stream = token.substr(pos);
			return true;
		}
		break;
	case script:
		if (auto pos = find_if(token, is_end_of_stream); pos != std::string_view::npos) {
			stream = token.substr(pos);
			return true;
		}
		break;
	case offset:
		if (!std::isdigit(token[0]) && token[0] != '+' && token[0] != '-') {
			return false;
		}
		if (auto pos = find_if(token, is_end_of_token); pos != std::string_view::npos) {
			if (!is_numeric_token(token.substr(1, pos - 1))) {
				return false;
			}
			stream = token.substr(pos);
			return true;
		}
		break;
	case line_number:
	case thread_id:
	case breakpoint_id:
		if (auto pos = find_if(token, is_end_of_token); pos != std::string_view::npos) {
			if (!is_numeric_token(token.substr(0, pos))) {
				return false;
			}
			stream = token.substr(pos);
			return true;
		}
		break;
	}
	return false;
}

bool CommandRunner::Parameter::complete(const Debugger& debugger, std::string_view stream,
    std::string_view::size_type parameter_offset, std::vector<mint::Completion>& completion) const {

	std::string::size_type start = 0;
	while (start < stream.size() && std::isblank(stream[start])) {
		++start;
	}

	auto token = stream.substr(start);
	bool match = false;

	switch (_type) {
	case action:
		for (const auto& name : _names) {
			if (name.starts_with(token)) {
				completion.push_back({
				    .offset = parameter_offset + start,
				    .token = name,
				    .hint = {},
				});
				match = true;
			}
		}
		break;
	case module:
		for (const std::filesystem::path& path : mint::FileSystem::instance().library_path()) {
			const std::filesystem::path root_path = std::filesystem::absolute(path);
			if (find_module_recursive_helper(root_path, root_path, token, parameter_offset + start, completion)) {
				match = true;
			}
		}
		break;
	case thread_id:
		for (const auto threads = debugger.get_threads(); const mint::CursorDebugger& thread : threads) {
			const auto thread_id = std::to_string(thread.get_thread_id());
			if (thread_id.starts_with(token)) {
				completion.push_back({
				    .offset = parameter_offset + start,
				    .token = thread_id,
				    .hint = {},
				});
				match = true;
			}
		}
		break;
	case breakpoint_id:
		for (const auto breakpoints = debugger.get_breakpoints(); const mint::Breakpoint& breakpoint : breakpoints) {
			const auto breakpoint_id = std::to_string(breakpoint.id);
			if (breakpoint_id.starts_with(token)) {
				completion.push_back({
				    .offset = parameter_offset + start,
				    .token = breakpoint_id,
				    .hint = {},
				});
				match = true;
			}
		}
		break;
	default:
		break;
	}

	return match;
}

void CommandRunner::Parameter::parse(std::string_view& stream, std::vector<value_t>& parameters) const {

	std::string::size_type start = 0;
	while (start < stream.size() && std::isblank(stream[start])) {
		++start;
	}

	auto token = stream.substr(start);
	if (token.empty()) {
		return;
	}

	switch (_type) {
	case action:
		if (auto pos = find_if(token, is_end_of_token); pos != std::string_view::npos) {
			stream = token.substr(pos);
		}
		break;
	case module:
		if (auto pos = find_if(token, is_end_of_token); pos != std::string_view::npos) {
			parameters.emplace_back(std::string(token.substr(0, pos)));
			stream = token.substr(pos);
		}
		break;
	case script:
		if (auto pos = find_if(token, is_end_of_stream); pos != std::string_view::npos) {
			parameters.emplace_back(std::string(token.substr(0, pos)));
			stream = token.substr(pos);
		}
		break;
	case offset:
	case thread_id:
		if (auto pos = find_if(token, is_end_of_token); pos != std::string_view::npos) {
			parameters.emplace_back(std::stoll(std::string(token.substr(0, pos))));
			stream = token.substr(pos);
		}
		break;
	case line_number:
	case breakpoint_id:
		if (auto pos = find_if(token, is_end_of_token); pos != std::string_view::npos) {
			parameters.emplace_back(std::stoull(std::string(token.substr(0, pos))));
			stream = token.substr(pos);
		}
		break;
	}
}

std::string CommandRunner::Parameter::help() const {
	switch (_type) {
	case action:
		return std::format("**{}**",
		    std::views::join_with(_names, std::string("** | **")) | std::ranges::to<std::string>());
	case module:
	case script:
		return std::format("*<{}>*", _names.front());
	case offset:
		return std::format("*<{0}>* | *+<{0}>* | *-<{0}>*", _names.front());
	case line_number:
	case thread_id:
	case breakpoint_id:
		return std::format("*<{}>*", _names.front());
	}
	return {};
}

CommandRunner::Command::Command(std::vector<std::string> names, std::string description) :
    _names(std::move(names)),
    _description(std::move(description)) {}

void CommandRunner::Command::add(std::initializer_list<Parameter> parameters, std::string description,
    std::function<void(Debugger&, mint::CursorDebugger&, const std::span<Parameter::value_t>&)>&& callback) {
	_choices.push_back(Choice {
	    .parameters = std::vector<Parameter>(parameters),
	    .description = std::move(description),
	    .callback = std::move(callback),
	});
}

bool CommandRunner::Command::complete(const Debugger& debugger, std::string_view stream,
    std::string_view::size_type offset, std::vector<mint::Completion>& completion) const {
	bool match = false;
	for (const auto& choice : _choices) {
		if (complete(choice, debugger, stream, offset, completion)) {
			match = true;
		}
	}
	return match;
}

void CommandRunner::Command::run(Debugger& debugger, mint::CursorDebugger& cursor, std::string_view& stream) {

	for (const auto& choice : _choices) {
		if (match(choice, stream)) {
			std::vector<Parameter::value_t> parameters;
			for (const auto& parameter : choice.parameters) {
				parameter.parse(stream, parameters);
			}
			choice.callback(debugger, cursor, parameters);
			return;
		}
	}

	print_help();
}

void CommandRunner::Command::print_help() const {
	const auto name = MINT_TERM_STDSTR(MINT_TERM_OPT(MINT_TERM_BOLD) + _names.back());
	for (const auto& choice : _choices) {
		if (choice.parameters.empty()) {
			std::println("{}:", name);
		}
		else {
			std::println("{} {}:", name, to_vt100(help(choice)));
		}
		std::println("\t{}", to_vt100(choice.description));
	}
}

bool CommandRunner::Command::complete(const Choice& choice, const Debugger& debugger, std::string_view stream,
    std::string_view::size_type offset, std::vector<mint::Completion>& completion) {
	const auto from = stream;
	for (const auto& parameter : choice.parameters) {
		const auto parameter_offset = offset + std::distance(from.data(), stream.data());
		if (!parameter.complete(debugger, stream, parameter_offset, completion)) {
			if (parameter.match(stream)) {
				continue;
			}
		}
		return true;
	}
	return false;
}

bool CommandRunner::Command::match(const Choice& choice, std::string_view stream) {
	for (const auto& parameter : choice.parameters) {
		if (!parameter.match(stream)) {
			return false;
		}
	}
	return true;
}

std::string CommandRunner::Command::help(const Choice& choice) {
	return {std::from_range,
	    std::views::transform(choice.parameters, &Parameter::help) | std::views::join_with(std::string(" "))};
}

CommandRunner::Command& CommandRunner::add_command(std::vector<std::string> names, std::string description) {
	auto& command = _commands.emplace_back(std::move(names), std::move(description));
	command.add({{{"?"}}}, "Prints this help message.",
	    [&command](Debugger&, mint::CursorDebugger&, const std::span<Parameter::value_t>&) {
		    command.print_help();
	    });
	return command;
}

std::optional<std::vector<mint::Completion>> CommandRunner::complete(const Debugger& debugger,
    std::string_view stream) const {

	auto completion = std::vector<mint::Completion>();
	const auto from = stream;

	for (auto command = get_command(stream); !command.empty(); command = get_command(stream)) {
		for (const Command& entry : _commands) {
			if (std::ranges::any_of(entry.get_names(), [&command](const std::string& name) {
				    return name.starts_with(command);
			    })) {
				const auto offset = std::distance(from.data(), stream.data());
				if (stream.empty()) {
					for (const std::string& name : entry.get_names()) {
						completion.push_back({
						    .offset = offset - command.length(),
						    .token = name,
						    .hint = {},
						});
					}
				}
				else {
					if (entry.complete(debugger, stream, offset, completion)) {
						return completion;
					}
				}
			}
		}
	}

	return completion;
}

void CommandRunner::run(Debugger& debugger, mint::CursorDebugger& cursor, std::string_view& stream) {
	for (auto command = get_command(stream); !command.empty(); command = get_command(stream)) {
		auto it = std::ranges::find_if(_commands, [&command](const Command& entry) {
			return std::ranges::any_of(entry.get_names(), [&command](const std::string& name) {
				return name == command;
			});
		});
		if (it != _commands.end()) {
			it->run(debugger, cursor, stream);
		}
		else {
			print_commands();
			return;
		}
	}
}

void CommandRunner::print_commands() {
	for (const Command& command : _commands) {
		const std::string names(std::from_range, std::views::join_with(command.get_names(), std::string(" | ")));
		mint::Terminal::println(stdout, std::format(MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_BOLD) "{}") ":", names));
		mint::Terminal::println(stdout, std::format("\t{}", command.get_description()));
	}
}
