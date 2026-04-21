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

#ifndef MINT_AST_CLASSREGISTER_H
#define MINT_AST_CLASSREGISTER_H

#include "mint/ast/symbol.h"
#include "mint/config.h"
#include "mint/memory/data.h"
#include "mint/memory/garbagecollector.h"
#include "mint/memory/reference.h"
#include "mint/memory/class.h"

#include <functional>
#include <initializer_list>
#include <memory>
#include <ranges>
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>

namespace mint {

class AbstractSyntaxTree;
class ClassDescription;
class FunctionData;
class PackageData;

class MINT_EXPORT ClassRegister {
public:
	class MINT_EXPORT Path {
	public:
		Path() = default;
		Path(Path&&) = default;
		Path(const Path& other) = default;
		Path(const Symbol& symbol);
		Path(std::initializer_list<Symbol> symbols);
		Path(const Path& other, const Symbol& symbol);
		Path(const std::string& path);
		~Path() = default;

		Path& operator=(Path&&) = default;
		Path& operator=(const Path&) = default;

		[[nodiscard]] const ClassDescription& locate(const ClassRegister& root_register) const;
		[[nodiscard]] ClassDescription& locate(ClassRegister& root_register) const;
		[[nodiscard]] std::string to_string() const;

		void append_symbol(const Symbol& symbol);
		void clear();

	private:
		std::vector<Symbol> _symbols;
	};

	static inline bool is_slot(const Reference& member);

	ClassRegister(AbstractSyntaxTree& ast);
	ClassRegister(ClassRegister&&) = delete;
	ClassRegister(const ClassRegister&) = delete;
	virtual ~ClassRegister() = default;

	ClassRegister& operator=(ClassRegister&&) = delete;
	ClassRegister& operator=(const ClassRegister&) = delete;

	[[nodiscard]] const ClassRegister& get_root_register() const;
	[[nodiscard]] ClassRegister& get_root_register();

	[[nodiscard]] const ClassRegister* get_owner_register() const;
	[[nodiscard]] ClassRegister* get_owner_register();
	void set_owner_register(ClassRegister* owner);

	[[nodiscard]] const PackageData* get_owner_package() const;
	[[nodiscard]] PackageData* get_owner_package();

	[[nodiscard]] ClassDescription* find_class_description(const Symbol& name) const;
	void register_class_description(ClassDescription& desc, Reference::Flags flags);

	[[nodiscard]] auto class_descriptions() const {
		return std::views::transform(_defined_classes,
		    [](const auto& entry) -> std::pair<ClassDescription&, Reference::Flags> {
			    return {entry.desc, entry.flags};
		    });
	}

	[[nodiscard]] virtual ClassRegister* locate(const Symbol& symbol) const;

	[[nodiscard]] virtual const FunctionData* get_function_data() const;
	[[nodiscard]] virtual FunctionData* get_function_data();

	[[nodiscard]] virtual const PackageData* get_package_data() const;
	[[nodiscard]] virtual PackageData* get_package_data();

	virtual void cleanup_memory();
	virtual void cleanup_metadata();

	[[nodiscard]] inline const AbstractSyntaxTree& ast() const;
	[[nodiscard]] inline AbstractSyntaxTree& ast();

private:
	struct ClassDescriptionEntry {
		std::reference_wrapper<ClassDescription> desc;
		Reference::Flags flags = Reference::default_flags;
	};

	ClassRegister* _owner = nullptr;
	std::vector<ClassDescriptionEntry> _defined_classes;
	std::reference_wrapper<AbstractSyntaxTree> _ast;
};

class MINT_EXPORT ClassDescription : public ClassRegister {
public:
	ClassDescription(AbstractSyntaxTree& ast, const std::string& name);

	[[nodiscard]] Symbol name() const;
	[[nodiscard]] std::string full_name() const;

	[[nodiscard]] Path get_path() const;
	void add_base(const Path& base);

	[[nodiscard]] const ClassDescription* get_owner_class() const;
	[[nodiscard]] ClassDescription* get_owner_class();

	[[nodiscard]] const Reference* find_member(const Symbol& name) const;
	bool create_member(const Symbol& name, const Reference& value);
	bool update_member(const Symbol& name, const Reference& value);

	[[nodiscard]] const std::vector<std::reference_wrapper<Class>>& bases() const;
	Class& generate();

	void cleanup_memory() override;
	void cleanup_metadata() override;

	void mark() {
		for (auto& member : _members) {
			member.second.data().mark();
		}
	}

private:
	std::unique_ptr<Class::MemberInfo> create_member_info(const Class::MemberInfo& member);
	Class::MemberInfo* update_member_info(const Symbol& symbol, WeakReference& value,
	    std::unordered_map<Symbol, std::vector<std::reference_wrapper<const Reference>>>& member_overrides);

	Symbol _name;
	std::vector<Path> _bases;
	std::unordered_map<Symbol, WeakReference> _members;

	std::unique_ptr<Class> _metadata;
	std::vector<std::reference_wrapper<Class>> _bases_metadata;
};

bool ClassRegister::is_slot(const Reference& member) {
	return ((member.flags() & (Reference::const_address | Reference::const_value))
	           != (Reference::const_address | Reference::const_value))
	       || member.data().format() == Data::Format::none;
}

const AbstractSyntaxTree& ClassRegister::ast() const {
	return _ast;
}

AbstractSyntaxTree& ClassRegister::ast() {
	return _ast;
}

}

#endif // MINT_AST_CLASSREGISTER_H
