#include "memory/memorytool.h"
#include "memory/globaldata.h"
#include "memory/objectprinter.h"
#include "memory/casttool.h"
#include "memory/builtin/string.h"
#include "memory/builtin/iterator.h"
#include "system/assert.h"
#include "system/error.h"
#include "ast/fileprinter.h"
#include "ast/cursor.h"

using namespace std;
using namespace mint;

static bool ensure_not_defined(const Symbol &symbol, SymbolTable &symbols) {

	auto it = symbols.find(symbol);
	if (it != symbols.end()) {
		if (UNLIKELY(it->second.data()->format != Data::fmt_none)) {
			return false;
		}
		symbols.erase(it);
	}

	return true;
}

string mint::type_name(const Reference &reference) {

	switch (reference.data()->format) {
	case Data::fmt_none:
		return "none";
	case Data::fmt_null:
		return "null";
	case Data::fmt_number:
		return "number";
	case Data::fmt_boolean:
		return "boolean";
	case Data::fmt_object:
		return reference.data<Object>()->metadata->name();
	case Data::fmt_package:
		return "package";
	case Data::fmt_function:
		return "function";
	}

	return string();
}

Printer *mint::create_printer(Cursor *cursor) {

	WeakReference ref = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	switch (ref.data()->format) {
	case Data::fmt_number:
		return new FilePrinter(static_cast<int>(ref.data<Number>()->value));
	case Data::fmt_object:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::string:
			return new FilePrinter(ref.data<String>()->str.c_str());
		case Class::object:
			return new ObjectPrinter(cursor, ref.flags(), ref.data<Object>());
		default:
			break;
		}
		fall_through;
	default:
		string type = type_name(ref);
		error("cannot open printer from '%s'", type.c_str());
	}
}

void mint::print(Printer *printer, Reference &reference) {

	assert(printer);

	printer->print(reference);
}

void mint::load_extra_arguments(Cursor *cursor) {

	WeakReference args = WeakReference::create(iterator_init(cursor->stack().back()));
	cursor->stack().pop_back();

	while (optional<WeakReference> &&item = iterator_next(args.data<Iterator>())) {
		cursor->stack().emplace_back(WeakReference(item->flags(), item->data()));
		cursor->waitingCalls().top().addExtraArgument();
	}
}

void mint::capture_symbol(Cursor *cursor, const Symbol &symbol) {

	Reference &function = cursor->stack().back();

	for (auto &signature : function.data<Function>()->mapping) {
		signature.second.capture->erase(symbol);
		auto item = cursor->symbols().find(symbol);
		if (item != cursor->symbols().end()) {
			signature.second.capture->emplace(item->first, WeakReference(item->second.flags(), item->second.data()));
		}
	}
}

void mint::capture_as_symbol(Cursor *cursor, const Symbol &symbol) {

	WeakReference reference = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	Reference &function = cursor->stack().back();

	for (auto &signature : function.data<Function>()->mapping) {
		signature.second.capture->erase(symbol);
		if ((reference.flags() & (Reference::const_value | Reference::temporary)) == Reference::const_value) {
			(*signature.second.capture)[symbol].copy(reference);
		}
		else {
			(*signature.second.capture)[symbol].move(reference);
		}
	}
}

void mint::capture_all_symbols(Cursor *cursor) {

	Reference &function = cursor->stack().back();

	for (auto &signature : function.data<Function>()->mapping) {
		signature.second.capture->clear();
		for (auto &item : cursor->symbols()) {
			signature.second.capture->emplace(item.first, WeakReference(item.second.flags(), item.second.data()));
		}
	}
}

