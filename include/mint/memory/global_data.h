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

#ifndef MINT_MEMORY_GLOBAL_DATA_H
#define MINT_MEMORY_GLOBAL_DATA_H

#include "mint/compiler/descriptions.h"
#include "mint/compiler/symbol_scope.h"
#include "mint/memory/memory_tools.h"
#include "mint/program/symbol.h"
#include "mint/config.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/garbage_collector.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/symbol_table.h"

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>

namespace mint {

class Program;

class MINT_EXPORT PackageData : public MemoryRoot<MemoryRootRegistrationMode::automatic> {
	SymbolTable _symbols;
	std::reference_wrapper<Program> _program;
	std::reference_wrapper<const PackageDescription> _description;
public:
	PackageData(Program& program, const PackageDescription& description);

	[[nodiscard]] std::string full_name() const;
	[[nodiscard]] Symbol name() const;
	[[nodiscard]] const PackageDescription& get_description() const;

	[[nodiscard]] Class* find_class(const Symbol& name) const;

	[[nodiscard]] inline const SymbolTable& symbols() const;
	[[nodiscard]] inline SymbolTable& symbols();

	[[nodiscard]] inline const Program& program() const;
	[[nodiscard]] inline Program& program();

	void mark() override;
};

class MINT_EXPORT GlobalData : public PackageData {
	friend class Program;
public:
	explicit GlobalData(Program& program, GlobalDataDescription& description);

	template<class BuiltinClass>
	BuiltinClass& builtin(Class::Metatype type);

	static inline Reference& none_ref();
	static inline Reference& null_ref();

	void cleanup_builtin();

private:
	std::array<std::unique_ptr<Class>, Class::builtin_class_count> _builtin;
};

template<class BuiltinClass>
BuiltinClass& GlobalData::builtin(Class::Metatype type) {
	const auto builtin_index = static_cast<std::size_t>(type);
	if (auto* instance = static_cast<BuiltinClass*>(_builtin[builtin_index].get())) {
		return *instance;
	}
	return *static_cast<BuiltinClass*>((_builtin[builtin_index] = std::make_unique<BuiltinClass>(program())).get());
}

Class* PackageData::find_class(const Symbol& name) const {
	if (const auto it = _symbols.find(name); it != _symbols.end() && it->second.data().format() == Data::Format::object
	                                         && is_class(it->second.data<Object>())) {
		return &it->second.data<Object>().metadata;
	}
	return nullptr;
}

const SymbolTable& PackageData::symbols() const {
	return _symbols;
}

SymbolTable& PackageData::symbols() {
	return _symbols;
}

const Program& PackageData::program() const {
	return _program;
}

Program& PackageData::program() {
	return _program;
}

Reference& GlobalData::none_ref() {
	return GarbageCollector::instance().none_ref();
}

Reference& GlobalData::null_ref() {
	return GarbageCollector::instance().null_ref();
}

}

#endif // MINT_MEMORY_GLOBAL_DATA_H
