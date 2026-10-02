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

#include "mint/system/arguments.h"
#include <array>
#include <span>
#include <string>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#include <processenv.h>
#include <shellapi.h>
#include <stringapiset.h>
#include <winnls.h>
#elifdef MINT_OS_LINUX
#include "mint/system/filesystem.h"
#include <gsl/pointers>
#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#elifdef MINT_OS_MAC
#include <mach-o/dyld.h>
#include <crt_externs.h>
#elifdef MINT_OS_FREE_BSD
#include <sys/types.h>
#include <sys/sysctl.h>
#include <unistd.h>
#endif

std::vector<std::string> mint::arguments() {
#ifdef MINT_OS_WINDOWS

	int argc = 0;
	auto* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	auto args = std::vector<std::string>();
	args.reserve(argc);

	for (const auto* arg : std::span(argv, argc)) {
		const int length = WideCharToMultiByte(CP_UTF8, 0, arg, -1, nullptr, 0, nullptr, nullptr);
		auto argument = std::string(length, '\0');
		WideCharToMultiByte(CP_UTF8, 0, arg, -1, argument.data(), length, nullptr, nullptr);
		argument.resize(length - 1);
		args.push_back(argument);
	}

	return args;

#elifdef MINT_OS_LINUX

	auto args = std::vector<std::string>();

	if (gsl::owner<FILE*> cmdline = mint::open_file("/proc/self/cmdline", "r")) {

		gsl::owner<char*> buffer = nullptr;
		std::size_t buffer_length = 0;

		while (getdelim(&buffer, &buffer_length, 0, cmdline) != -1) {
			args.emplace_back(buffer);
		}

		std::fclose(cmdline);
		std::free(buffer);
	}

	return args;

#elifdef MINT_OS_FREE_BSD

	auto mib = std::to_array<int>({CTL_KERN, KERN_PROC, KERN_PROC_ARGS, getpid()});
	auto args = std::vector<std::string>();

	if (std::size_t size = 0; sysctl(mib.data(), mib.size(), nullptr, &size, nullptr, 0) == 0 && size > 0) {
		if (auto buffer = std::vector<char>(size);
		    sysctl(mib.data(), mib.size(), buffer.data(), &size, nullptr, 0) == 0) {
			for (std::size_t i = 0; i < size; ++i) {
				if (auto arg = std::string(&buffer[i]); !arg.empty()) {
					i += arg.size();
					args.push_back(std::move(arg));
				}
			}
		}
	}

	return args;

#elifdef MINT_OS_MAC

	int argc = *_NSGetArgc();
	char** argv = *_NSGetArgv();

	return std::vector<std::string>(std::from_range, std::span(argv, argc));

#else
#error "Platform not supported"
#endif
}