void mint::init_call(Cursor *cursor) {

	if (cursor->stack().back().data()->format != Data::fmt_object) {
		cursor->waitingCalls().emplace(std::forward<Reference>(cursor->stack().back()));
		cursor->stack().pop_back();
	}
	else {

		Object *object = cursor->stack().back().data<Object>();
		Cursor::Call::Flags flags = Cursor::Call::member_call;
		Class *metadata = object->metadata;

		if (is_class(object)) {

			if (metadata->metatype() == Class::object) {
				WeakReference instance = WeakReference::clone(cursor->stack().back().data());
				object = instance.data<Object>();
				object->construct();
				cursor->stack().back() = std::move(instance);
			}
			else {
				/*
				 * Builtin classes can not be aliased, there is no need to clone the prototype.
				 */
				object->construct();
			}

			if (Class::MemberInfo *info = metadata->findOperator(Class::new_operator)) {

				switch (info->value.flags() & Reference::visibility_mask) {
				case Reference::protected_visibility:
					if (UNLIKELY(!is_protected_accessible(cursor, info->owner))) {
						string type = metadata->name();
						error("could not access protected member 'new' of class '%s'", type.c_str());
					}
					break;
				case Reference::private_visibility:
					if (UNLIKELY(!is_private_accessible(cursor, info->owner))) {
						string type = metadata->name();
						error("could not access private member 'new' of class '%s'", type.c_str());
					}
					break;
				case Reference::package_visibility:
					if (UNLIKELY(!is_package_accessible(cursor, info->owner))) {
						string type = metadata->name();
						error("could not access package member 'new' of class '%s'", type.c_str());
					}
					break;
				}

				cursor->waitingCalls().emplace(WeakReference::share(object->data[info->offset]));
				metadata = info->owner;
			}
			else {
				cursor->waitingCalls().emplace(WeakReference::create<None>());
			}
		}
		else if (Class::MemberInfo *info = metadata->findOperator(Class::call_operator)) {
			cursor->waitingCalls().emplace(WeakReference::share(object->data[info->offset]));
			flags |= Cursor::Call::operator_call;
			metadata = info->owner;
		}
		else {
			string type = metadata->name();
			error("class '%s' dosen't ovreload operator '()'", type.c_str());
		}

		Cursor::Call &call = cursor->waitingCalls().top();
		call.setMetadata(metadata);
		call.setFlags(flags);
	}
}

void mint::init_member_call(Cursor *cursor, const Symbol &member) {

	Class *owner = nullptr;
	Reference &reference = cursor->stack().back();
	WeakReference function = get_object_member(cursor, reference, member, &owner);

	if (function.flags() & Reference::global) {
		cursor->stack().pop_back();
	}

	cursor->stack().emplace_back(std::forward<Reference>(function));

	init_call(cursor);

	Cursor::Call &call = cursor->waitingCalls().top();

	if (!call.getMetadata()) {
		call.setMetadata(owner);
	}

	if (call.getFlags() & Cursor::Call::operator_call) {
		function = std::move(cursor->stack().back());
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(function));
	}
}

void mint::init_operator_call(Cursor *cursor, Class::Operator op) {

	Class *owner = nullptr;
	Reference &reference = cursor->stack().back();
	WeakReference function = get_object_operator(cursor, reference, op, &owner);

	if (function.flags() & Reference::global) {
		cursor->stack().pop_back();
	}

	cursor->stack().emplace_back(std::forward<Reference>(function));

	init_call(cursor);

	Cursor::Call &call = cursor->waitingCalls().top();

	if (!call.getMetadata()) {
		call.setMetadata(owner);
	}

	if (call.getFlags() & Cursor::Call::operator_call) {
		function = std::move(cursor->stack().back());
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(function));
	}
}

void mint::exit_call(Cursor *cursor) {
	cursor->exitCall();
}

void mint::init_exception(Cursor *cursor, const Symbol &symbol) {

	Reference &value = cursor->stack().back();
	SymbolTable &symbols = cursor->symbols();

	if (!ensure_not_defined(symbol, symbols)) {
		string symbol_str = symbol.str();
		error("symbol '%s' was already defined in this context", symbol_str.c_str());
	}

	symbols.emplace(symbol, value);
	cursor->stack().pop_back();
}

void mint::reset_exception(Cursor *cursor, const Symbol &symbol) {
	SymbolTable &symbols = cursor->symbols();
	symbols.erase(symbol);
}

