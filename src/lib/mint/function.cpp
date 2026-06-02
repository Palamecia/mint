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

#include "mint/memory/builtin/libobject.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/reference.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/operator_tools.h"
#include "mint/ast/cursor.h"
#include <algorithm>
#include <string>
#include <utility>

namespace {

std::string get_member_name(const mint::Class::MemberInfo& infos) {

	mint::Class& metadata = infos.owner.get();
	const auto members = metadata.members();

	auto it = std::ranges::find(members, &infos, [](const auto& member) {
		return &member.second.get();
	});
	if (it != members.end()) {
		return metadata.full_name() + "." + (*it).first.str();
	}
	return metadata.full_name() + ".<function>";
}

mint::Reference mint_get_member_info(mint::Cursor& cursor, const mint::Reference& object,
    const mint::Reference& member) {
	if (is_instance_of(object, mint::Data::Format::object)) {
		if (const auto* infos = find_member_info(object.data<mint::Object>(), member)) {
			return create_c_object(cursor.ast(), infos);
		}
	}
	return {};
}

mint::Reference mint_function_name(mint::Cursor& cursor, const mint::Reference& infos) {
	return mint::create_string(cursor.ast(),
	    get_member_name(*infos.data<mint::LibObject<const mint::Class::MemberInfo>>().ptr));
}

}

MINT_RAW_FUNCTION(mint_function_call, 4, cursor) {

	const auto args = std::move(cursor.stack().back());
	cursor.stack().pop_back();

	auto func = std::move(cursor.stack().back());
	cursor.stack().pop_back();

	auto object = std::move(cursor.stack().back());
	cursor.stack().pop_back();

	const auto member_info = std::move(cursor.stack().back());
	cursor.stack().pop_back();

	const auto signature = static_cast<int>(args.data<mint::Iterator>().ctx.size());
	cursor.stack().emplace_back(std::move(object));
	cursor.stack().append_range(args.data<mint::Iterator>().ctx);

	cursor.waiting_calls().emplace(std::move(func),
	    member_info.data<mint::LibObject<const mint::Class::MemberInfo>>().ptr->owner);

	call_member_operator(cursor, signature);
}

MINT_EXPORT_FUNCTION(mint_get_member_info, 2);
MINT_EXPORT_FUNCTION(mint_function_name, 1);
