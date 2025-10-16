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

#ifndef MDBG_COMMANDRUNNER_H
#define MDBG_COMMANDRUNNER_H

#include "debugger.h"
#include "mint/debug/cursordebugger.h"
#include "mint/system/terminal.h"
#include <cstdint>
#include <deque>
#include <functional>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

class CommandRunner {
public:
	class Parameter {
	public:
		using value_t = std::variant<std::intmax_t, std::uintmax_t, std::string>;

		enum Type : std::uint8_t {
			action,
			module,
			script,
			offset,
			line_number,
			thread_id,
			breakpoint_id,
		};

		Parameter(const std::vector<std::string>& names);
		Parameter(Type type, const std::string& name);

		[[nodiscard]] bool match(std::string_view& stream) const;
		[[nodiscard]] bool complete(const Debugger& debugger, std::string_view stream,
		    std::string_view::size_type parameter_offset, std::vector<mint::Completion>& completion) const;
		void parse(std::string_view& stream, std::vector<value_t>& parameters) const;
		[[nodiscard]] std::string help() const;

	private:
		Type _type;
		std::vector<std::string> _names;
	};

	class Command {
		struct Choice {
			std::vector<Parameter> parameters;
			std::string description;
			std::function<void(Debugger&, mint::CursorDebugger&, const std::span<Parameter::value_t>&)> callback;
		};

		std::vector<std::string> _names;
		std::string _description;
		std::vector<Choice> _choices;
	public:
		Command(std::vector<std::string> names, std::string description);

		[[nodiscard]] inline std::span<const std::string> get_names() const;
		[[nodiscard]] inline std::string_view get_description() const;

		void add(std::initializer_list<Parameter> parameters, std::string description,
		    std::function<void(Debugger&, mint::CursorDebugger&, const std::span<Parameter::value_t>&)>&& callback);

		bool complete(const Debugger& debugger, std::string_view stream, std::string_view::size_type offset,
		    std::vector<mint::Completion>& completion) const;
		void run(Debugger& debugger, mint::CursorDebugger& cursor, std::string_view& stream);
		void print_help() const;

	private:
		static bool complete(const Choice& choice, const Debugger& debugger, std::string_view stream,
		    std::string_view::size_type offset, std::vector<mint::Completion>& completion);
		static bool match(const Choice& choice, std::string_view stream);
		static std::string help(const Choice& choice);
	};

	CommandRunner() = default;

	Command& add_command(std::vector<std::string> names, std::string description);

	[[nodiscard]] std::optional<std::vector<mint::Completion>> complete(const Debugger& debugger,
	    std::string_view stream) const;
	void run(Debugger& debugger, mint::CursorDebugger& cursor, std::string_view& stream);
	void print_commands();

private:
	std::deque<Command> _commands;
};

template<typename T>
T get_parameter(const CommandRunner::Parameter::value_t& parameter) {
	if constexpr (std::is_integral_v<T>) {
		if constexpr (std::is_signed_v<T>) {
			return std::get<std::intmax_t>(parameter);
		}
		else {
			return std::get<std::uintmax_t>(parameter);
		}
	}
	else {
		return std::get<std::string>(parameter);
	}
}

std::span<const std::string> CommandRunner::Command::get_names() const {
	return {_names};
}

std::string_view CommandRunner::Command::get_description() const {
	return _description;
}

#endif // MDBG_COMMANDRUNNER_H