void mint::init_parameter(Cursor *cursor, const Symbol &symbol, mint::Reference::Flags flags, size_t index) {

	Reference &value = cursor->stack().back();
	SymbolTable &symbols = cursor->symbols();

	if (flags & Reference::const_value) {
		symbols.setup_fast(symbol, index, flags).move(value);
	}
	else if ((value.flags() & (Reference::const_value | Reference::temporary)) == Reference::const_value) {
		symbols.setup_fast(symbol, index, flags).copy(value);
	}
	else {
		symbols.setup_fast(symbol, index, flags).move(value);
	}

	cursor->stack().pop_back();
}

Function::mapping_type::iterator mint::find_function_signature(Cursor *cursor, Function::mapping_type &mapping, int signature) {

	auto it = mapping.find(signature);

	if (it != mapping.end()) {
		return it;
	}

	it = mapping.lower_bound(-signature);

	if (it != mapping.end()) {

		const int required = -it->first;

		if (UNLIKELY(required <= 0)) {
			return mapping.end();
		}

		Iterator *va_args = Reference::alloc<Iterator>();
		va_args->construct();

		while (required < signature--) {
			va_args->ctx.emplace_front(std::forward<Reference>(cursor->stack().back()));
			cursor->stack().pop_back();
		}

		cursor->stack().emplace_back(WeakReference(Reference::standard, va_args));
	}

	return it;
}

void mint::yield(Cursor *cursor, Reference &generator) {
	WeakReference item = std::move(cursor->stack().back());
	cursor->stack().pop_back();
	iterator_insert(generator.data<Iterator>(), WeakReference(item.flags(), item.data()));
}

void mint::load_generator_result(Cursor *cursor) {
	((void)cursor);
}

WeakReference mint::get_symbol_reference(SymbolTable *symbols, const Symbol &symbol) {

	auto it_local = symbols->find(symbol);
	if (it_local != symbols->end()) {
		return WeakReference::share(it_local->second);
	}

	for (PackageData *package = symbols->getPackage(); package != nullptr; package = package->getPackage()) {
		auto it_package = package->symbols().find(symbol);
		if (it_package != package->symbols().end()) {
			return WeakReference::share(it_package->second);
		}
	}

	return WeakReference::share((*symbols)[symbol]);
}

