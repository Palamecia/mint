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

#ifndef MINT_MEMORY_GLOBALDATA_H
#define MINT_MEMORY_GLOBALDATA_H

#include "mint/ast/classregister.h"
#include "mint/ast/symbol.h"
#include "mint/config.h"
#include "mint/memory/data.h"
#include "mint/memory/garbagecollector.h"
#include "mint/memory/reference.h"
#include "mint/memory/symboltable.h"

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <utility>

namespace mint {

class MINT_EXPORT PackageData : public ClassRegister {
public:
	explicit PackageData(AbstractSyntaxTree& ast, const std::string& name, PackageData* owner = nullptr);
	PackageData(PackageData&&) = delete;
	PackageData(const PackageData&) = delete;
	~PackageData() override;

	PackageData& operator=(PackageData&&) = delete;
	PackageData& operator=(const PackageData&) = delete;

	[[nodiscard]] Symbol name() const;
	[[nodiscard]] std::string full_name() const;
	[[nodiscard]] Path get_path() const;

	[[nodiscard]] PackageData* get_owner_package() const;
	[[nodiscard]] PackageData& get_package(const Symbol& name);
	[[nodiscard]] PackageData* find_package(const Symbol& name) const;

	[[nodiscard]] auto packages() {
		return std::views::transform(_packages,
		    [](auto& item) -> std::pair<Symbol, std::reference_wrapper<PackageData>> {
			    return {item.first, *item.second};
		    });
	}

	void register_class(Id id);
	[[nodiscard]] Class* find_class(const Symbol& name) const;

	[[nodiscard]] auto classes() {
		return std::views::filter(_symbols, [](auto& item) {
			return item.second.data().format() == Data::Format::object
			       && item.second.template data<Object>().data == nullptr;
		}) | std::views::transform([](auto& item) -> std::pair<Symbol, std::reference_wrapper<Class>> {
			return {item.first, item.second.template data<Object>().metadata};
		});
	}

	[[nodiscard]] inline const SymbolTable& symbols() const;
	[[nodiscard]] inline SymbolTable& symbols();

	void cleanup_memory() override;
	void cleanup_metadata() override;

private:
	Symbol _name;
	PackageData* _owner;
	std::unordered_map<Symbol, std::unique_ptr<PackageData>> _packages;
	SymbolTable _symbols;
};

class MINT_EXPORT GlobalData : public PackageData {
	friend class AbstractSyntaxTree;
public:
	GlobalData(AbstractSyntaxTree& ast);

	template<class BuiltinClass>
	BuiltinClass& builtin(Class::Metatype type);

	static inline Reference& none_ref();
	static inline Reference& null_ref();

	void cleanup_builtin();

private:
	std::array<std::unique_ptr<Class>, Class::builtin_class_count> _builtin;
};

const SymbolTable& PackageData::symbols() const {
	return _symbols;
}

SymbolTable& PackageData::symbols() {
	return _symbols;
}

template<class BuiltinClass>
BuiltinClass& GlobalData::builtin(Class::Metatype type) {
	const auto builtin_index = static_cast<std::size_t>(type);
	if (auto* instance = static_cast<BuiltinClass*>(_builtin[builtin_index].get())) {
		return *instance;
	}
	return *static_cast<BuiltinClass*>((_builtin[builtin_index] = std::make_unique<BuiltinClass>(ast())).get());
}

Reference& GlobalData::none_ref() {
	return GarbageCollector::instance().none_ref();
}

Reference& GlobalData::null_ref() {
	return GarbageCollector::instance().null_ref();
}

}

#endif // MINT_MEMORY_GLOBALDATA_H
