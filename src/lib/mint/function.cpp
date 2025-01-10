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

#include <mint/memory/functiontool.h>
#include <mint/memory/builtin/iterator.h>
#include <mint/memory/memorytool.h>
#include <mint/memory/operatortool.h>
#include <mint/ast/cursor.h>

#include <optional>

using namespace mint;

static const std::string get_member_name(Class::MemberInfo *infos) {

	Class *metadata = infos->owner;
	const Class::MembersMapping &members = metadata->members();

	auto it = std::find_if(members.begin(), members.end(), [infos](const auto &member) {
		return infos == member.second;
	});
	if (it != members.end()) {
		return metadata->full_name() + "." + it->first.str();
	}
	return metadata->full_name() + ".<function>";
}

MINT_FUNCTION(mint_get_member_info, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &member = helper.pop_parameter();
	Reference &object = helper.pop_parameter();

	if (object.data()->format == Data::FMT_OBJECT) {
		if (Class::MemberInfo *infos = find_member_info(object.data<Object>(), member)) {
			helper.return_value(create_object(infos));
		}
	}
}

MINT_FUNCTION(mint_function_name, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	Class::MemberInfo *infos = helper.pop_parameter().data<LibObject<Class::MemberInfo>>()->impl;
	helper.return_value(create_string(get_member_name(infos)));
}

MINT_FUNCTION(mint_function_call, 4, cursor) {

	WeakReference args = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	WeakReference func = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	WeakReference object = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	WeakReference member_info = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	int signature = static_cast<int>(args.data<Iterator>()->ctx.size());

	cursor->stack().emplace_back(std::move(object));
	cursor->stack().insert(cursor->stack().end(),
						   std::make_move_iterator(args.data<Iterator>()->ctx.begin()),
						   std::make_move_iterator(args.data<Iterator>()->ctx.end()));

	cursor->waiting_calls().emplace(std::move(func));
	cursor->waiting_calls().top().set_metadata(member_info.data<LibObject<Class::MemberInfo>>()->impl->owner);

	call_member_operator(cursor, signature);
}