WeakReference mint::get_object_member(Cursor *cursor, const Reference &reference, const Symbol &member, Class **owner) {

	switch (reference.data()->format) {
	case Data::fmt_package:
		for (PackageData *package = reference.data<Package>()->data; package != nullptr; package = package->getPackage()) {

			auto it_package = package->symbols().find(member);
			if (it_package != package->symbols().end()) {

				if (owner) {
					*owner = nullptr;
				}

				return WeakReference::share(it_package->second);
			}
		}

		if (PackageData *package = reference.data<Package>()->data) {
			string type = package->name();
			string member_str = member.str();
			error("package '%s' has no member '%s'", type.c_str(), member_str.c_str());
		}

		break;

	case Data::fmt_object:
		if (Object *object = reference.data<Object>()) {

			auto it_member = object->metadata->members().find(member);
			if (it_member != object->metadata->members().end()) {
				if (is_object(object)) {

					Reference &result = object->data[it_member->second->offset];

					switch (result.flags() & Reference::visibility_mask) {
					case Reference::protected_visibility:
						if (UNLIKELY(!is_protected_accessible(cursor, it_member->second->owner))) {
							string type = object->metadata->name();
							string member_str = member.str();
							error("could not access protected member '%s' of class '%s'", member_str.c_str(), type.c_str());
						}
						break;
					case Reference::private_visibility:
						if (UNLIKELY(!is_private_accessible(cursor, it_member->second->owner))) {
							string type = object->metadata->name();
							string member_str = member.str();
							error("could not access private member '%s' of class '%s'", member_str.c_str(), type.c_str());
						}
						break;
					case Reference::package_visibility:
						if (UNLIKELY(!is_package_accessible(cursor, it_member->second->owner))) {
							string type = object->metadata->name();
							string member_str = member.str();
							error("could not access package member '%s' of class '%s'", member_str.c_str(), type.c_str());
						}
						break;
					}

					if (owner) {
						*owner = it_member->second->owner;
					}

					return WeakReference::share(result);
				}
				else {

					if (UNLIKELY(cursor->isInBuiltin() || cursor->symbols().getMetadata() == nullptr)) {
						string type = object->metadata->name();
						string member_str = member.str();
						error("could not access member '%s' of class '%s' without object", member_str.c_str(), type.c_str());
					}
					if (UNLIKELY(cursor->symbols().getMetadata()->bases().find(object->metadata) == cursor->symbols().getMetadata()->bases().end())) {
						string type = object->metadata->name();
						string context_type = cursor->symbols().getMetadata()->name();
						error("class '%s' is not a direct base of '%s'", type.c_str(), context_type.c_str());
					}
					if (UNLIKELY((it_member->second->value.flags() & Reference::private_visibility) && (it_member->second->owner != cursor->symbols().getMetadata()))) {
						string type = object->metadata->name();
						string member_str = member.str();
						error("could not access private member '%s' of class '%s'", member_str.c_str(), type.c_str());
					}

					if (owner) {
						*owner = it_member->second->owner;
					}

					WeakReference result(Reference::const_address | Reference::const_value | Reference::global);
					result.copy(it_member->second->value);
					return result;
				}
			}

			auto it_global = object->metadata->globals().find(member);
			if (it_global != object->metadata->globals().end()) {

				Reference &result = it_global->second->value;

				if (result.data()->format != Data::fmt_none) {
					switch (result.flags() & Reference::visibility_mask) {
					case Reference::protected_visibility:
						if (UNLIKELY(!is_protected_accessible(cursor, it_global->second->owner))) {
							if (result.data()->format == Data::fmt_object && is_class(result.data<Object>())) {
								if (UNLIKELY(!is_protected_accessible(cursor, result.data<Object>()->metadata))) {
									string type = object->metadata->name();
									string member_str = member.str();
									error("could not access protected member type '%s' of class '%s'", member_str.c_str(), type.c_str());
								}
							}
							else {
								string type = object->metadata->name();
								string member_str = member.str();
								error("could not access protected member '%s' of class '%s'", member_str.c_str(), type.c_str());
							}
						}
						break;
					case Reference::private_visibility:
						if (UNLIKELY(!is_private_accessible(cursor, it_global->second->owner))) {
							if (result.data()->format == Data::fmt_object && is_class(result.data<Object>())) {
								if (UNLIKELY(!is_private_accessible(cursor, result.data<Object>()->metadata))) {
									string type = object->metadata->name();
									string member_str = member.str();
									error("could not access private member type '%s' of class '%s'", member_str.c_str(), type.c_str());
								}
							}
							else {
								string type = object->metadata->name();
								string member_str = member.str();
								error("could not access private member '%s' of class '%s'", member_str.c_str(), type.c_str());
							}
						}
						break;
					case Reference::package_visibility:
						if (UNLIKELY(!is_package_accessible(cursor, it_global->second->owner))) {
							if (result.data()->format == Data::fmt_object && is_class(result.data<Object>())) {
								if (UNLIKELY(!is_package_accessible(cursor, result.data<Object>()->metadata))) {
									string type = object->metadata->name();
									string member_str = member.str();
									error("could not access package member type '%s' of class '%s'", member_str.c_str(), type.c_str());
								}
							}
							else {
								string type = object->metadata->name();
								string member_str = member.str();
								error("could not access package member '%s' of class '%s'", member_str.c_str(), type.c_str());
							}
						}
						break;
					}
				}

				if (owner) {
					*owner = it_global->second->owner;
				}

				return WeakReference::share(result);
			}

			for (PackageData *package = object->metadata->getPackage(); package != nullptr; package = package->getPackage()) {

				auto it_package = package->symbols().find(member);
				if (it_package != package->symbols().end()) {

					if (owner) {
						*owner = nullptr;
					}

					return WeakReference(Reference::const_address | Reference::const_value, it_package->second.data());
				}
			}

			if (is_object(object)) {
				string type = object->metadata->name();
				string member_str = member.str();
				error("class '%s' has no member '%s'", type.c_str(), member_str.c_str());
			}
			else {
				string type = object->metadata->name();
				string member_str = member.str();
				error("class '%s' has no global member '%s'", type.c_str(), member_str.c_str());
			}
		}

		break;

	default:
		GlobalData *externals = GlobalData::instance();

		auto it_external = externals->symbols().find(member);
		if (it_external != externals->symbols().end()) {

			if (owner) {
				*owner = nullptr;
			}

			return WeakReference(Reference::const_address | Reference::const_value, it_external->second.data());
		}

		string member_str = member.str();
		error("non class values dosen't have member '%s'", member_str.c_str());
	}

	return WeakReference();
}

