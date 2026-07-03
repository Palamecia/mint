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

#ifndef MINT_AST_CLASS_DESCRIPTION_H
#define MINT_AST_CLASS_DESCRIPTION_H

#include "mint/ast/class_register.h"
#include "mint/ast/symbol.h"
#include "mint/config.h"
#include "mint/memory/class.h"
#include "mint/memory/reference.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mint {

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
	Class::MemberInfo* update_member_info(const Symbol& symbol, Reference& value,
	    std::unordered_map<Symbol, std::vector<std::reference_wrapper<const Reference>>>& member_overrides);

	Symbol _name;
	std::vector<Path> _bases;
	std::unordered_map<Symbol, Reference> _members;

	std::unique_ptr<Class> _metadata;
	std::vector<std::reference_wrapper<Class>> _bases_metadata;
};

}

#endif // MINT_AST_CLASS_DESCRIPTION_H