WeakReference mint::get_object_operator(Cursor *cursor, const Reference &reference, Class::Operator op, Class **owner) {

	switch (reference.data()->format) {
	case Data::fmt_object:
		if (Class::MemberInfo *member = reference.data<Object>()->metadata->findOperator(op)) {

			Object *object = reference.data<Object>();

			if (is_object(object)) {

				Reference &result = object->data[member->offset];

				switch (result.flags() & Reference::visibility_mask) {
				case Reference::protected_visibility:
					if (UNLIKELY(!is_protected_accessible(cursor, member->owner))) {
						string type = object->metadata->name();
						string operator_str = get_operator_symbol(op).str();
						error("could not access protected member '%s' of class '%s'", operator_str.c_str(), type.c_str());
					}
					break;
				case Reference::private_visibility:
					if (UNLIKELY(!is_private_accessible(cursor, member->owner))) {
						string type = object->metadata->name();
						string operator_str = get_operator_symbol(op).str();
						error("could not access private member '%s' of class '%s'", operator_str.c_str(), type.c_str());
					}
					break;
				case Reference::package_visibility:
					if (UNLIKELY(!is_package_accessible(cursor, member->owner))) {
						string type = object->metadata->name();
						string operator_str = get_operator_symbol(op).str();
						error("could not access package member '%s' of class '%s'", operator_str.c_str(), type.c_str());
					}
					break;
				}

				if (owner) {
					*owner = member->owner;
				}

				return WeakReference::share(result);
			}
			else {

				if (UNLIKELY(cursor->isInBuiltin() || cursor->symbols().getMetadata() == nullptr)) {
					string type = object->metadata->name();
					string operator_str = get_operator_symbol(op).str();
					error("could not access member '%s' of class '%s' without object", operator_str.c_str(), type.c_str());
				}
				if (UNLIKELY(cursor->symbols().getMetadata()->bases().find(object->metadata) == cursor->symbols().getMetadata()->bases().end())) {
					string type = object->metadata->name();
					string context_type = cursor->symbols().getMetadata()->name();
					error("class '%s' is not a direct base of '%s'", type.c_str(), context_type.c_str());
				}
				if (UNLIKELY((member->value.flags() & Reference::private_visibility) && (member->owner != cursor->symbols().getMetadata()))) {
					string type = object->metadata->name();
					string operator_str = get_operator_symbol(op).str();
					error("could not access private member '%s' of class '%s'", operator_str.c_str(), type.c_str());
				}

				if (owner) {
					*owner = member->owner;
				}

				WeakReference result(Reference::const_address | Reference::const_value | Reference::global);
				result.copy(member->value);
				return result;
			}
		}

		if (is_object(reference.data<Object>())) {
			string type = reference.data<Object>()->metadata->name();
			string operator_str = get_operator_symbol(op).str();
			error("class '%s' has no member '%s'", type.c_str(), operator_str.c_str());
		}
		else {
			string type = reference.data<Object>()->metadata->name();
			string operator_str = get_operator_symbol(op).str();
			error("class '%s' has no global member '%s'", type.c_str(), operator_str.c_str());
		}

	default:
		string operator_str = get_operator_symbol(op).str();
		error("non class values dosen't have member '%s'", operator_str.c_str());
	}
}

void mint::reduce_member(Cursor *cursor, Reference &&member) {
	cursor->stack().back() = std::move(member);
}

Class::MemberInfo *mint::get_member_infos(Object *object, const Reference &member) {

	for (auto &infos : object->metadata->members()) {
		if (member.data() == infos.second->value.data()) {
			return infos.second;
		}
		if (member.data() == object->data[infos.second->offset].data()) {
			return infos.second;
		}
	}

	return nullptr;
}

bool mint::is_protected_accessible(Class *owner, Class *context) {
	return owner->isBaseOrSame(context) || (context && context->isBaseOf(owner));
}

bool mint::is_protected_accessible(Cursor *cursor, Class *owner) {
	return !cursor->isInBuiltin() && is_protected_accessible(owner, cursor->symbols().getMetadata());
}

bool mint::is_private_accessible(Cursor *cursor, Class *owner) {
	return !cursor->isInBuiltin() && owner == cursor->symbols().getMetadata();
}

bool mint::is_package_accessible(Cursor *cursor, Class *owner) {
	return !cursor->isInBuiltin() && owner->getPackage() == cursor->symbols().getPackage();
}

Symbol mint::var_symbol(Cursor *cursor) {

	WeakReference var = std::move(cursor->stack().back());
	cursor->stack().pop_back();
	return Symbol(to_string(var));
}

void mint::create_symbol(Cursor *cursor, const Symbol &symbol, Reference::Flags flags) {

	if (flags & Reference::global) {

		PackageData *package = cursor->symbols().getPackage();

		if (UNLIKELY(!ensure_not_defined(symbol, package->symbols()))) {
			string symbol_str = symbol.str();
			error("symbol '%s' was already defined in global context", symbol_str.c_str());
		}

		cursor->stack().emplace_back(WeakReference::share(package->symbols().emplace(symbol, WeakReference(flags)).first->second));
	}
	else {

		if (UNLIKELY(!ensure_not_defined(symbol, cursor->symbols()))) {
			string symbol_str = symbol.str();
			error("symbol '%s' was already defined in this context", symbol_str.c_str());
		}

		cursor->stack().emplace_back(WeakReference::share(cursor->symbols().emplace(symbol, WeakReference(flags)).first->second));
	}
}

void mint::create_symbol(Cursor *cursor, const Symbol &symbol, size_t index, Reference::Flags flags) {

	if (flags & Reference::global) {

		PackageData *package = cursor->symbols().getPackage();

		if (UNLIKELY(!ensure_not_defined(symbol, package->symbols()))) {
			string symbol_str = symbol.str();
			error("symbol '%s' was already defined in global context", symbol_str.c_str());
		}

		cursor->stack().emplace_back(WeakReference::share(package->symbols().emplace(symbol, WeakReference::share(cursor->symbols().setup_fast(symbol, index, flags))).first->second));
	}
	else {

		if (UNLIKELY(!ensure_not_defined(symbol, cursor->symbols()))) {
			string symbol_str = symbol.str();
			error("symbol '%s' was already defined in this context", symbol_str.c_str());
		}

		cursor->stack().emplace_back(WeakReference::share(cursor->symbols().setup_fast(symbol, index, flags)));
	}
}

void mint::create_function(Cursor *cursor, const Symbol &symbol, Reference::Flags flags) {

	assert(flags & Reference::global);

	PackageData *package = cursor->symbols().getPackage();

	auto it = package->symbols().find(symbol);
	if (it != package->symbols().end()) {
		switch (it->second.data()->format) {
		case Data::fmt_none:
			it->second = WeakReference(flags, Reference::alloc<Function>());
			break;
		case Data::fmt_function:
			if (UNLIKELY(flags != it->second.flags())) {
				string symbol_str = symbol.str();
				error("function '%s' was already defined in global context", symbol_str.c_str());
			}
			break;
		default:
			string symbol_str = symbol.str();
			error("symbol '%s' was already defined in this context", symbol_str.c_str());
		}
	}
	else {
		it = package->symbols().emplace(symbol, WeakReference(flags, Reference::alloc<Function>())).first;
	}

	cursor->stack().emplace_back(WeakReference::share(it->second));
}

void mint::function_overload_from_stack(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &signature = load_from_stack(cursor, base);
	Reference &function = load_from_stack(cursor, base - 1);

	for (const auto &item : signature.data<Function>()->mapping) {
		if (UNLIKELY(!function.data<Function>()->mapping.insert(item).second)) {
			error("defined function already takes %d%s parameter(s)", abs(item.first), item.first < 0 ? "..." : "");
		}
	}

	cursor->stack().pop_back();
}
